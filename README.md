# Atlas Engine

![GitHub contributors](https://img.shields.io/github/contributors/neutralsoftware/atlas)
![GitHub last commit](https://img.shields.io/github/last-commit/neutralsoftware/atlas)
![Tests](https://github.com/neutralsoftware/atlas/actions/workflows/build.yaml/badge.svg)
![GitHub Issues or Pull Requests](https://img.shields.io/github/issues/neutralsoftware/atlas)
![GitHub Repo stars](https://img.shields.io/github/stars/neutralsoftware/atlas)
[![](https://dcbadge.limes.pink/api/server/WKrxKtr7kW)](https://discord.gg/WKrxKtr7kW)

Atlas is a Game Engine that uses the latest technologies to provide a fast, simple but powerful experience for developers.
It is built with C++ and uses OpenGL, Vulkan and Metal for rendering. It also has a physics engine, an audio engine, a terrain system,
an environment system and a debugging system with more to come.

![Atlas Screenshot](example.png)
![Editor Screenshot](editorExample.png)

## Features

- Cross-platform support for Windows, macOS, Linux although the main focus is on macOS.
- Fast graphics rendering with OpenGL, Vulkan and Metal (via Opal)
- Simple and intuitive API
- Physics engine (Bezel)
- Audio engine (Finewave)
- Terrain system (Aurora)
- Environment system (Hydra)
- Debugging system (Tracer)
- Global illumination and path tracing (Photon)
- UI library (Graphite)
- Powerful CLI tool for project management and build automation
- Asset management
- Intuitive C++ scripting mode

## Engine Architecture

- The main renderer is Atlas. Atlas is the one in charge of the graphics rendering, input handling, window management and more.
- The physics engine is Bezel. Bezel is a separate library that handles all the physics calculations and simulations.
- The audio engine is Finewave. Finewave is a separate library that handles all the audio playback and processing.
- The terrain system is Aurora. Aurora is a separate library that handles terrain generation and rendering.
- The environment system is Hydra. Hydra is a separate library that handles sky, atmosphere, weather and fluid simulation.
- The rendering backend is Opal, an in-house rendering abstraction layer that allows for easy switching between different graphics APIs.
- The debugging system is Tracer. Tracer is a module in the engine that provides various debugging tools and utilities.
- The illumination engine is Photon. Photon is a module in the engine that handles global illumination and path tracing.
- The UI library is Graphite. Graphite is a module in the engine that provides a set of tools and components for creating user interfaces.

## Getting the Engine

Go to the releases page [here](https://github.com/neutralsoftware/atlas/releases) to download the latest version of Atlas. Then you can follow the instructions in the documentation to get started.

```bash
atlas create myProject
```

## Atlas CLI Workflow

The CLI now provides a full project lifecycle:

```bash
atlas create myProject
atlas build
atlas run
atlas pack
atlas clangd
```

### Project Creation Experience

- `atlas create` offers an interactive terminal flow.
- You can choose:
  - release version (from available GitHub releases)
  - target platform (`macOS`, `Windows`, `Linux`)
  - rendering backend (`METAL`, `OPENGL`, `VULKAN`)
- It downloads compiler-independent prelinked macOS libraries:
  - `macOS-atlas-metal.a`
  - `macOS-atlas-opengl.a`
  - `macOS-atlas-vulkan.a`
- It can optionally create/switch to a new git branch right after scaffolding.

### Release Output

Running:

```bash
just release
```

now produces:

- `dist/release/macOS-atlas-metal.a`
- `dist/release/macOS-atlas-opengl.a`
- `dist/release/macOS-atlas-vulkan.a`

Each archive bundles Atlas and its internal engine modules into one static archive per backend.

### Build / Run / Pack / Clangd

- `atlas build` configures and builds using the backend from `project.atlas`.
- `atlas run` builds then runs the produced executable.
- `atlas pack` creates distributable output from the configured Atlas runtime and project resources:
  - macOS: `.app` bundle in `dist/`
  - other platforms: executable in `dist/`
- `atlas clangd` generates `build/compile_commands.json` and exposes it at project root as `compile_commands.json`.

You can override backend per command:

```bash
atlas build --backend VULKAN
atlas run --backend OPENGL
atlas pack --backend METAL
atlas clangd --backend METAL
```

## Building from Source

To build the engine from source, you will need to have CMake and a C++ compiler installed on your system. Then you can clone the repository and build the engine using the following commands:

```bash
git clone https://github.com/neutralsoftware/atlas.git
cd atlas
mkdir build
cd build
cmake ..
make
```

You can then run the engine using the following command:

```bash
./Atlas
```

Be sure to check the documentation for more detailed instructions on building and running the engine.
