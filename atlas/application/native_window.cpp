#include "atlas/core/native_window.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

bool atlasQueryNativeWindowPixelSize(void *nativeWindow, int *width,
                                     int *height) {
#ifdef _WIN32
    RECT client{};
    if (nativeWindow == nullptr ||
        !GetClientRect(static_cast<HWND>(nativeWindow), &client)) {
        return false;
    }
    if (width != nullptr) {
        *width = client.right - client.left;
    }
    if (height != nullptr) {
        *height = client.bottom - client.top;
    }
    return true;
#else
    (void)nativeWindow;
    (void)width;
    (void)height;
    return false;
#endif
}
