# Atlas architecture guide

This folder is a source-guided map of Atlas. It explains where each major responsibility lives, how control and data move through the engine, and which functions are the best entry points when changing behavior.

The line references describe this checkout on 2026-07-19. They are deliberately written as `path:line` so they remain searchable even after surrounding code moves. When a line has drifted, search for the named function rather than trusting the old number.

## Suggested reading order

1. [System map](01-system-map.md) — the vocabulary and module boundaries.
2. [Build graph and binaries](02-build-and-binaries.md) — what is compiled and linked into what.
3. [Runtime and frame loop](03-runtime-and-frame-loop.md) — startup, project loading, one frame, and shutdown.
4. [Scene, objects, and components](04-scene-objects-and-components.md) — the engine's object model and ownership rules.
5. [Rendering](05-rendering.md) — the render graph, Opal abstraction, deferred/forward/path-traced paths, and effects.
6. [Scripting and runtime API](06-scripting-and-runtime-api.md) — QuickJS, script components, and the C boundary.
7. [Editor architecture](07-editor.md) — Qt, docks, the embedded Metal runtime, inspector/hierarchy synchronization, and play mode.
8. [Input and camera](08-input-and-camera.md) — SDL input, actions, editor input translation, and camera movement.
9. [Physics](09-physics.md) — Atlas components, the Bezel boundary, Jolt, queries, collisions, joints, and vehicles.
10. [Audio and resources](10-audio-and-resources.md) — Finewave/OpenAL and workspace resources.
11. [CLI, projects, and packaging](11-cli-projects-and-packaging.md) — manifests, scene files, dynamic runtime loading, builds, and export.
12. [Specialized systems](12-specialized-systems.md) — Graphite, Photon, Hydra, Aurora, and tracing.
13. [Source and function index](13-source-and-function-index.md) — a practical “where does this live?” lookup table.

## The three most important distinctions

- **Atlas library vs Atlas runtime:** `atlas/` and `include/atlas/` implement the engine. `runtime/lib/` turns that engine into a project loader, scripting host, editor-facing API, and shared library.
- **Engine window vs editor window:** `Window` is the engine's SDL/graphics/frame-loop object. `EditorWindow` is the Qt desktop shell. `ViewportPanel` connects them through the non-blocking C++ `runtime::Context` API; the separate C API exists for FFI consumers such as the Rust CLI.
- **Front-end type vs backend implementation:** Atlas exposes engine-facing types such as `Rigidbody`, `AudioPlayer`, `RenderTarget`, and `Pipeline`; Bezel, Finewave, and Opal contain the lower-level backend work.

## Reference notation

`atlas/application/window.cpp:1405` means “the definition beginning at line 1405.” A range such as `1405-1954` means that the implementation spans that region. Header references point to the public contract; source references point to behavior.

The guide documents architecturally meaningful functions individually. Routine getters, setters, value conversions, and repetitive scripting shims are grouped by responsibility in the source index instead of receiving hundreds of near-identical paragraphs.
