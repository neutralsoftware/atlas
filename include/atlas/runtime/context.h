//
// context.h
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Context for the definition of the runtime
// Copyright (c) 2026 Max Van den Eynde
//

#ifndef RUNTIME_CONTEXT_H
#define RUNTIME_CONTEXT_H

#include "atlas/camera.h"
#include "atlas/core/renderable.h"
#include "atlas/scene.h"
#include "atlas/runtime/scripting.h"
#include "atlas/texture.h"
#include "quickjs.h"
#include <atlas/window.h>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <json.hpp>

using json = nlohmann::json;

class Context;

class RuntimeScene : public Scene {
  public:
    std::shared_ptr<Context> context;

    void update(Window &window) override;
    void initialize(Window &window) override;
    void onMouseMove(Window &window, Movement2d movement) override;
    void onMouseScroll(Window &window, Movement2d offset) override;
};

class ProjectConfig {
  public:
    std::string renderer;
    bool globalIllumination;
    std::string mainScene;
    bool useUpscaling = false;
    std::vector<std::string> assetDirectories;
};

#define RUNTIME_LOG(msg)                                                       \
    std::cout << "\033[1;35m[RUINTIME LOG]: \033[0m" << (msg) << std::endl;

class Context {
  public:
    Context() = default;
    std::string projectFile;
    std::string projectDir;
    std::string sceneDir;
    std::string currentSceneFile;
    std::string currentSceneName;
    std::shared_ptr<RuntimeScene> scene;

    JSRuntime *runtime = nullptr;
    JSContext *context = nullptr;
    ScriptHost scriptHost;
    std::unordered_map<std::string, std::string> scriptRegistry;
    std::unordered_map<std::string, std::string> loadedScriptModules;
    std::string scriptBundleModuleName = "__atlas_scripts__";

    std::unique_ptr<Camera> camera;
    std::map<std::string, std::unique_ptr<RenderTarget>> renderTargets;
    std::vector<std::unique_ptr<DirectionalLight>> directionalLights;
    std::vector<std::unique_ptr<Light>> pointLights;
    std::vector<std::unique_ptr<Spotlight>> spotlights;
    std::vector<std::unique_ptr<AreaLight>> areaLights;
    std::vector<std::string> cameraActions;
    bool cameraAutomaticMoving = false;

    std::unique_ptr<Window> window;
    std::vector<std::shared_ptr<Renderable>> objects;
    std::unordered_map<std::string, GameObject *> objectReferences;
    std::unordered_map<int, std::string> objectNames;
    std::unordered_map<int, std::string> objectSceneReferences;

    ProjectConfig config;

    void runWindowed();
    bool stepFrame();
    bool resize(int width, int height, float scale);
    bool setEditorControlsEnabled(bool enabled);
    bool setEditorSimulationEnabled(bool enabled);
    bool setEditorControlMode(int mode);
    bool editorPointerEvent(int action, float x, float y, int button,
                            float scale);
    bool editorScrollEvent(float delta, float scale);
    bool editorKeyEvent(int key, bool pressed);
    int selectedObjectId() const;
    std::string selectedObjectName() const;
    bool saveCurrentScene();
    void end();
    void loadProject();
    void loadMainScene(Window &window);
    void loadScene(Window &window, const json &sceneData);
    void initializeScripting();
    std::string registerScriptModule(const std::string &modulePath);
    std::string toProjectScriptPath(const std::string &path) const;
};

namespace runtime {
std::shared_ptr<Context> makeContext(std::string projectFile);
std::shared_ptr<Context>
makeContextForMetalView(std::string projectFile, void *metalView,
                        CoreWindowReference sdlInputWindow = nullptr);
void runProjectInMetalView(std::string projectFile, void *metalView,
                           CoreWindowReference sdlInputWindow = nullptr);
std::shared_ptr<Context> makeContextForMetalViewNonBlocking(
    std::string projectFile, void *metalView,
    CoreWindowReference sdlInputWindow = nullptr);

namespace scripting {
void dumpExecution(JSContext *ctx);
bool checkNotException(JSContext *ctx, JSValueConst value, const char *what);
}; // namespace scripting
}; // namespace runtime

#endif // RUNTIME_CONTEXT_H
