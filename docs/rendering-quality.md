# Rendering quality controls and validation

The Cornell test accepts `ATLAS_CORNELL_RENDERER=path`, `ddgi`, or `deferred`. The default remains path tracing with one stationary sample per pixel. Use the same camera, resolution, exposure, and scene for comparisons. Its render scale is explicitly 1.0; the old 5.0 value was already clamped to 1.0.

## Vulkan DDGI

Probe tracing and writeback share the exact Fibonacci ray directions. Probe hits evaluate shadowed directional, point, spot, and sampled rectangular area lights. Diffuse bounce feedback samples the surrounding irradiance probes at the hit position, with directional distance visibility and material color, metallic and transmission response. It no longer samples an arbitrary atlas location or injects unconfigured sky light. Probes dominated by backface hits are excluded, and fresh history converges without immediately applying full hysteresis.

Defaults are 0.3 world units between probes, 128 rays per probe, one update group, and 0.01 world units of surface bias. The existing grid cap is 16 probes per axis; large scenes therefore use coarser spacing. Public `GlobalIllumination` settings can reduce cost. Changing the ray count also resizes its storage before dispatch.

This remains diffuse probe GI, with opaque visibility and approximate rough reflections. It is not a replacement for Photon specular transport, glass caustics, or fine geometry below the probe spacing. The native Metal DDGI integrator is separate and does not receive the Vulkan transport changes here.

## Deferred lighting

Shadow maps affect only their owning light, at full strength. Ambient occlusion affects indirect lighting rather than darkening direct light. Rectangular lights use 16 area samples and area/inverse-square weighting; spotlights also use inverse-square falloff. DDGI replaces constant ambient and unoccluded diffuse environment lighting. Probe irradiance used for approximate reflection is converted back to radiance. The default bloom threshold is now 2.0 in linear HDR units; explicit scene overrides still apply.

Area shadows still use the existing filtered shadow map, not per-sample ray-traced visibility. Their penumbrae will differ from Photon.

## Photon motion and antialiasing

`PathTracing::minimumMotionSamples` defaults to 8, bounded to 1–64 when used. Initial frames, moving frames, and the short settling period render at least this many samples before presenting. The configured bounce limit is retained during movement. Stationary rendering resumes `raysPerPixel`; cinematic sampling retains its explicit total sample budget. Raising the motion floor costs GPU time and can reduce frame rate.

Both native path tracers use a separate center ray for stable denoising guides. Every radiance sample is jittered, including the one-sample setting. The extra guide ray is excluded from the radiance average. Camera movement still resets accumulated history; this change spends more samples per frame rather than reusing potentially invalid moving-camera history.

## Validation

Standalone Slang compilation and SPIR-V validation cover Vulkan DDGI tracing/writeback, Photon, and deferred lighting. No Atlas project build or GPU visual/performance run was performed. Native Metal compilation requires a macOS validation run.

After rebuilding, compare all three Cornell modes. Check red/green bounce on the white ceiling and boxes, shadow separation with multiple lights, and bloom on bright emitters rather than ordinary walls. Move continuously at one stationary sample per pixel, comparing motion floors of 1 and 8; inspect diagonal edges after settling. Measure GPU frame time before choosing higher probe or motion budgets for larger scenes.

## Activation and ceiling-shadow follow-up

`enableGlobalIllumination()` now enables the deferred `IndirectLighting` feature as well as probe updates. Previously the default realtime feature mask excluded indirect lighting, so the deferred shader ignored DDGI even after its probe pipeline was enabled.

One-sided area lights now use a perspective shadow frustum from a virtual point behind the emitter. Its near plane starts just beyond the emitter to exclude the debug plane and ceiling behind it. The depth pass retains both surface orientations and uses a smaller raster bias. This replaces the parallel orthographic projection that hid overhead box shadows beneath their footprints. Two-sided lights retain the existing orthographic approximation.

## GPU buffer layout and shadow filtering follow-up

Vulkan DDGI triangle records used a 272-byte shader stride while C++ uploaded 256-byte records. Changing the shader's vector padding to three scalar elements restores the 256-byte stride. Material color fields also disagreed: CPU albedo began at byte 40, but the shader expected byte 48. Material records now explicitly align albedo at 48, emissive color at 64, and optical data at 80, with a 176-byte stride. Native Metal material padding is updated to the same format. C++ and Metal static assertions guard record sizes; `python scripts/check_ddgi_layout.py` validates the actual Slang-generated SPIR-V layout for all six scene-data buffers.

Shadow filtering now interpolates four depth comparisons per tap, rather than interpolating raw depth or returning a nearest-texel result. Perspective area shadows use 24 blocker-search samples and 32 filtered taps, with a penumbra derived from emitter size and blocker/receiver separation. Filtering grows away from contact, up to a 32-texel cap. This adds texture-fetch cost and remains an approximation using one shadow map.

Validation includes a syntax-only check of `photon/gi.cpp` with its CPU layout assertions and SPIR-V reflection/validation. No project build, GPU image comparison, or native Metal compilation was performed.

## Position precision, emission, and effects coverage

World positions now use RGBA32F rather than RGBA16F. Quantized world-position derivatives were feeding false surface normals into DDGI and shadow bias. Lighting now prefers the stored surface normal, with geometric derivatives used only when no normal is available. Shadow filters use 48 blocker samples and 64 comparison taps with a stable kernel. This increases position-buffer bandwidth and shadow sampling cost.

Probe sampling rejects locations more than half a grid cell outside the volume and fades over the boundary. Near-zero visibility weights fade toward black rather than being normalized back to full brightness. The fallback that ignored surface orientation has been removed. This reduces exterior leakage but does not replace probe relocation or ray-traced visibility at the receiver.

A seventh, HDR emission attachment carries material emissive color times intensity into deferred lighting, bloom, and the scene color used by SSR. The EmissiveLighting feature controls visible emission. DDGI continues to gather emission through its probe rays. SSR is enabled by default for native windows; explicit application settings still override it.

| Photon feature | Deferred / DDGI mapping | Remaining difference |
| --- | --- | --- |
| Direct lighting and shadows | PBR lights, per-light filtered shadow maps | Approximate area-light visibility |
| Diffuse indirect light | DDGI material-colored probe transport | Grid-scale visibility, no GI in deferred without DDGI |
| Reflections | SSR, sky/environment fallback, approximate probe reflection | Off-screen local objects are unavailable to SSR |
| Emission | HDR emission attachment, bloom, DDGI probe source | Small emitters can be missed by probe rays |
| Transmission/refraction | Screen-space transmission and probe/environment fallback | No complete refracted geometry traversal |
| Dispersion | Per-channel refracted directions from IOR/Abbe number | Screen-space/environment approximation |
| Absorption | Attenuation color/distance and medium extinction | Estimated thickness |
| Volumes | Existing atmosphere/volumetric passes and material medium response | No general path-integrated heterogeneous transport |
| Normal maps / textures / alpha | Existing deferred material pass | Raster visibility rules |
| Iridescence | No deferred thin-film parameter mapping yet | Photon-only |
| Subsurface scattering | No deferred subsurface parameter mapping yet | Photon-only |
| Caustics | No deferred photon-map resolve yet | Photon-only |
| Diffraction | No physical diffraction solver in either renderer | Dispersion is a separate effect |

This table describes current coverage, not full effect parity. Full transport parity requires additional material storage and ray-traced secondary transport. Shader and syntax checks cannot establish visual equivalence; GPU comparison remains outstanding.

## DDGI ray-traced shadows and temporal stability

Vulkan DDGI now dispatches a per-pixel visibility pass after the geometry buffer and probe update. It traces opaque shadow rays against the DDGI scene acceleration structure, supporting the first two directional, four point, two spot, and two area lights (matching the DDGI light-table limits). Area lights use 64 fixed emitter samples with cosine/distance weighting. Visibility is stored in three RGBA atlas tiles and consumed per light by deferred lighting. Additional lights, non-DDGI rendering, and Metal retain filtered shadow maps. The global Shadows feature still disables shadow application.

This is ray-traced direct visibility alongside diffuse probe GI, not full path tracing. Alpha-tested/transmitting occluders use opaque visibility. Area-light BRDF integration still uses the existing 16 lighting samples; the 64-sample visibility ratio approximates their joint integral. Cost scales with pixels, light count, and shadow rays; the pass has no temporal denoiser or adaptive budget yet.

Vulkan probe directions and area-light sample seeds are now fixed across updates, preventing rotating visibility moments and changing direct-light samples from continually perturbing bounce feedback. Probe validity transitions smoothly over backface fractions and is blended with history. Fixed sampling trades temporal noise for deterministic integration error; increasing rays per probe improves that error. Geometry changes continue to rebuild the acceleration structure through the existing layout update path.

Validation covers standalone SPIR-V compilation/validation of both DDGI entry points, writeback, and deferred lighting, plus C++ syntax checks. The new DDGI_SHADOWS packed shader symbol is generated from the Vulkan manifest during the next build. No project build or GPU visual/performance run was performed.
