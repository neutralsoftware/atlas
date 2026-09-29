//
// biome.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Biome generation functions
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/texture.h"
#include "aurora/terrain.h"
#include "opal/opal.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

float aurora::computeSlope(const uint8_t *heightMap, int width, int height,
                           int x, int y) {
    int xm = std::max(x - 1, 0);
    int xp = std::min(x + 1, width - 1);
    int ym = std::max(y - 1, 0);
    int yp = std::min(y + 1, height - 1);

    float dzdx =
        float(heightMap[xp + (y * width)]) - float(heightMap[xm + (y * width)]);
    float dzdy =
        float(heightMap[x + (yp * width)]) - float(heightMap[x + (ym * width)]);

    return std::sqrt((dzdx * dzdx) + (dzdy * dzdy)) / 255.0f;
}

void Terrain::generateMaps(unsigned char *heightmapData, int width, int height,
                           int generationParameter, int nChannels) {
    const float maxHeight = 255.0f;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = x + (y * width);

            uint8_t heightValue;
            if (nChannels == 1) {
                heightValue = heightmapData[idx];
            } else if (nChannels >= 3) {
                int r = heightmapData[(idx * nChannels) + 0];
                int g = heightmapData[(idx * nChannels) + 1];
                int b = heightmapData[(idx * nChannels) + 2];
                heightValue = static_cast<uint8_t>((r + g + b) / 3);
            } else {
                heightValue = 0;
            }

            if (generationParameter == 1) {
                float temperature = std::max(
                    0.0f, 1.0f - (float(heightValue) / float(maxHeight)));
                uint8_t temperatureValue =
                    static_cast<uint8_t>(temperature * 255.0f);
                this->temperatureData.push_back(temperatureValue);
            }

            if (generationParameter == 2) {
                float slope =
                    aurora::computeSlope(heightmapData, width, height, x, y);
                float moisture = (1.0f - slope) *
                                 (1.0f - float(heightValue) / float(maxHeight));
                moisture = std::clamp(moisture, 0.0f, 1.0f);
                uint8_t moistureValue = static_cast<uint8_t>(moisture * 255.0f);

                this->moistureData.push_back(moistureValue);
            }
        }
    }
}

void Terrain::generateBiomes(unsigned char *heightmapData, int width,
                             int height, int nChannels) {
    if (moistureTexture.id == 0) {
        generateMaps(heightmapData, width, height, 2, nChannels);
    } else {
        std::vector<uint8_t> data(width * height * 4);
        moistureTexture.texture->readData(data.data(),
                                          opal::TextureDataFormat::Rgba);

        for (int i = 0; i < width * height; ++i) {
            this->moistureData.push_back(data[i * 4]);
        }
    }

    if (temperatureTexture.id == 0) {
        generateMaps(heightmapData, width, height, 1, nChannels);
    } else {
        std::vector<uint8_t> data(width * height * 4);
        temperatureTexture.texture->readData(data.data(),
                                             opal::TextureDataFormat::Rgba);

        for (int i = 0; i < width * height; ++i) {
            this->temperatureData.push_back(data[i * 4]);
        }
    }

    if (biomesTexture.id != 0) {
        return;
    }

    moistureTexture.texture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::Red8, width, height,
        opal::TextureDataFormat::Red, moistureData.data(), 1);
    moistureTexture.texture->setWrapMode(opal::TextureAxis::S,
                                         opal::TextureWrapMode::Repeat);
    moistureTexture.texture->setWrapMode(opal::TextureAxis::T,
                                         opal::TextureWrapMode::Repeat);
    moistureTexture.texture->setFilterMode(opal::TextureFilterMode::Linear,
                                           opal::TextureFilterMode::Linear);
    moistureTexture.id = moistureTexture.texture->textureID;

    temperatureTexture.texture = opal::Texture::create(
        opal::TextureType::Texture2D, opal::TextureFormat::Red8, width, height,
        opal::TextureDataFormat::Red, temperatureData.data(), 1);
    temperatureTexture.texture->setWrapMode(opal::TextureAxis::S,
                                            opal::TextureWrapMode::Repeat);
    temperatureTexture.texture->setWrapMode(opal::TextureAxis::T,
                                            opal::TextureWrapMode::Repeat);
    temperatureTexture.texture->setFilterMode(opal::TextureFilterMode::Linear,
                                              opal::TextureFilterMode::Linear);
    temperatureTexture.id = temperatureTexture.texture->textureID;

    for (auto &biome : biomes) {
        biome.condition(biome);
    }
}
