#include "atlas/window.h"
#include "atlas/camera.h"
#include "atlas/effect.h"
#include "atlas/particle.h"
#include "atlas/light.h"
#include "atlas/object.h"
#include "atlas/physics.h"
#include "atlas/scene.h"
#include "atlas/texture.h"
#include "atlas/units.h"
#include "atlas/window.h"
#include "atlas/workspace.h"
#include "atlas/component.h"
#include "atlas/audio.h"
#include "aurora/procedural.h"
#include "aurora/terrain.h"
#include "hydra/atmosphere.h"
#include "hydra/fluid.h"
#include <cstdlib>
#include <string_view>
#include <iostream>
#include <memory>

class MainScene : public Scene {
    CoreObject ground;
    CoreObject wallLeft;
    CoreObject wallRight;
    CoreObject wallBack;
    CoreObject cieling;
    CoreObject tallBox;
    CoreObject cubeBox;
    DirectionalLight light;
    Camera camera;
    CoreObject lightObject;
    RenderTarget frameBuffer;
    AreaLight areaLight;

    bool doesUpdate = true;
    bool fall = false;
    int giDebugMode = 0;

  public:
    void update(Window &window) override {
        if (!doesUpdate)
            return;

        camera.update(window);
        if (window.isKeyPressed(Key::Escape)) {
            window.releaseMouse();
            doesUpdate = false;
        } else if (window.isKeyPressed(Key::Q)) {
            fall = !fall;
        } else if (window.isKeyPressed(Key::G) &&
                   window.ddgiSystem != nullptr &&
                   window.ddgiSystem->probeSpace != nullptr) {
            giDebugMode = (giDebugMode + 1) % 4;
            float debugAlpha = 0.0f;
            if (giDebugMode == 1) {
                debugAlpha = 20.0f;
            } else if (giDebugMode == 2) {
                debugAlpha = 30.0f;
            } else if (giDebugMode == 3) {
                debugAlpha = 40.0f;
            }
            window.ddgiSystem->probeSpace->debugColor =
                Color(3.0f, 3.0f, 3.0f, debugAlpha);
        }
        if (fall) {
            camera.position.y -= 10.f * window.getDeltaTime();
        }
    }

    void onMouseMove(Window &window, Movement2d movement) override {
        if (!doesUpdate) {
            return;
        }
        camera.updateLook(window, movement);
    }

    Cubemap createCubemap() {
        Resource right = Workspace::get().createResource(
            "skybox/px.png", "RightSkybox", ResourceType::Image);
        Resource left = Workspace::get().createResource(
            "skybox/nx.png", "LeftSkybox", ResourceType::Image);
        Resource top = Workspace::get().createResource(
            "skybox/py.png", "TopSkybox", ResourceType::Image);
        Resource bottom = Workspace::get().createResource(
            "skybox/ny.png", "BottomSkybox", ResourceType::Image);
        Resource front = Workspace::get().createResource(
            "skybox/pz.png", "FrontSkybox", ResourceType::Image);
        Resource back = Workspace::get().createResource(
            "skybox/nz.png", "BackSkybox", ResourceType::Image);
        ResourceGroup group = Workspace::get().createResourceGroup(
            "Skybox", {right, left, top, bottom, front, back});

        return Cubemap::fromResourceGroup(group);
    }

#define COZY_LIGHT_COLOR Color(1.0f, 0.96f, 0.86f)

    void initialize(Window &window) override {
        Environment env;
        env.fog.intensity = 0.0;
        env.volumetricLighting.enabled = false;
        env.lightBloom.radius = 0.008f;
        env.lightBloom.maxSamples = 5;
        this->setEnvironment(env);

        Workspace::get().setRootPath(std::string(TEST_PATH) + "/resources/");

        camera = Camera();
        camera.setPosition({0.0f, 1.0f, -4.f});
        camera.lookAt({0.0f, 1.0f, 0.2f});
        camera.farClip = 100.0f;
        window.setCamera(&camera);

        areaLight = AreaLight();
        areaLight.position = {0.0f, 1.98f, 0.0f};
        areaLight.setColor(COZY_LIGHT_COLOR);
        areaLight.range = 6.5f;
        areaLight.size = {0.70f, 0.46f};
        areaLight.angle = 125.0f;
        areaLight.castsBothSides = false;
        areaLight.setRotation({90.0f, 0.0f, 0.0f});
        areaLight.castShadows(window, 2048);
        areaLight.intensity = 15;
        this->addAreaLight(&areaLight);
        areaLight.createDebugObject();
        areaLight.addDebugObject(window);

        const Color wallWhite = Color(0.84f, 0.74f, 0.58f);
        const Color boxWhite = Color(0.90f, 0.84f, 0.70f);
        const Color wallRed = Color(0.98f, 0.05f, 0.03f);
        const Color wallGreen = Color(0.04f, 0.95f, 0.07f);

        ground = createBox({2.0f, 0.05f, 2.0f});
        ground.material.albedo = wallWhite;
        ground.material.roughness = 0.96f;
        ground.material.metallic = 0.0f;
        ground.material.ao = 1.0f;
        ground.setPosition({0.0f, -0.025f, 0.0f});
        window.addObject(&ground);

        cieling = createBox({2.0f, 0.05f, 2.0f});
        cieling.material.albedo = wallWhite;
        cieling.material.roughness = 0.95f;
        cieling.material.metallic = 0.0f;
        cieling.material.ao = 1.0f;
        cieling.setPosition({0.0f, 2.025f, 0.0f});
        window.addObject(&cieling);

        wallLeft = createBox({0.05f, 2.0f, 2.0f});
        wallLeft.material.albedo = wallRed;
        wallLeft.material.roughness = 0.94f;
        wallLeft.material.metallic = 0.0f;
        wallLeft.material.ao = 1.0f;
        wallLeft.setPosition({-1.025f, 1.0f, 0.0f});
        window.addObject(&wallLeft);

        wallRight = createBox({0.05f, 2.0f, 2.0f});
        wallRight.material.albedo = wallGreen;
        wallRight.material.roughness = 0.94f;
        wallRight.material.metallic = 0.0f;
        wallRight.material.ao = 1.0f;
        wallRight.setPosition({1.025f, 1.0f, 0.0f});
        window.addObject(&wallRight);

        wallBack = createBox({2.0f, 2.0f, 0.05f});
        wallBack.material.albedo = wallWhite;
        wallBack.material.roughness = 0.94f;
        wallBack.material.metallic = 0.0f;
        wallBack.material.ao = 1.0f;
        wallBack.setPosition({0.0f, 1.0f, 1.025f});
        window.addObject(&wallBack);

        tallBox = createBox({0.56f, 1.2f, 0.56f});
        tallBox.material.albedo = boxWhite;
        tallBox.material.roughness = 0.78f;
        tallBox.material.metallic = 0.0f;
        tallBox.material.ao = 1.0f;
        tallBox.setPosition({-0.42f, 0.6f, 0.5f});
        tallBox.setRotation({0.0f, -16.0f, 0.0f});
        window.addObject(&tallBox);

        cubeBox = createBox({0.6f, 0.6f, 0.6f});
        cubeBox.material.albedo = boxWhite;
        cubeBox.material.roughness = 0.0f;
        cubeBox.material.metallic = 0.0f;
        cubeBox.material.ao = 1.0f;
        cubeBox.setPosition({0.42f, 0.3f, -0.5f});
        cubeBox.setRotation({0.0f, 11.0f, 0.0f});
        window.addObject(&cubeBox);

        this->setAmbientIntensity(0.10f);

        frameBuffer = RenderTarget(window, RenderTargetType::Multisampled);
        window.addRenderTarget(&frameBuffer);
        frameBuffer.display(window);

        this->setUseAtmosphereSkybox(false);

        const char *rendererSetting = std::getenv("ATLAS_CORNELL_RENDERER");
        const std::string_view renderer =
            rendererSetting ? rendererSetting : "path";
        if (renderer == "ddgi") {
            window.enableGlobalIllumination();
            return;
        }
        if (renderer == "deferred") {
            return;
        }
        if (renderer != "path") {
            std::cerr
                << "ATLAS_CORNELL_RENDERER must be path, ddgi, or deferred\n";
            return;
        }
        window.enablePathTracing();

        if (!window.usePathTracing || !window.pathTracer) {
            std::cerr << window.getPathTracingError() << '\n';
            return;
        }
        // window.pathTracer->pathTracingTexture->display(window);
        window.pathTracer->raysPerPixel = 1;
    }
};

int main() {
    Window window({.title = "My Window",
                   .width = 1600,
                   .height = 1200,
                   .renderScale = 1.0f,
                   .mouseCaptured = true,
                   .multisampling = false,
                   .ssaoScale = 0.4f});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
