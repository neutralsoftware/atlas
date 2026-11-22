# Concurrent loading and rendering

Atlas uses worker threads for CPU data preparation. The calling thread still owns the scene, progress callbacks, GPU resource creation, and command submission.

## Shared CPU workers

[Workers](../include/atlas/core/workers.h) is created on first use. It leaves one logical CPU available for the caller where possible and uses up to 12 worker threads. An ordered job window keeps a bounded number of results in flight. As soon as the next result is ready, the caller schedules a replacement before consuming it. Workers can finish out of order while the caller applies results in source order.

Worker functions read stable inputs and write private results or disjoint output ranges. Consumers run on the caller and may mutate the scene or call Opal. If a job or consumer throws, the caller waits for outstanding jobs before releasing their inputs. Worker jobs return results to the caller and leave scheduling dependent work to that caller.

## Model startup

[Model import](../atlas/object/model.cpp) uses the worker pool in two stages.

- Texture decoding uses up to six pending jobs. A new decode can run while the caller uploads the preceding texture. Embedded texture jobs borrow data from the live Assimp scene, so queuing an image does not copy its compressed payload. The decoded cache hashes the input once and publishes completed files with a rename.
- Geometry preparation transforms vertices, validates indices, and calculates skin weights and tangents on workers. Independent meshes run in parallel. A large single mesh can instead distribute its vertex conversion across workers. Atlas moves the resulting arrays into model objects and attaches materials in scene order.

Assimp scene parsing and scene-cache persistence still precede texture and geometry jobs. Texture upload, GPU object initialization, and changes to Workspace resources stay on the calling thread.

## Photon and frame submission

[Photon](../photon/path_tracing.cpp) assigns offsets before it packs a scene. Workers convert independent acceleration-structure chunks into disjoint ranges of vertex, index, and triangle-owner data. The caller joins those jobs before uploading buffers and recording the build. Geometry indices, material slots, and emissive triangles retain the scene's original ordering. The caller combines chunk bounds after the workers finish. An unchanged scene reuses its geometry and skips the rebuild.

Photon omits volume processing when the packed scene contains no volume materials. This bypasses Vulkan's per-ray material scans for medium detection and shadow transmittance, and avoids dispatching the volume beam pass for those scenes.

[Window submission](../atlas/application/window.cpp) removes retired objects in one pass per render queue. The Vulkan HUD shows CPU render time and fence wait separately. Fence wait is time the CPU spent waiting for the previous submission. It is distinct from GPU execution time. Vulkan still uses the existing single in-flight frame, so the fence continues to protect shared resources reused by the next frame. Supporting additional in-flight frames requires per-frame command and resource lifetimes in Opal. [Khronos frames in flight](https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/03_Drawing/03_Frames_in_flight.html)

## Measuring the changes

Use the same scene, resolution, renderer, and sampling settings on both revisions. Record cold and warm model-load times separately. For Photon, record the first render and camera-movement frame times separately from a converged stationary image, where sampling stops. Compare frame time percentiles, CPU render time, Vulkan fence wait, and the GPU engine running the graphics or compute work. Asset-load time and frame-rate gains require an Atlas build and a GPU run.
