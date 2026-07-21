//
// runtime.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Runtime core scene and files
// Copyright (c) 2026 Max Van den Eynde
//

#include "atlas/runtime/context.h"
#include "atlas/window.h"

void RuntimeScene::initialize(Window &window) {
    // Set the properties of the project
    auto runtimeContext = context.lock();
    if (runtimeContext == nullptr) {
        return;
    }

    if (runtimeContext->config.renderer == "deferred") {
        window.useDeferredRendering();

        if (runtimeContext->config.globalIllumination) {
            window.enableGlobalIllumination();
        }
        window.enableSSR(runtimeContext->config.screenSpaceReflections);
        window.setSSRQuality(
            runtimeContext->config.screenSpaceReflectionQuality);
        window.setSSRDebugMode(
            runtimeContext->config.screenSpaceReflectionDebug);
    } else if (runtimeContext->config.renderer == "pathtracing") {
        window.enablePathTracing();
    }

    if (runtimeContext->config.useUpscaling) {
#ifdef METAL
        window.useMetalUpscaling(runtimeContext->config.upscalingRatio);
#endif
    }

    runtimeContext->loadMainScene(window);
}
