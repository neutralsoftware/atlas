# Editor geometry and scene rendering

Editor light proxies are interaction aids. Exclude editor-only renderables and model meshes from the deferred G-buffer and path-tracing object collection. Forward-only objects must not make an otherwise empty path-traced scene count as invalid scene geometry. Retain validation errors for actual scene meshes with invalid indices.

Draw editor-only late-forward objects once, after scene render targets are composited and before the remaining editor controls. Use filled triangles without depth reads or writes, restoring the prior render state afterward. This keeps light handles visible as overlays and prevents scene exposure and bloom from changing their appearance. They are hidden when editor controls are disabled and omitted from offscreen scene output.

The inspected Windows runtime log reported `Scene geometry is not valid for path tracing` on startup. The opened scene had no ordinary objects and one ambient-light editor proxy. That proxy was collected before eligibility filtering, preventing the empty-scene path from running. The user also reported red output in raster mode; the available log does not independently establish that mode's failure.

Clang syntax-only checks passed for the modified window, deferred-rendering, and path-tracing translation units using the existing Windows compile database. No object files were generated, and Atlas was not built or GPU-tested. Visual acceptance still requires an empty scene with light handles, ordinary geometry, switching raster/path-traced modes, light intensity changes, and editor playback.
