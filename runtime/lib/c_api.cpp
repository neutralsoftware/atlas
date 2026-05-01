#include "atlas/runtime/c_api.h"

#include "atlas/runtime/context.h"

#include <exception>
#include <memory>
#include <string>

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

bool atlas_runtime_set_editor_controls_enabled(void *runtimeContext,
                                               bool enabled) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->setEditorControlsEnabled(enabled);
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

bool atlas_runtime_set_editor_simulation_enabled(void *runtimeContext,
                                                 bool enabled) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->setEditorSimulationEnabled(enabled);
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

bool atlas_runtime_set_editor_control_mode(void *runtimeContext, int mode) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->setEditorControlMode(mode);
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

bool atlas_runtime_editor_pointer_event(void *runtimeContext, int action,
                                        float x, float y, int button,
                                        float scale) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->editorPointerEvent(action, x, y, button, scale);
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

bool atlas_runtime_editor_scroll_event(void *runtimeContext, float delta,
                                       float scale) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->editorScrollEvent(delta, scale);
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

bool atlas_runtime_editor_key_event(void *runtimeContext, int key,
                                    bool pressed) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->editorKeyEvent(key, pressed);
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

int atlas_runtime_get_selected_object_id(void *runtimeContext) {
    if (runtimeContext == nullptr) {
        return -1;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return -1;
        }
        return (*handle)->selectedObjectId();
    } catch (const std::exception &) {
        return -1;
    } catch (...) {
        return -1;
    }
}

const char *atlas_runtime_get_selected_object_name(void *runtimeContext) {
    static thread_local std::string selectedName;
    selectedName.clear();
    if (runtimeContext == nullptr) {
        return selectedName.c_str();
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return selectedName.c_str();
        }
        selectedName = (*handle)->selectedObjectName();
        return selectedName.c_str();
    } catch (const std::exception &) {
        return selectedName.c_str();
    } catch (...) {
        return selectedName.c_str();
    }
}

const char *atlas_runtime_get_scene_objects(void *runtimeContext) {
    static thread_local std::string sceneObjects;
    sceneObjects = "{\"name\":\"Scene\",\"objects\":[],\"selectedId\":-1}";
    if (runtimeContext == nullptr) {
        return sceneObjects.c_str();
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return sceneObjects.c_str();
        }
        sceneObjects = (*handle)->sceneObjectsJson();
        return sceneObjects.c_str();
    } catch (const std::exception &) {
        return sceneObjects.c_str();
    } catch (...) {
        return sceneObjects.c_str();
    }
}

bool atlas_runtime_select_object(void *runtimeContext, int id,
                                 bool focusCamera) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->selectObject(id, focusCamera);
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

bool atlas_runtime_rename_object(void *runtimeContext, int id,
                                 const char *name) {
    if (runtimeContext == nullptr || name == nullptr || name[0] == '\0') {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->renameObject(id, name);
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

bool atlas_runtime_set_object_parent(void *runtimeContext, int childId,
                                     int parentId) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->setObjectParent(childId, parentId);
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

int atlas_runtime_create_object(void *runtimeContext, const char *type,
                                const char *name) {
    if (runtimeContext == nullptr || type == nullptr) {
        return -1;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return -1;
        }
        return (*handle)->createObject(type, name != nullptr ? name : "");
    } catch (const std::exception &) {
        return -1;
    } catch (...) {
        return -1;
    }
}

bool atlas_runtime_save_current_scene(void *runtimeContext) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->saveCurrentScene();
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
