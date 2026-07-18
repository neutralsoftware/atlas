//
// audio.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Audio definitions and declarations
// Copyright (c) 2025 Max Van den Eynde
//

#ifndef FINEWAVE_AUDIO_H
#define FINEWAVE_AUDIO_H

#include "atlas/units.h"
#include "atlas/workspace.h"
#include "finewave/effect.h"
#include <AL/alc.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief Central audio engine that manages the audio system and global audio
 * settings.
 *
 */
class AudioEngine {
  public:
    /**
     * @brief Initializes the audio engine.
     *
     * @return (bool) True if initialization succeeded, false otherwise.
     */
    bool initialize();
    /**
     * @brief Shuts down the audio engine and releases resources.
     *
     */
    void shutdown();
    ~AudioEngine();

    /**
     * @brief Sets the position of the audio listener in 3D space.
     *
     * @param position The new listener position.
     */
    void setListenerPosition(Position3d position);
    /**
     * @brief Sets the orientation of the audio listener.
     *
     * @param forward The forward direction vector.
     * @param up The up direction vector.
     */
    void setListenerOrientation(Magnitude3d forward, Normal3d up);
    /**
     * @brief Sets the velocity of the audio listener for Doppler effects.
     *
     * @param velocity The listener velocity vector.
     */
    void setListenerVelocity(Magnitude3d velocity);
    /**
     * @brief Sets the master volume for all audio.
     *
     * @param volume Volume level (0.0 to 1.0).
     */
    void setMasterVolume(float volume);

    /**
     * @brief Name of the currently selected playback device.
     */
    std::string deviceName;

  private:
    ALCdevice *device = nullptr;
    ALCcontext *context = nullptr;
};

/**
 * @brief Class representing loaded audio data that can be played by AudioSource
 * instances.
 *
 */
class AudioData {
  public:
    /**
     * @brief Creates audio data from a resource file.
     *
     * @param resource The resource containing audio data.
     * @return (std::shared_ptr<AudioData>) Shared pointer to the created audio
     * data.
     */
    static std::shared_ptr<AudioData> fromResource(Resource resource);
    /**
     * @brief Destructor for AudioData.
     *
     */
    ~AudioData();

    /**
     * @brief Gets the internal ID of the audio data.
     *
     * @return (Id) The audio data ID.
     */
    Id getId() const { return id; }

    /**
     * @brief Indicates whether the decoded data contains a single channel.
     */
    bool isMono = false;

    /**
     * @brief Source resource used to create this buffer.
     */
    Resource resource;

  private:
    /** @brief Backend buffer identifier. */
    Id id;
    /** @brief Raw decoded PCM bytes. */
    std::vector<char> data;
    /** @brief Sample rate in Hz. */
    unsigned int sampleRate;
    friend class AudioSource;
};

/**
 * @brief Class representing an audio source that can play audio data with 3D
 * spatial positioning.
 *
 * \subsection audio-source-example Example
 * ```cpp
 * // Create an audio source
 * AudioSource audioSource;
 * // Load audio from a resource
 * audioSource.fromFile(Workspace::get().getResource("explosion"));
 * // Set 3D position
 * audioSource.setPosition({5.0, 0.0, 10.0});
 * // Play the audio
 * audioSource.play();
 * ```
 *
 */
class AudioSource {
  public:
    /**
     * @brief Constructs a new AudioSource.
     *
     */
    AudioSource();
    /**
     * @brief Destructor for AudioSource.
     *
     */
    ~AudioSource();

    /**
     * @brief Sets the audio data for this source.
     *
     * @param buffer Shared pointer to audio data.
     */
    void setData(const std::shared_ptr<AudioData> &buffer);
    /**
     * @brief Loads audio data directly from a resource file.
     *
     * @param resource The resource containing audio data.
     */
    void fromFile(Resource resource);

    /**
     * @brief Starts playing the audio.
     *
     */
    void play();
    /**
     * @brief Pauses the audio playback.
     *
     */
    void pause();
    /**
     * @brief Stops the audio playback.
     *
     */
    void stop();

    /**
     * @brief Sets whether the audio should loop.
     *
     * @param loop True to enable looping, false to disable.
     */
    void setLooping(bool loop);
    /**
     * @brief Sets the volume of the audio source.
     *
     * @param volume Volume level (0.0 to 1.0).
     */
    void setVolume(float volume);
    /**
     * @brief Sets the pitch of the audio source.
     *
     * @param pitch Pitch multiplier (1.0 is normal pitch).
     */
    void setPitch(float pitch);

    /**
     * @brief Sets the 3D position of the audio source.
     *
     * @param position The new position in 3D space.
     */
    void setPosition(Position3d position);
    /**
     * @brief Sets the velocity of the audio source for Doppler effects.
     *
     * @param velocity The velocity vector.
     */
    void setVelocity(Magnitude3d velocity);

    /**
     * @brief Checks if the audio is currently playing.
     *
     * @return (bool) True if playing, false otherwise.
     */
    bool isPlaying() const;
    /**
     * @brief Starts playing from a specific time position.
     *
     * @param seconds Time position in seconds to start from.
     */
    void playFrom(float seconds);

    /**
     * @brief Disables 3D spatialization for this source.
     *
     */
    void disableSpatialization();
    /**
     * @brief Applies an audio effect to this source.
     *
     * @param effect The audio effect to apply.
     */
    void applyEffect(const AudioEffect &effect) const;

    /**
     * @brief Gets the current position of the audio source.
     *
     * @return (Position3d) The current 3D position.
     */
    Position3d getPosition() const;
    /**
     * @brief Gets the position of the audio listener.
     *
     * @return (Position3d) The listener's 3D position.
     */
    Position3d getListenerPosition() const;

    /**
     * @brief Enables 3D spatialization for this source.
     *
     */
    void useSpatialization();

  private:
    /** @brief Backend source identifier. */
    Id id;
    /** @brief Optional mono companion source used by some backends. */
    Id monoId;
    /** @brief Bound audio data currently assigned to this source. */
    std::shared_ptr<AudioData> data;
    /** @brief Mono conversion cache when required by backend features. */
    std::shared_ptr<AudioData> monoData;
    /** @brief True when source attenuation is spatialized in 3D. */
    bool isSpatialized = false;
};

#endif // FINEWAVE_AUDIO_H
