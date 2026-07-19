# Source and function index

This is the fast lookup page. It lists the architecturally significant functions and groups repetitive helpers by file. Use `rg -n "FunctionName" <path>` when a referenced line moves.

## Startup and lifetime

| Question | Function/source |
|---|---|
| Where does the native runtime start? | `main()` — `runtime/executable/main.cpp:13` |
| Where does the editor start? | `main()` — `editor/main.cpp:32` |
| Where does the CLI dispatch? | `main()` — `cli/src/main.rs:10` |
| How is a Context/Window constructed? | `makeContextWithWindowOptions()` — `runtime/lib/context.cpp:4008` |
| How is a project manifest loaded? | `Context::loadProject()` — `runtime/lib/context.cpp:5752` |
| Where is the main scene loaded? | `Context::loadMainScene()` — `runtime/lib/context.cpp:5876` |
| Where is the window initialized? | `Window::Window()` — `atlas/application/window.cpp:929` |
| Where is lazy frame-loop state initialized? | `Window::initializeRunLoop()` — `atlas/application/window.cpp:1197` |
| Where is shutdown? | `Context::end()`/destructor — `runtime/lib/context.cpp:5715,5723`; `Window::~Window()` — `atlas/application/window.cpp:3859` |

## Frame, scene, and objects

| Question | Function/source |
|---|---|
| What happens each frame? | `Window::stepFrame()` — `atlas/application/window.cpp:1405` |
| Where is the blocking loop? | `Window::run()` — `atlas/application/window.cpp:3427` |
| How are SDL events handled? | `Window::pollEvents()` — `atlas/application/window.cpp:1257` |
| How are scenes swapped? | `Window::setScene()` / `applyScene()` — `atlas/application/window.cpp:3741,3671` |
| How are renderables registered? | `addObject/addInitializedObject` — `atlas/application/window.cpp:3433,3463` |
| How are renderables removed? | `removeObjectInternal()` — `atlas/application/window.cpp:3500` |
| How is the environment updated? | `Scene::updateScene()` — `atlas/application/scene.cpp:16` |
| How is scene JSON interpreted? | `Context::loadScene()` — `runtime/lib/context.cpp:5891` |
| How is a mesh initialized/rendered? | `CoreObject::initialize/render` — `atlas/object/core_object.cpp:384,520` |
| How are imported models built? | `Model::loadModel/processNode/processMesh` — `atlas/object/model.cpp:149,209,227` |
| How are compound children forwarded? | `CompoundObject::initialize/render/update` — `atlas/object/compound.cpp:120,138,246` |

## Rendering

| Area | Source anchors |
|---|---|
| Frame render-mode routing | `atlas/application/window.cpp:1574-1954` |
| Deferred renderer | `Window::deferredRendering()` — `atlas/graphics/deferred.cpp:263` |
| Deferred enable/GI | `atlas/application/window.cpp:4689`; `atlas/graphics/deferred.cpp:255` |
| SSAO | `atlas/graphics/ssao.cpp:21,79` |
| Shadows | `Window::renderLightsToShadowMaps()` — `atlas/application/window.cpp:4020`; `atlas/graphics/light.cpp` |
| Bloom | `atlas/graphics/bloom.cpp:21-302`; `Window::renderPhysicalBloom()` — `atlas/application/window.cpp:4705` |
| Render targets | `atlas/graphics/render_target.cpp:469-650` |
| Textures/cubemaps/skybox | `atlas/graphics/texture_create.cpp`; `atlas/graphics/texture.cpp:387-744` |
| Particles | `atlas/graphics/particle.cpp:42-501` |
| Fluids | `hydra/fluid.cpp:35-431`; `Window::updateFluidCaptures()` — `atlas/application/window.cpp:4754` |
| GI/path tracing | `photon/gi.cpp:226`; `photon/path_tracing.cpp:154`; activation at `atlas/application/window.cpp:5447` |
| Opal context/device | `opal/device.cpp` |
| Opal commands | `opal/command_buffer.cpp:1125-2277` |
| Opal pipeline | `opal/pipeline.cpp:353-1708` |
| Opal framebuffer/pass | `opal/framebuffer.cpp:107-802` |
| Opal buffers/textures/shaders | `opal/buffer.cpp`, `opal/texture.cpp`, `opal/shaders.cpp` |

## Runtime/editor API

| Area | Source anchors |
|---|---|
| C ABI contract | `include/atlas/runtime/c_api.h:12-91` |
| C ABI implementation | `runtime/lib/c_api.cpp:12-436` |
| Context editor methods | `include/atlas/runtime/context.h:111-160`; `runtime/lib/context.cpp:4185-5721` |
| Scene snapshot | `Context::sceneObjectsJson()` — `runtime/lib/context.cpp:4733` |
| Selection and focus | `Context::selectObject/focusObjects` — `runtime/lib/context.cpp:4768,4784` |
| Object mutation | `runtime/lib/context.cpp:4798-5621` |
| Scene save/open | `Context::saveCurrentScene/openSceneFile` — `runtime/lib/context.cpp:5622,5691` |
| Editor viewport start/step/resize | `editor/views/editor/viewport.cpp:572,678,700` |
| Editor snapshot refresh | `ViewportPanel::refreshSceneSnapshot()` — `editor/views/editor/viewport.cpp:1444` |
| Hierarchy snapshot consumer | `editor/views/editor/hierarchy.cpp:267` |
| Inspector snapshot consumer | `editor/views/editor/inspector.cpp:1235` |
| Play/pause/step/stop | `editor/views/editor/viewport.cpp:1209-1290` |
| Docks/layout/close | `editor/views/editor/editor.cpp:586,1714-1815`; `editor/core/dockManager.cpp:19-66` |

## Scripting

| Area | Source anchors |
|---|---|
| QuickJS creation/module setup | `Context::initializeScripting()` — `runtime/lib/context.cpp:4120` |
| Project module registration | `Context::registerScriptModule()` — `runtime/lib/context.cpp:4170` |
| Built-in native registration | `runtime::scripting::installGlobals()` — `runtime/lib/scripting.cpp:15628` |
| Module loader | `runtime/lib/scripting.cpp:16732-16776` |
| Script instance construction/calls | `runtime/lib/scripting.cpp:16808-16874` |
| Interactive frame/mouse dispatch | `runtime/lib/scripting.cpp:15417-15504` |
| Window/scene/camera wrappers | `runtime/lib/scripting.cpp:3538-3944` |
| Object/UI application | `runtime/lib/scripting.cpp:4518-5292` |
| Physics bridge | `runtime/lib/scripting.cpp:5616-7604,11419-11987` |
| Audio bridge | `runtime/lib/scripting.cpp:1368-2244,9644-10500` |
| Scene binding cleanup | `runtime/lib/scripting.cpp:15003` |

## Input, physics, audio, resources

| Area | Source anchors |
|---|---|
| Trigger factories/controllers | `atlas/application/input.cpp:15-161` |
| Named action evaluation | `atlas/application/window.cpp:5104-5320` |
| Runtime camera | `atlas/camera.cpp:54-304` |
| Editor selection/gizmo/camera | `atlas/application/window.cpp:2200-3048` |
| Atlas Rigidbody | `atlas/physics/rigidbody.cpp:91-1110` |
| Physics world | `bezel/jolt/world.cpp:186,227` |
| Contact/query dispatcher | `bezel/jolt/query/rigidbody_query.cpp:136-439` and remainder |
| Atlas/backend joints | `atlas/physics/atlas_joints.cpp`; `bezel/jolt/joints.cpp` |
| Atlas/backend vehicles | `atlas/physics/vehicle.cpp`; `bezel/jolt/vehicle.cpp` |
| Audio engine | `finewave/engine.cpp:46-165` |
| Audio data/source/effects | `finewave/load.cpp:74`; `finewave/source.cpp:79-316`; `finewave/effect.cpp:34-112` |
| AudioPlayer component | `include/atlas/audio.h:43-220` |
| Workspace registry | `atlas/application/workspace.cpp:13-90` |

## CLI and project workflow

| Command/area | Source anchors |
|---|---|
| Command schema/dispatch | `cli/src/lib.rs:13-89`; `cli/src/main.rs:10-30` |
| Create project | `cli/src/create.rs:230`; editor templates at `editor/project/projectStore.cpp:47-138,211-285` |
| Run/dynamic runtime loading | `cli/src/run.rs:188-313` |
| Build/clangd | `cli/src/pack.rs:313-381` |
| Pack | `cli/src/pack.rs:383-559` |
| Script init/compile/new | `cli/src/script.rs:601,696,748,832` |
| Project browser/recent list | `editor/views/general/projectBrowser.cpp:308-537`; `editor/project/projectStore.cpp:148-327` |
| Editor export | `editor/views/editor/editor.cpp:1127`; CLI discovery at `editor/views/editor/editor.cpp:167` |

## File-level map for the remaining implementation

| Directory | What to expect |
|---|---|
| `atlas/object/` | mesh/object/material/shader/shape/model/compound behavior |
| `atlas/graphics/` | high-level render features and renderable graphics types |
| `opal/vulkan/` | Vulkan device/swapchain/buffer/pipeline/framebuffer/texture implementation |
| `bezel/native/shapes/` | native GJK/EPA/convex hull/projection algorithms |
| `graphite/input/` | runtime UI widget behavior |
| `atlas/tracer/data/` | structured diagnostic event serialization |
| `editor/views/editor/` | engine-facing editing panels and viewport tools |
| `editor/views/general/` | project/content/dialog/splash shell UI |

## Keeping this index current

To find C++ definitions after line drift:

```sh
rg -n 'ClassName::functionName' atlas runtime editor opal bezel finewave graphite photon hydra aurora
```

To list likely C++ function definitions in one file:

```sh
rg -n '^[A-Za-z_~][A-Za-z0-9_:<>,*& ]+[[:space:]]+[A-Za-z_~][A-Za-z0-9_:~]*\([^;{}]*\).*[[:space:]]\{' path/to/file.cpp
```

To list Rust functions:

```sh
rg -n '^(pub )?fn ' cli/src
```

The large exceptions are `runtime/lib/context.cpp` and `runtime/lib/scripting.cpp`: use the subsystem tables above first, then search within the relevant region.
