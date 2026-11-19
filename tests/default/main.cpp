#include "atlas/object.h"
#include "atlas/scene.h"
#include "atlas/window.h"

class MainScene : public Scene {
    CoreObject cube;

  public:
    void initialize(Window &window) override {
        window.useDeferredRendering();
        cube = createBox({1.0f, 1.0f, 1.0f});
        window.addObject(&cube);
    }
};

int main() {
    WindowConfiguration settings;
    settings.title = "Atlas Test";
    settings.width = 800;
    settings.height = 600;
    settings.mouseCaptured = false;

    Window window(settings);
    MainScene scene;
    window.setScene(&scene);
    window.useDeferredRendering();
    window.run();
}