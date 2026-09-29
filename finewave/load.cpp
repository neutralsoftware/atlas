//
// load.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Function to load and parse audio files
// Copyright (c) 2025 Max Van den Eynde
//

#include "finewave/audio.h"
#include "atlas/tracer/log.h"
#include "atlas/workspace.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <fstream>
#include <memory>
#include <vector>
#include <iostream>
#include <cstring>

#define DR_MP3_IMPLEMENTATION
#include "dr/dr_mp3.h"

struct WavHeader {
    char riff[4]; // "RIFF"
    unsigned int chunkSize;
    char wave[4];               // "WAVE"
    char fmt[4];                // "fmt "
    unsigned int subchunk1Size; // 16 for PCM
    unsigned short audioFormat; // 1 = PCM
    unsigned short numChannels;
    unsigned int sampleRate;
    unsigned int byteRate;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
    char data[4];          // "data"
    unsigned int dataSize; // size of raw PCM data
};

struct Mp3Data {
    std::vector<float> samples;
    unsigned int sampleRate;
    unsigned int numChannels;
};

const char *getALErrorString(ALenum error) {
    switch (error) {
    case AL_NO_ERROR:
        return "AL_NO_ERROR";
    case AL_INVALID_NAME:
        return "AL_INVALID_NAME";
    case AL_INVALID_ENUM:
        return "AL_INVALID_ENUM";
    case AL_INVALID_VALUE:
        return "AL_INVALID_VALUE";
    case AL_INVALID_OPERATION:
        return "AL_INVALID_OPERATION";
    case AL_OUT_OF_MEMORY:
        return "AL_OUT_OF_MEMORY";
    default:
        return "UNKNOWN_AL_ERROR";
    }
}

#define CHECK_AL_ERROR()                                                       \
    {                                                                          \
        ALenum err = alGetError();                                             \
        if (err != AL_NO_ERROR) {                                              \
            std::cerr << "OpenAL error: " << getALErrorString(err) << " ("     \
                      << err << ") at " << __FILE__ << ":" << __LINE__         \
                      << std::endl;                                            \
        }                                                                      \
    }

std::shared_ptr<AudioData> AudioData::fromResource(Resource resource) {
    bool isMono = false;
    CHECK_AL_ERROR();
    if (resource.type != ResourceType::Audio) {
        atlas_error("Resource is not of type Audio: " + resource.name);
        throw std::invalid_argument("Resource is not of type Audio");
    }

    if (resource.path.extension() == ".mp3") {
        Mp3Data data{};
        drmp3 mp3;
#ifdef _WIN32
        // std::filesystem::path is wide on Windows.
        if (!drmp3_init_file_w(&mp3, resource.path.c_str(), nullptr)) {
#else
        if (!drmp3_init_file(&mp3, resource.path.c_str(), nullptr)) {
#endif
            atlas_error("Failed to open MP3 file: " + resource.path.string());
            throw std::runtime_error("Failed to open MP3 file");
        }

        data.numChannels = mp3.channels;
        data.sampleRate = mp3.sampleRate;

        drmp3_uint64 totalFrameCount = drmp3_get_pcm_frame_count(&mp3);
        data.samples.resize(totalFrameCount * data.numChannels);
        drmp3_read_pcm_frames_f32(&mp3, totalFrameCount, data.samples.data());
        drmp3_uninit(&mp3);

        std::vector<int16_t> int16Samples(data.samples.size());
        for (size_t i = 0; i < data.samples.size(); ++i) {
            int16Samples[i] = static_cast<int16_t>(data.samples[i] * 32767.0f);
        }

        ALenum format;
        if (data.numChannels == 1) {
            format = AL_FORMAT_MONO16;
            isMono = true;
        } else if (data.numChannels == 2) {
            format = AL_FORMAT_STEREO16;
            isMono = false;
        } else {
            throw std::runtime_error("Unsupported number of channels: " +
                                     std::to_string(data.numChannels));
        }

        ALuint buffer;
        alGenBuffers(1, &buffer);
        CHECK_AL_ERROR();
        if (!alIsBuffer(buffer)) {
            std::cerr << "Failed to generate OpenAL buffer" << std::endl;
            throw std::runtime_error("Failed to generate OpenAL buffer");
        }

        alBufferData(buffer, format, int16Samples.data(),
                     int16Samples.size() * sizeof(int16_t), data.sampleRate);
        CHECK_AL_ERROR();

        auto audioData = std::make_shared<AudioData>();
        audioData->id = buffer;
        audioData->isMono = isMono;
        audioData->resource = resource;
        std::vector<char> dataChar;
        dataChar.resize(int16Samples.size() * sizeof(int16_t));
        std::memcpy(dataChar.data(), int16Samples.data(),
                    int16Samples.size() * sizeof(int16_t));
        audioData->data = std::move(dataChar);
        audioData->sampleRate = data.sampleRate;
        return audioData;
    }

    std::ifstream file(resource.path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open audio file: " +
                                 resource.path.string());
    }

    WavHeader header;
    file.read(reinterpret_cast<char *>(&header), sizeof(WavHeader));

    if (std::string(header.riff, 4) != "RIFF" ||
        std::string(header.wave, 4) != "WAVE") {
        throw std::runtime_error("Invalid WAV file format: " +
                                 resource.path.string());
    }

    std::vector<char> data(header.dataSize);
    file.read(data.data(), header.dataSize);

    ALenum format;
    if (header.numChannels == 1) {
        format =
            (header.bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
        isMono = true;
    } else if (header.numChannels == 2) {
        format = (header.bitsPerSample == 8) ? AL_FORMAT_STEREO8
                                             : AL_FORMAT_STEREO16;
    } else {
        throw std::runtime_error("Unsupported number of channels: " +
                                 std::to_string(header.numChannels));
    }

    ALuint buffer;
    alGenBuffers(1, &buffer);
    CHECK_AL_ERROR();

    if (!alIsBuffer(buffer)) {
        std::cerr << "Failed to generate OpenAL buffer" << std::endl;
        throw std::runtime_error("Failed to generate OpenAL buffer");
    }

    alBufferData(buffer, format, data.data(), header.dataSize,
                 header.sampleRate);
    CHECK_AL_ERROR();

    auto audioData = std::make_shared<AudioData>();
    audioData->id = buffer;
    audioData->isMono = isMono;
    audioData->data = std::move(data);
    audioData->resource = resource;
    return audioData;
}

AudioData::~AudioData() {
    if (alIsBuffer(id)) {
        alDeleteBuffers(1, &id);
    }
}
