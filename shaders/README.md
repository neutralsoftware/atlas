# Atlas shaders

Slang is the source language for Atlas raster passes and Photon denoising. `manifest.json` records each program, stage, entry point, and packed C++ symbol. Vertex and fragment stages share a file; `ATLAS_VERTEX`, `ATLAS_FRAGMENT`, and `ATLAS_COMPUTE` select the stage. Reusable code lives in `utils`.

CMake requires `slangc` and `spirv-cross`. Slang 2024.13 is the validated compiler and is installed by CI. Override `SLANGC_EXECUTABLE` or `SPIRV_CROSS_EXECUTABLE` when these tools are outside PATH.

`generate_shaders` compiles Slang to SPIR-V. Vulkan consumes that bytecode; Metal consumes MSL generated from the same bytecode with SPIRV-Cross. The packed header and intermediate artifacts live under `build/generated/<backend>/atlas/core`. Nothing generated is written into the source tree. Source files, shared libraries, manifests, compiler executables, and generation settings are dependencies. Compilation errors stop generation; an incomplete header never replaces the previous one.

Author entry points use `vertexMain`, `screenVertexMain`, `fragmentMain`, or `computeMain`. Generated entry points are normalized to Vulkan `main` and Metal `main0`, matching Opal's existing loaders. Matrices use row-major Slang storage and row-vector `mul` calls to preserve Atlas's column-major CPU matrix bytes. Do not change one convention without the other.

The name-based uniform API remains supported. Opal reflects generated array and matrix wrappers, indexed uniforms, named storage buffers, and sampler-free textures. Photon ray tracing stays native Metal under `photon/metal`; its local includes are expanded before packing.

`BACKEND` supports `AUTO`, `METAL`, and `VULKAN`. `ATLAS_PHOTON_BACKEND` supports `AUTO`, `METAL`, `VULKAN`, and `OFF`; AUTO enables native Photon only on Metal. A future Vulkan implementation must provide `photon/vulkan/manifest.json`, with `PATH`, `DDGI`, and `DDGI_WRITE` entries. The manifest accepts `shaders` entries in the root manifest format or `native` entries containing a shader-root-relative `.spv` source and symbol. Selecting Vulkan Photon before this exists fails explicitly. No Vulkan ray-tracing implementation is included.

Legacy forward Main shaders now resolve to deferred PBR. Unused Blinn-Phong, tessellated terrain, geometry-shadow variants, SSR blur, and volumetric-cloud shaders have been removed. The old terrain and geometry shader factory values reject use explicitly. Cloud settings retained in older scene files no longer activate a rendering pass. Volumetric light scattering, fluids, bloom, SSAO, SSR, skyboxes, text, and debugging passes remain.

Shader-only verification, without building Atlas:

```sh
python3 scripts/pack_shaders.py shaders /tmp/atlas-shaders/default_shaders.h metal --photon-backend metal
python3 scripts/check_shader_contracts.py /tmp/atlas-shaders/shader_artifacts
spirv-val --target-env vulkan1.1 /tmp/atlas-shaders/shader_artifacts/DEFERRED_FRAG.spv
xcrun -sdk macosx metal -std=metal3.0 -c /tmp/atlas-shaders/shader_artifacts/DEFERRED_FRAG.metal -o /tmp/deferred.air
```

The contract checker compiles an isolated harness from Opal's actual reflection functions and compares Vulkan and Metal uniform offsets and buffer names. It requires a C++20 compiler and SPIRV-Cross development headers/library; `--spirv-cross-prefix` selects their installation prefix. Shader validation does not replace an Atlas build or a rendered-scene check.
