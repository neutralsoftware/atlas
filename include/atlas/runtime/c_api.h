#ifndef ATLAS_RUNTIME_C_API_H
#define ATLAS_RUNTIME_C_API_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

bool atlas_runtime_run_project(const char *projectFile);

/**
 * @brief Runs an Atlas project using a host NSView as the Metal target.
 *
 * This entrypoint is intended for FFI consumers (for example Rust). It is
 * available only when Atlas is built with the Metal backend.
 *
 * @param projectFile Absolute or relative path to the .atlas project file.
 * @param metalView Host NSView pointer used for CAMetalLayer presentation.
 * @param sdlInputWindow Optional SDL_Window* cast to void*. This window can
 * be used to keep SDL input/audio integration active while rendering to
 * `metalView`.
 * @return `true` on success, `false` when startup fails.
 */
bool atlas_runtime_run_in_metal_view(const char *projectFile, void *metalView,
                                     void *sdlInputWindow);

/**
 * @brief Creates a non-blocking runtime context bound to a Metal NSView.
 *
 * Returns an opaque handle that must be passed to step/end/destroy.
 */
void *atlas_runtime_create_metal_view_context(const char *projectFile,
                                              void *metalView,
                                              void *sdlInputWindow);

/**
 * @brief Advances one frame on a non-blocking runtime context.
 *
 * @return `true` while rendering should continue, `false` when ended.
 */
bool atlas_runtime_step_frame(void *runtimeContext);

bool atlas_runtime_resize_context(void *runtimeContext, int width, int height,
                                  float scale);

bool atlas_runtime_set_editor_controls_enabled(void *runtimeContext,
                                               bool enabled);
bool atlas_runtime_set_editor_simulation_enabled(void *runtimeContext,
                                                 bool enabled);
bool atlas_runtime_set_editor_control_mode(void *runtimeContext, int mode);
bool atlas_runtime_editor_pointer_event(void *runtimeContext, int action,
                                        float x, float y, int button,
                                        float scale);
bool atlas_runtime_editor_scroll_event(void *runtimeContext, float delta,
                                       float scale);
bool atlas_runtime_editor_key_event(void *runtimeContext, int key,
                                    bool pressed);
int atlas_runtime_get_selected_object_id(void *runtimeContext);
const char *atlas_runtime_get_selected_object_name(void *runtimeContext);
const char *atlas_runtime_get_scene_objects(void *runtimeContext);
bool atlas_runtime_select_object(void *runtimeContext, int id,
                                 bool focusCamera);
bool atlas_runtime_rename_object(void *runtimeContext, int id,
                                 const char *name);
bool atlas_runtime_set_object_parent(void *runtimeContext, int childId,
                                     int parentId);
int atlas_runtime_create_object(void *runtimeContext, const char *type,
                                const char *name);
bool atlas_runtime_save_current_scene(void *runtimeContext);

/**
 * @brief Requests shutdown and releases frame-loop resources.
 */
void atlas_runtime_end_context(void *runtimeContext);

/**
 * @brief Destroys a context handle created by
 * atlas_runtime_create_metal_view_context.
 */
void atlas_runtime_destroy_context(void *runtimeContext);

#ifdef __cplusplus
}
#endif

#endif
