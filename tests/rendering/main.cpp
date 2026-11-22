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
#include <array>
#include <cstdlib>
#include <string_view>
#include <iostream>
#include <memory>

class MainScene : public Scene {
    Camera camera;
    CoreObject lightObject;
    RenderTarget frameBuffer;
    AreaLight areaLight;
    DirectionalLight light;
    std::array<Light, 8> candleLights;

    Model model;
    Model curtains;
    Model candles;

    bool doesUpdate = true;
    bool fall = false;
    int giDebugMode = 0;

    bool night = true;

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
        } else if (window.isKeyActive(Key::C)) {
            std::cout << "Camera Position: " << camera.position.x << ", "
                      << camera.position.y << ", " << camera.position.z
                      << std::endl;
        } else if (window.isKeyActive(Key::L)) {
            Position3d lookDir = camera.getFrontVector();
            std::cout << "Camera Look Direction: " << lookDir.x << ", "
                      << lookDir.y << ", " << lookDir.z << std::endl;
        }
        if (fall) {
            camera.position.y -= 10.f * window.getDeltaTime();
        }
    }

    void onMouseMove(Window &window, Movement2d movement) override {
        if (!doesUpdate) {
            return;
        }
        // camera.updateLook(window, movement);
    }

    void initialize(Window &window) override {
        Environment env;
        env.fog.intensity = 0.0;
        env.volumetricLighting.enabled = false;
        env.lightBloom.radius = 0.008f;
        env.lightBloom.maxSamples = 5;
        this->setEnvironment(env);

        atmosphere.enable();
        atmosphere.cycle = false;
        if (night) {
            atmosphere.setTime(0.0);
        } else {
            atmosphere.setTime(13.0);
        }

        Workspace::get().setRootPath(std::string(TEST_PATH) +
                                     "/resources/private/");

        camera = Camera();
        camera.setPosition({-10, 2.0, 0.});
        camera.lookAt({15.0, 11.0, 0.0});
        camera.farClip = 100.0f;
        window.setCamera(&camera);

        this->setAmbientIntensity(0.10f);

        frameBuffer = RenderTarget(window, RenderTargetType::Scene);
        window.addRenderTarget(&frameBuffer);
        frameBuffer.display(window);

        areaLight = AreaLight();
        areaLight.position = {0.0f, 2.5f, 0.0f};
        areaLight.size = {.width = 100.0f, .height = 10.0f};
        areaLight.rotate({0.0f, -90.0f, 0.0f});
        // addAreaLight(&areaLight);

        light = DirectionalLight();
        light.direction = {-0.5f, -1.0f, -0.2f};
        if (!night) {
            light.intensity = 100.5f;
            light.color = Color(1.0f, 0.88f, 0.72f);
        } else {
            light.intensity = 0.5f;
            light.color = Color(0.2f, 0.2f, 0.4f);
        }
        addDirectionalLight(&light);

        model = Model();
        model.fromResource(Workspace::get().createResource(
            "main_sponza/main_sponza/NewSponza_Main_glTF_003.gltf",
            "Monk Model", ResourceType::Model));

        curtains = Model();
        curtains.fromResource(Workspace::get().createResource(
            "pkg_a_curtains/pkg_a_curtains/NewSponza_Curtains_glTF.gltf",
            "Curtains Model", ResourceType::Model));

        if (night) {
            candles = Model();
            candles.fromResource(Workspace::get().createResource(
                "pkg_d_10k_candles/pkg_d_10k_candles/"
                "NewSponza_4_Combined_glTF.gltf",
                "Candles Model", ResourceType::Model));
            for (const auto &mesh : candles.getObjects()) {
                if (mesh && mesh->material.emissiveIntensity > 0.0f) {
                    mesh->material.emissiveColor = Color(1.0f, 0.72f, 0.14f);
                    mesh->material.emissiveIntensity *= 6.0f;
                }
            }
            window.addObject(&candles);

            for (size_t i = 0; i < candleLights.size(); ++i) {
                auto &candleLight = candleLights[i];
                const float x = -12.0f + 8.0f * static_cast<float>(i % 4);
                const float y = i < 4 ? 3.0f : 8.0f;
                candleLight.position = {x, y, 0.0f};
                candleLight.color = Color(1.0f, 0.56f, 0.10f);
                candleLight.shineColor = candleLight.color;
                candleLight.intensity = 20.0f;
                candleLight.distance = 12.0f;
                addLight(&candleLight);
            }
        }

        window.addObject(&model);
        window.addObject(&curtains);

        this->setUseAtmosphereSkybox(true);

        const char *rendererSetting = std::getenv("ATLAS_TEST_RENDERER");
        const std::string_view renderer =
            rendererSetting ? rendererSetting : "deferred";
        if (renderer == "ddgi") {
            window.enableGlobalIllumination();
            return;
        }
        if (renderer == "deferred") {
            window.useDeferredRendering();
            return;
        }
        if (renderer != "path") {
            std::cerr
                << "ATLAS_TEST_RENDERER must be path, ddgi, or deferred\n";
            return;
        }
        window.enablePathTracing();
        window.configurePathTracing(2048, 32, false, 1,
                                    photon::AllPathTracingFeatures, true);

        if (!window.usePathTracing || !window.pathTracer) {
            std::cerr << window.getPathTracingError() << '\n';
            return;
        }
        // window.pathTracer->pathTracingTexture->display(window);
    }
};

int main() {
    Window window({.title = "My Window",
                   .width = 1600,
                   .height = 1200,
                   .renderScale = 0.8f,
                   .mouseCaptured = true,
                   .multisampling = false,
                   .ssaoScale = 0.4f});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
