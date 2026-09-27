#ifndef ATLAS_RUNTIME_WINDOW_ACTIVATION_SCOPE_H
#define ATLAS_RUNTIME_WINDOW_ACTIVATION_SCOPE_H

#include "atlas/window.h"

class WindowActivationScope {
  public:
    explicit WindowActivationScope(Window &window)
        : previousWindow(Window::mainWindow),
          previousDevice(opal::Device::globalInstance) {
        window.activateRenderingContext();
    }

    ~WindowActivationScope() {
        if (previousWindow != nullptr) {
            previousWindow->activateRenderingContext();
            return;
        }
        Window::mainWindow = nullptr;
        opal::Device::globalInstance = previousDevice;
    }

  private:
    Window *previousWindow;
    opal::Device *previousDevice;
};

#endif
