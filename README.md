<div align="center">
  <img src="images/logo.svg" width="60" alt="Atlas Engine">
  <h1>Atlas Engine</h1>
  <b>Build everything. Then go beyond.</b><br></br>
</div>

![GitHub contributors](https://img.shields.io/github/contributors/neutralsoftware/atlas)
![GitHub last commit](https://img.shields.io/github/last-commit/neutralsoftware/atlas)
![Tests](https://github.com/neutralsoftware/atlas/actions/workflows/build.yaml/badge.svg)
![GitHub Issues or Pull Requests](https://img.shields.io/github/issues/neutralsoftware/atlas)
![GitHub Repo stars](https://img.shields.io/github/stars/neutralsoftware/atlas)
[![](https://dcbadge.limes.pink/api/server/WKrxKtr7kW)](https://discord.gg/WKrxKtr7kW)

Atlas is an open-source game engine built for creating real-time 3D experiences with a native editor, TypeScript scripting, and modern rendering backends. The engine is written in C++20 and brings rendering, physics, audio, terrain, environments, UI, tooling, and project packaging together in one repository.

<div align="center">
  <a href="https://atlasengine.org">Website</a> ·
  <a href="https://github.com/neutralsoftware/atlas/releases">Releases</a> ·
  <a href="ROADMAP.md">Roadmap</a> ·
  <a href="https://github.com/neutralsoftware/atlas/issues">Report an issue</a> ·
  <a href="https://discord.gg/WKrxKtr7kW">Discord</a>
</div>
<br></br>

![Atlas Screenshot](images/cornell-box-trim.png)
![Editor Screenshot](images/editorExample.png)

## Overview

Atlas provides both a visual editor and a standalone runtime. Projects are described with a `project.atlas` manifest, scenes are stored as `.ascene` files, and gameplay can be implemented with TypeScript components. The Atlas CLI handles the project lifecycle from creation through building and packaging.

The engine currently supports Metal, Vulkan, and OpenGL through the Opal rendering abstraction. macOS is the primary development platform; Windows and Linux support are part of the cross-platform design but may require additional platform-specific setup.

### Highlights

- Native editor with a hierarchy, inspector, viewport, content browser, console, and project tooling
- Forward, deferred, and path-traced rendering modes
- Metal, Vulkan, and OpenGL rendering backends
- TypeScript scripting through the Atlas runtime and QuickJS
- Scene serialization, asset management, input actions, and project packaging
- 3D physics, collision queries, joints, and vehicles through Bezel and Jolt
- Spatial audio and effects through Finewave and OpenAL Soft
- Terrain generation, atmospheric environments, weather, and fluids
- Graphite UI authoring and runtime rendering
- Global illumination, post-processing, and debugging tools

## Engine Architecture

Atlas is organized as a set of focused systems that are linked into the runtime and editor:

| System | Responsibility |
| --- | --- |
| Atlas | Scene rendering, objects, input, windows, resources, and the frame loop |
| Opal | Rendering abstraction across Metal, Vulkan, and OpenGL |
| Photon | Path tracing and global illumination |
| Bezel | Physics integration and simulation |
| Finewave | Audio playback and processing |
| Aurora | Terrain generation and rendering |
| Hydra | Atmosphere, environments, weather, and fluids |
| Graphite | Engine UI layout and rendering |
| Tracer | Logging and debugging facilities |
| Runtime | Project loading, scene execution, scripting, and the C API |
| Atlas Editor | Qt desktop editor built on the same runtime |

For a deeper source-guided explanation, start with the [architecture guide](internals/README.md).

## Get Atlas

The easiest way to use Atlas is to download a packaged build from [GitHub Releases](https://github.com/neutralsoftware/atlas/releases). Visit [atlasengine.org](https://atlasengine.org) for project news and the main Atlas entry point.

To work on the engine itself, build it from source using the instructions below.

## Build from Source

### Requirements

- Git with submodule support
- CMake 3.21 or newer
- Ninja
- Clang with C++20 support
- Python 3
- Qt 6 with the Widgets component
- Rust and Cargo for the Atlas CLI bundled with the editor
- Internet access during the first configuration so CMake can fetch dependencies

Atlas fetches SDL3, GLM, fmt, FreeType, Assimp, OpenAL Soft, and Jolt during configuration. Vulkan builds additionally require a Vulkan SDK or equivalent Vulkan loader and SPIRV-Cross development packages.

fTetWild softbody tetrahedralization is enabled by default on supported platforms and disabled on Windows. Use `-DENABLE_FTET_WILD=OFF` when configuring CMake to disable it, or `just config AUTO OFF OFF`. Cached tetrahedral meshes remain usable with fTetWild disabled; generating new meshes requires `-DENABLE_FTET_WILD=ON` on a supported platform.

### macOS

Install the Xcode command-line tools and build dependencies:

```bash
xcode-select --install
brew install cmake ninja qt@6 python rust
```

Clone Atlas with its submodules:

```bash
git clone --recurse-submodules https://github.com/neutralsoftware/atlas.git
cd atlas
```

Configure and build the default backend. On macOS, `AUTO` selects Metal:

```bash
cmake -S . -B build -G Ninja \
  -DBACKEND=AUTO \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
cmake --build build --parallel
```

Launch the editor:

```bash
open "build/bin/Atlas Engine.app"
```

To build only the editor and its required dependencies, use:

```bash
cmake --build build --target AtlasEditor --parallel
```

### Linux and Windows

Install the requirements above using your platform's package manager, including Qt 6 Widgets and the development packages for your selected graphics backend. Clone with `--recurse-submodules`, then configure and build with CMake:

```bash
git clone --recurse-submodules https://github.com/neutralsoftware/atlas.git
cd atlas
cmake -S . -B build -G Ninja -DBACKEND=AUTO
cmake --build build --parallel
```

`AUTO` selects Vulkan on non-Apple platforms. Use `-DBACKEND=VULKAN` to select Vulkan explicitly.

### Select a Rendering Backend

The backend is selected at configure time:

```bash
cmake -S . -B build-metal -G Ninja -DBACKEND=METAL
cmake -S . -B build-vulkan -G Ninja -DBACKEND=VULKAN
```

Valid values are `AUTO`, `METAL`, and `VULKAN`. Metal is available on Apple platforms.

### Build with `just`

If you have [just](https://github.com/casey/just) installed, the repository includes shortcuts for common tasks:

```bash
just config
just build
just editor
```

You can select a backend when configuring:

```bash
just config METAL
just config VULKAN
just config OPENGL
```

Build products are written to `build/bin` and libraries to `build/lib`.

## Native C++ Tests

Each native test lives in `tests/<name>/main.cpp` and links directly to
`Atlas::Atlas`. Call the C++ engine API directly, including `CoreObject::rotate`,
or create a `Window` and `Scene` for rendering experiments. No script bindings,
project manifest, editor, or inspector changes are needed.

```bash
just run
just run simple
just run transforms
just addTest my-feature
just run my-feature
```

`just run` reads the default name from `tests/default.txt`, currently `simple`,
which opens a rotating cube. `transforms` checks object movement, rotation,
scaling, and instance transforms without opening a window. A failed check
returns a nonzero exit code.

`just addTest <name>` creates a folder containing a copy of the native `simple`
example. Edit its `main.cpp`; CMake discovers the new test automatically and also
compiles any additional `.cpp` files within that folder. Relative asset paths
resolve from the test folder, and shared assets can be read from `../resources`.

`just run <name> [backend] [bezel_native] [ftet_wild]` configures and builds only
the selected executable and its engine dependencies under `build/native-tests`.
The editor and scripting runtime are disabled in this separate configuration;
the usual engine, graphics, and shader toolchain dependencies still apply.
For example, `just run simple VULKAN` selects Vulkan. The equivalent CMake target
is `atlas_test_simple`.

Native consumers link against the `Atlas::Atlas` CMake target, which exposes the
engine headers and transitive dependencies. A native-only configuration uses
`-DATLAS_BUILD_EDITOR=OFF -DATLAS_BUILD_RUNTIME=OFF`. This is an in-tree CMake
integration; an installed SDK is not provided yet.

Existing scripted scene projects remain available with `just run-project <name>`
using the CLI and `atlasrun` from the regular build. `just debug [name]` launches
a native test with the Metal HUD enabled on macOS, and `just frametest [name]`
performs a short native startup smoke run.

## Atlas CLI Workflow

The `atlas` CLI manages the full lifecycle of an Atlas project:

```bash
atlas create myProject
cd myProject
atlas build
atlas run
atlas pack
```

The main commands are:

| Command | Purpose |
| --- | --- |
| `atlas create <name>` | Create a project and choose its platform and rendering backend |
| `atlas build` | Configure and build the current project |
| `atlas run` | Build and launch the current project |
| `atlas pack` | Create a distributable app or executable under `dist/` |
| `atlas register <version> --atlas <path> --runtime-lib <path>` | Register an Atlas CLI and runtime library in `~/.atlas/config.json` |
| `atlas clangd` | Generate and expose `compile_commands.json` for editor tooling |
| `atlas script init` | Initialize TypeScript support in a project |
| `atlas script compile` | Compile the project's TypeScript bundle |
| `atlas script new <name>` | Create a new script component |

Override the manifest backend for an individual command when needed:

```bash
atlas build --backend VULKAN
atlas run --backend OPENGL
atlas pack --backend METAL
atlas clangd --backend METAL
```

## Repository Guide

| Path | Contents |
| --- | --- |
| `atlas/` | Core engine implementation |
| `include/atlas/` | Public engine headers |
| `editor/` | Qt editor application |
| `runtime/` | Project runtime, scripting host, executable, and runtime API |
| `cli/` | Rust command-line tool |
| `opal/` | Rendering backend abstraction |
| `photon/` | Global illumination and path tracing |
| `bezel/` | Physics layer |
| `finewave/` | Audio layer |
| `aurora/` | Terrain system |
| `hydra/` | Environment and fluid systems |
| `graphite/` | UI system |
| `shaders/` | Backend-specific shaders |
| `tests/` | Example and test projects |
| `internals/` | Source-guided architecture documentation |

## Documentation and Community

- Visit the [Atlas website](https://atlasengine.org)
- Read the [internal architecture guide](internals/README.md)
- Follow development on the [roadmap](ROADMAP.md)
- Ask questions and share work on [Discord](https://discord.gg/WKrxKtr7kW)
- Report bugs or request features through [GitHub Issues](https://github.com/neutralsoftware/atlas/issues)

## Contributing

Contributions are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md) for branch conventions and pull-request expectations, and follow the [Code of Conduct](CODE_OF_CONDUCT.md) when participating in the community.

## License

Atlas Engine is available under the [MIT License](LICENSE.md).

## Renders

<p align="center">
  A selection of scenes rendered with Atlas and Photon.
</p>

<table>
  <tr>
    <td width="50%">
      <img src="images/diamond-ring.png" alt="Diamond ring render" width="100%">
    </td>
    <td width="50%">
      <img src="images/diamonds.png" alt="Diamond render" width="100%">
    </td>
  </tr>
  <tr>
    <td align="center"><b>Diamond Ring</b></td>
    <td align="center"><b>Diamonds</b></td>
  </tr>
  <tr>
    <td width="50%">
      <img src="images/cornell-box.png" alt="Cornell Box with Glass Dragon" width="100%">
    </td>
    <td width="50%">
      <img src="images/prism.png" alt="Prism scattering colors" width="100%">
    </td>
  </tr>
  <tr>
    <td align="center"><b>Cornell Box with Glass Dragon</b></td>
    <td align="center"><b>Spectral rendering in a Prism</b></td>
  </tr>
</table>

