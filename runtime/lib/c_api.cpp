#include "atlas/runtime/c_api.h"

#include "atlas/runtime/context.h"

#include <exception>
#include <memory>

namespace {
using RuntimeContextHandle = std::shared_ptr<Context>;
}

bool atlas_runtime_run_project(const char *projectFile) {
    if (projectFile == nullptr || projectFile[0] == '\0') {
        return false;
    }

    try {
        auto context = runtime::makeContext(projectFile);
        context->loadProject();
        context->runWindowed();
        return true;
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

bool atlas_runtime_run_in_metal_view(const char *projectFile, void *metalView,
                                     void *sdlInputWindow) {
    if (projectFile == nullptr || projectFile[0] == '\0') {
        return false;
    }

    try {
        runtime::runProjectInMetalView(
            projectFile, metalView,
            reinterpret_cast<CoreWindowReference>(sdlInputWindow));
        return true;
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

void *atlas_runtime_create_metal_view_context(const char *projectFile,
                                              void *metalView,
                                              void *sdlInputWindow) {
    if (projectFile == nullptr || projectFile[0] == '\0') {
        return nullptr;
    }

    try {
        auto context = runtime::makeContextForMetalViewNonBlocking(
            projectFile, metalView,
            reinterpret_cast<CoreWindowReference>(sdlInputWindow));
        auto *handle = new RuntimeContextHandle(std::move(context));
        return reinterpret_cast<void *>(handle);
    } catch (const std::exception &) {
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

bool atlas_runtime_step_frame(void *runtimeContext) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->stepFrame();
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

bool atlas_runtime_resize_context(void *runtimeContext, int width, int height,
                                  float scale) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->resize(width, height, scale);
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

void atlas_runtime_end_context(void *runtimeContext) {
    if (runtimeContext == nullptr) {
        return;
    }
    auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
    if (*handle == nullptr) {
        return;
    }
    try {
        (*handle)->end();
    } catch (...) {
    }
}

void atlas_runtime_destroy_context(void *runtimeContext) {
    if (runtimeContext == nullptr) {
        return;
    }
    auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
    delete handle;
}
