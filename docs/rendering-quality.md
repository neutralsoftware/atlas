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
