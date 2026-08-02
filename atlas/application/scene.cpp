//
// scene.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Scene implementation for the Atlas Engine
// Copyright (c) 2025 Max Van den Eynde
//

#include "atlas/scene.h"
#include "atlas/texture.h"
#include "atlas/window.h"
#include <algorithm>
#include <array>

void Scene::clearSkyboxes(Window &window) {
    if (userSkybox != nullptr) {
        window.removePreludeObject(userSkybox.get());
    }
    if (atmosphereSkybox != nullptr && atmosphereSkybox != userSkybox) {
        window.removePreludeObject(atmosphereSkybox.get());
    }
    skybox.reset();
    userSkybox.reset();
    atmosphereSkybox.reset();
    useAtmosphereSkybox = false;
}

void Scene::updateScene(float dt) {
    atmosphere.update(dt);

    if (atmosphere.isEnabled() && useAtmosphereSkybox) {
        constexpr int dynamicSkyResolution = 128;

        if (getAtmosphereSkybox() == nullptr ||
            getAtmosphereSkybox()->cubemap.creationData.width !=
                dynamicSkyResolution) {
            Cubemap dynamicCubemap =
                atmosphere.createSkyCubemap(dynamicSkyResolution);
            setAtmosphereSkybox(
                Skybox::create(dynamicCubemap, *(Window::mainWindow)));
        } else {
            atmosphere.updateSkyCubemap(getAtmosphereSkybox()->cubemap);
        }

        if (automaticAmbient) {
            updateAutomaticAmbientFromSkybox();
            float lightIntensity =
                std::clamp(atmosphere.getLightIntensity(), 0.02f, 1.0f);
            automaticAmbientIntensity = lightIntensity * 0.25f;
        }

        if (!isUsingAtmosphereSkybox() && this->skybox == nullptr) {
            static const std::array<Color, 6> noonSkyColors = {
                Color::fromHex(0x7FC1FF), Color::fromHex(0x89CBFF),
                Color::fromHex(0x2F62D5), Color::fromHex(0xF6E9D2),
                Color::fromHex(0x85CCFF), Color::fromHex(0x80C6FF)};

            Cubemap defaultCubemap = Cubemap::fromColors(noonSkyColors, 1024);
            this->setSkybox(
                Skybox::create(defaultCubemap, *(Window::mainWindow)));
        }
    } else {
    }
}
