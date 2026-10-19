# Atlas Roadmap

## For Beta 3

### Editor and User Experience

- Redesign the Atlas Editor UI with a modern, consistent visual style M1
- Improve the Scene Hierarchy, Inspector, Asset Browser and editor docking system M2
- Add editor quality-of-life features: better gizmos, keyboard shortcuts, drag-and-drop workflows and undo/redo support M3

### Animation

- Add skeletal animation, GPU skinning and animation importing (glTF) M4
- Implement an animation playback system with interpolation, looping, blending and crossfading M5
- Introduce animation state machines with parameters, transitions and an editor graph M6
- Add an animation timeline and animation preview tools M7

### Shader and Material System

- Introduce a node-based material editor with shader generation using Slang M8
- Add procedural material nodes, including noise, gradients, mathematical operations, UV manipulation and texture blending M9
- Integrate custom material graphs with Atlas PBR and Photon rendering pipelines M10
- Add material instances, reusable parameters and live material previews M11

### Asset Pipeline and Scene Workflow

- Improve the asset importing pipeline and introduce reliable asset reimporting and hot reload M12
- Add reusable scene prefabs with editable properties and instancing M13
- Improve asset serialization, dependency tracking and project organization M14

### Final Integration

- Optimize editor responsiveness, material compilation and animation performance M15
- Build a complete demo scene showcasing animated characters, procedural materials and the redesigned editor M16

---

## For Beta 2

- Investigate and move shaders to slang _M1_
- Add better support for Vulkan in Opal _M2_

- Move Photon to a spectral renderer _M3_
- Add dispersion, volumes and accurate caustics to Photon _M4_
- Add iridescence and SSS to Photon _M5_
- Introduce denoising and ReSTIR DI to Atlas _M6_
- Make Photon real-time by using different techniques _M7_

- Fix PBR SSR and DDGI isssues and add transmission, refraction and fresnel to the PBR pipeline _M8_
- Add geometry physical displacement and tessellation _M9_

- Add Bezel Jolt softbodies and cloth simulation _M10_
- _Optional:_ Revamp Bezel Native

- Move Photon to slang to be prepared for Windows _M14_
- Add Vulkan Ray Tracing API support _M15_
- Build Atlas for Windows and Linux _M16_
