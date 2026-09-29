/*
 * native_window.h
 * As part of the Atlas project
 * --------------------------------------
 * Description: Queries on host-owned native windows used for embedding
 */

#ifndef ATLAS_NATIVE_WINDOW_H
#define ATLAS_NATIVE_WINDOW_H

/**
 * @brief Returns the drawable size, in physical pixels, of a native window
 * handle (an HWND on Windows).
 *
 * This matches the extent the presentation surface reports, which SDL's cached
 * size for a foreign window does not always do.
 *
 * @return (bool) False when the size cannot be queried on this platform.
 */
bool atlasQueryNativeWindowPixelSize(void *nativeWindow, int *width,
                                     int *height);

#endif // ATLAS_NATIVE_WINDOW_H
