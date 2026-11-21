#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/window.h"

class MainScene : public Scene {
    Model humanoid;
    Camera camera;
    RenderTarget frameBuffer;
    DirectionalLight light;
    bool doesUpdate = true;

  public:
    void update(Window &window) override {
        if (!doesUpdate)
            return;

        camera.update(window);
        if (window.isKeyPressed(Key::Escape)) {
            window.releaseMouse();
            doesUpdate = false;
        }
    }

    void onMouseMove(Window &window, Movement2d movement) override {
        if (!doesUpdate) {
            return;
        }
        camera.updateLook(window, movement);
    }

    void initialize(Window &window) override {
        camera = Camera();
        camera.setPosition({0.0f, 1.0f, -4.f});
        camera.lookAt({0.0f, 1.0f, 0.2f});
        camera.farClip = 100.0f;
        window.setCamera(&camera);

        light = DirectionalLight({0.0f, -1.0f, 0.0f});
        addDirectionalLight(&light);

        Environment env;
        env.fog.intensity = 0.0;
        env.volumetricLighting.enabled = false;
        env.lightBloom.radius = 0.008f;
        env.lightBloom.maxSamples = 4;
        this->setEnvironment(env);

        this->setAmbientIntensity(0.10f);

        frameBuffer = RenderTarget(window, RenderTargetType::Multisampled);
        window.addRenderTarget(&frameBuffer);
        frameBuffer.display(window);

        this->setUseAtmosphereSkybox(false);

        Workspace::get().setRootPath(TEST_PATH);
        Resource humanoidResource = Workspace::get().createResource(
            "resources/walkXBot.fbx", "Humanoid", ResourceType::Model);

        humanoid = Model();
        humanoid.fromResource(humanoidResource, true);
        auto &animator = humanoid.getAnimator();
        for (const auto &clip : animator.getClips()) {
            std::cout << clip.name << std::endl;
        }
        animator.play("mixamo.com", true, 1.0f);
        humanoid.setScale({0.01f, 0.01f, 0.01f});
        window.addObject(&humanoid);
    }
};

int main() {
    WindowConfiguration settings;
    settings.title = "Atlas Test";
    settings.width = 1200;
    settings.height = 800;
    settings.mouseCaptured = true;

    Window window(settings);
    MainScene scene;
    window.setScene(&scene);
    window.useDeferredRendering();
    window.run();
}