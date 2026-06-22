/*
* viewport.h
* As part of the Atlas project
* Created by Max Van den Eynde in 2026
* --------------------------------------
* Description: ${FILE_DESCRIPTION}
* Copyright (c) 2026 Max Van den Eynde
*/

#ifndef ATLAS_VIEWPORT_H
#define ATLAS_VIEWPORT_H

#include "atlas/units.h"
#include <Metal/Metal.hpp>
#include <memory>
#include <string>

class Context;
class RenderTarget;
struct SDL_Window;

class EditorViewport {
public:
    ~EditorViewport();

    void initialize(MTL::Device* device);
    void initialize(MTL::Device* device, const std::string& projectFile);
    void initialize(MTL::Device* device, const std::string& projectFile, SDL_Window* hostWindow);
    void resize(Size2d newSize);

    void renderRuntime();
    void drawImGui();
    void shutdown();
    MTL::Device* getMetalDevice() const;

private:
    void ensureRenderTarget();

    std::shared_ptr<Context> runtimeContext;
    RenderTarget* renderTarget;
    Size2d size = {.width = 0.0f, .height = 0.0f};
    Size2d pendingSize = {.width = 0.0f, .height = 0.0f};
    bool initialized = false;
    bool running = false;
};

#endif //ATLAS_VIEWPORT_H
