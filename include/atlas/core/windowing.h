#ifndef ATLAS_WINDOWING_H
#define ATLAS_WINDOWING_H

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdint>

/**
 * @brief Returns the drawable size of an SDL window in physical pixels.
 *
 * This helper falls back to the logical window size multiplied by the display
 * scale when SDL cannot provide the pixel size directly.
 *
 * @param window The SDL window to inspect.
 * @param width Output pointer for the pixel width. May be nullptr.
 * @param height Output pointer for the pixel height. May be nullptr.
 */
inline void atlasGetWindowSizeInPixels(SDL_Window *window, int *width,
                                       int *height) {
    int pixelWidth = 0;
    int pixelHeight = 0;
    if (window != nullptr &&
        !SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight)) {
        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        float scale = SDL_GetWindowDisplayScale(window);
        if (scale <= 0.0f) {
            scale = 1.0f;
        }
        pixelWidth = std::max(
            1, static_cast<int>(static_cast<float>(windowWidth) * scale));
        pixelHeight = std::max(
            1, static_cast<int>(static_cast<float>(windowHeight) * scale));
    }
    if (window != nullptr) {
        pixelWidth = std::max(1, pixelWidth);
        pixelHeight = std::max(1, pixelHeight);
    }
    if (width != nullptr) {
        *width = pixelWidth;
    }
    if (height != nullptr) {
        *height = pixelHeight;
    }
}

/**
 * @brief Converts a signed 16-bit controller axis value to the normalized
 * range `[-1.0, 1.0]`.
 *
 * @param value Raw SDL controller axis value.
 * @return (float) Normalized axis value.
 */
inline float atlasNormalizeAxisValue(std::int16_t value) {
    if (value >= 0) {
        return static_cast<float>(value) / 32767.0f;
    }
    return static_cast<float>(value) / 32768.0f;
}

/**
 * @brief Returns the SDL runtime clock in seconds.
 *
 * @return (float) Time elapsed since SDL initialization.
 */
inline float atlasGetTimeSeconds() {
    return static_cast<float>(SDL_GetTicksNS()) / 1'000'000'000.0f;
}

inline double atlasGetTimeSecondsPrecise() {
    return static_cast<double>(SDL_GetTicksNS()) / 1'000'000'000.0;
}

#endif
