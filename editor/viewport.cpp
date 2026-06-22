#include "editor/viewport.h"

#include "atlas/runtime/context.h"
#include "atlas/texture.h"
#include "atlas/window.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <exception>
#include <imgui.h>
#include <iostream>
#include <stdexcept>

namespace {
    std::string defaultProjectFile() {
#ifdef ATLAS_DEFAULT_PROJECT_FILE
        return ATLAS_DEFAULT_PROJECT_FILE;
#else
        return "tests/simple/project.atlas";
#endif
    }

    ImTextureRef textureRef(MTL::Texture* texture) {
        if (texture == nullptr) {
            return ImTextureRef();
        }
        return ImTextureRef(static_cast<ImTextureID>(
            reinterpret_cast<std::uintptr_t>(texture)));
    }
}

EditorViewport::~EditorViewport() { shutdown(); }

void EditorViewport::initialize(MTL::Device* device) {
    initialize(device, defaultProjectFile(), nullptr);
}

void EditorViewport::initialize(MTL::Device* device,
                                const std::string& projectFile) {
    initialize(device, projectFile, nullptr);
}

void EditorViewport::initialize(MTL::Device* newDevice,
                                const std::string& newProjectFile,
                                SDL_Window* newHostWindow) {
    if (initialized) {
        return;
    }

    (void)newDevice;
    (void)newHostWindow;
    runtimeContext = runtime::makeHiddenContext(newProjectFile);
    runtimeContext->loadProject();
    if (runtimeContext->window != nullptr) {
        runtimeContext->window->setHostWindowVisible(false);
        runtimeContext->window->setDefaultFramebufferRenderingEnabled(false);
    }
    initialized = true;
    running = runtimeContext != nullptr;
}

void EditorViewport::resize(Size2d newSize) {
    pendingSize = {
        .width = std::max(1.0f, std::floor(newSize.width)),
        .height = std::max(1.0f, std::floor(newSize.height)),
    };
}

void EditorViewport::ensureRenderTarget() {
    if (runtimeContext == nullptr || runtimeContext->window == nullptr) {
        return;
    }
    if (pendingSize.width <= 0.0f || pendingSize.height <= 0.0f) {
        return;
    }
    if (renderTarget != nullptr && pendingSize.width == size.width &&
        pendingSize.height == size.height) {
        return;
    }

    if (renderTarget != nullptr) {
        runtimeContext->window->removeRenderTarget(renderTarget);
        renderTarget = nullptr;
    }

    size = pendingSize;
    runtimeContext->resize(static_cast<int>(size.width),
                           static_cast<int>(size.height), 1.0f);
    renderTarget =
        new RenderTarget(*runtimeContext->window,
                         RenderTargetType::Scene);
    runtimeContext->window->addRenderTarget(renderTarget);
    runtimeContext->window->setHostWindowVisible(false);
}

void EditorViewport::renderRuntime() {
    if (!running || runtimeContext == nullptr) {
        return;
    }

    ensureRenderTarget();

    try {
        running = runtimeContext->stepFrame();
        if (runtimeContext->window != nullptr) {
            runtimeContext->window->setHostWindowVisible(false);
        }
    }
    catch (const std::exception& exception) {
        running = false;
        std::cerr << "Runtime viewport stopped: " << exception.what()
            << std::endl;
    }
}

void EditorViewport::drawImGui() {
    if (!initialized) {
        return;
    }

    const bool visible = ImGui::Begin("Viewport");
    ImVec2 available = ImGui::GetContentRegionAvail();
    available.x = std::max(1.0f, available.x);
    available.y = std::max(1.0f, available.y);
    resize({.width = available.x, .height = available.y});

    if (!visible) {
        ImGui::End();
        return;
    }

    ensureRenderTarget();

    MTL::Texture* metalTexture = nullptr;
    if (renderTarget != nullptr && renderTarget->texture.texture != nullptr) {
        metalTexture = renderTarget->texture.texture->getMetalTexture();
    }

    if (metalTexture != nullptr) {
        ImGui::Image(textureRef(metalTexture), available, ImVec2(0.0f, 0.0f),
                     ImVec2(1.0f, 1.0f));
    } else {
        ImGui::InvisibleButton("##AtlasRuntimeViewport", available);
    }

    ImGui::End();
}

void EditorViewport::shutdown() {
    if (runtimeContext != nullptr && runtimeContext->window != nullptr &&
        renderTarget != nullptr) {
        runtimeContext->window->removeRenderTarget(renderTarget);
    }
    renderTarget = nullptr;

    if (runtimeContext != nullptr) {
        runtimeContext->end();
        runtimeContext.reset();
    }

    initialized = false;
    running = false;
}

MTL::Device* EditorViewport::getMetalDevice() const {
    if (runtimeContext == nullptr || runtimeContext->window == nullptr ||
        runtimeContext->window->getDevice() == nullptr) {
        return nullptr;
    }
    return runtimeContext->window->getDevice()->getMetalDevice();
}
