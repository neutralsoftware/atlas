#include "atlas/runtime/c_api.h"

#include "atlas/runtime/context.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <filesystem>
#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#endif
#include <memory>
#include <string>

namespace {
using RuntimeContextHandle = std::shared_ptr<Context>;
#ifdef _WIN32
HANDLE runtimeCrashLog = INVALID_HANDLE_VALUE;
std::wstring runtimeDumpPath;
decltype(&MiniDumpWriteDump) runtimeWriteDump = nullptr;

LONG WINAPI runtimeCrashHandler(EXCEPTION_POINTERS *exception) {
    HMODULE module = nullptr;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(exception->ExceptionRecord->ExceptionAddress),
        &module);
    wchar_t modulePath[MAX_PATH]{};
    GetModuleFileNameW(module, modulePath, MAX_PATH);
    const auto offset = reinterpret_cast<ULONG_PTR>(
                            exception->ExceptionRecord->ExceptionAddress) -
                        reinterpret_cast<ULONG_PTR>(module);
    char message[2048]{};
    const int length = std::snprintf(
        message, sizeof(message),
        "Atlas runtime native crash: 0x%08lX\nModule: %ls\nOffset: 0x%llX\n",
        exception->ExceptionRecord->ExceptionCode, modulePath,
        static_cast<unsigned long long>(offset));
    if (runtimeCrashLog != INVALID_HANDLE_VALUE && length > 0) {
        DWORD written = 0;
        WriteFile(runtimeCrashLog, message,
                  static_cast<DWORD>(std::min<size_t>(length, sizeof(message) - 1)),
                  &written, nullptr);
        FlushFileBuffers(runtimeCrashLog);
    }
    if (runtimeWriteDump != nullptr) {
        HANDLE dump = CreateFileW(runtimeDumpPath.c_str(), GENERIC_WRITE,
                                  FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, nullptr);
        if (dump != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(), exception,
                                                FALSE};
            runtimeWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dump,
                             MiniDumpNormal, &info, nullptr, nullptr);
            CloseHandle(dump);
        }
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

class RuntimeDiagnostics {
  public:
    explicit RuntimeDiagnostics(const char *projectFile) {
        const auto directory = std::filesystem::u8path(projectFile).parent_path() /
                               ".atlas" / "logs";
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        runtimeDumpPath = (directory / "runtime-crash.dmp").wstring();
        runtimeCrashLog = CreateFileW((directory / "runtime-crash.log").c_str(),
                                      GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        debugHelp = LoadLibraryExW(L"dbghelp.dll", nullptr,
                                   LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (debugHelp != nullptr) {
            auto entry = GetProcAddress(debugHelp, "MiniDumpWriteDump");
            static_assert(sizeof(runtimeWriteDump) == sizeof(entry));
            std::memcpy(&runtimeWriteDump, &entry, sizeof(entry));
        }
        previousHandler = SetUnhandledExceptionFilter(runtimeCrashHandler);
    }

    ~RuntimeDiagnostics() {
        SetUnhandledExceptionFilter(previousHandler);
        if (runtimeCrashLog != INVALID_HANDLE_VALUE)
            CloseHandle(runtimeCrashLog);
        runtimeCrashLog = INVALID_HANDLE_VALUE;
        runtimeWriteDump = nullptr;
        if (debugHelp != nullptr)
            FreeLibrary(debugHelp);
    }

  private:
    HMODULE debugHelp = nullptr;
    LPTOP_LEVEL_EXCEPTION_FILTER previousHandler = nullptr;
};
#endif

}

bool atlas_runtime_run_project(const char *projectFile) {
    if (projectFile == nullptr || projectFile[0] == '\0') {
        return false;
    }

    try {
#ifdef _WIN32
        RuntimeDiagnostics diagnostics(projectFile);
#endif
        std::cerr << "Atlas runtime build revision: " << ATLAS_BUILD_REVISION
                  << std::endl;
        auto context = runtime::makeContext(projectFile);
        context->loadProject();
        context->runWindowed();
        return true;
    } catch (const std::exception &error) {
        std::cerr << "Atlas runtime failed: " << error.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "Atlas runtime failed with an unknown exception" << std::endl;
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

bool atlas_runtime_set_object_property(void *runtimeContext, int id,
                                       const char *component,
                                       int componentIndex,
                                       const char *propertyPath,
                                       const char *jsonValue) {
    if (runtimeContext == nullptr || component == nullptr ||
        propertyPath == nullptr || jsonValue == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->setObjectProperty(
            id, component, componentIndex, propertyPath,
            nlohmann::json::parse(jsonValue));
    } catch (const std::exception &) {
        return false;
    } catch (...) {
        return false;
    }
}

int atlas_runtime_add_object_component(void *runtimeContext, int id,
                                       const char *jsonComponent) {
    if (runtimeContext == nullptr || jsonComponent == nullptr) {
        return -1;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return -1;
        }
        return (*handle)->addObjectComponent(
            id, nlohmann::json::parse(jsonComponent));
    } catch (const std::exception &) {
        return -1;
    } catch (...) {
        return -1;
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

bool atlas_runtime_delete_object(void *runtimeContext, int id) {
    if (runtimeContext == nullptr) {
        return false;
    }
    try {
        auto *handle = reinterpret_cast<RuntimeContextHandle *>(runtimeContext);
        if (*handle == nullptr) {
            return false;
        }
        return (*handle)->deleteObject(id);
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
