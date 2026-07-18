# Atlas Editor workflow

## Scene and project commands

| Command | Shortcut |
| --- | --- |
| New scene | Command N |
| Open scene | Command O |
| Save scene | Command S |
| Save scene as | Command Shift S |
| Close scene tab | Command W |
| Quit Atlas | Command Q |
| Project settings | Command , |
| Export project | File > Export Project |
| Build project | Command B |
| Run project | Command Shift B |

Scenes open as tabs above the viewport. Creating a scene uses the Atlas scene dialog and writes a new `.ascene` file. Project settings are stored inside the project at `.atlas/project-settings.ini`.

## Editing commands

| Command | Shortcut |
| --- | --- |
| Undo | Command Z |
| Redo | Command Shift Z |
| Cut, copy, paste | Command X, Command C, Command V |
| Duplicate selection | Command D |
| Delete selection | Backspace |
| Select all | Command A |
| Move, rotate, scale | G, R, S |
| Reset position | Command Option G |
| Reset rotation | Command Option R |
| Reset scale | Command Option S |
| Toggle local or world space | Shift T |
| Focus or frame selection | Tab |
| Pan viewport | Right-click and drag |
| Orbit viewport | Middle-click and drag |

G, R, and S start a modal transform. X, Y, and Z constrain axes; multiple axis keys combine constraints; Shift plus an axis excludes it. Enter or left click confirms. Escape or right click cancels. The pointer wraps around the viewport during modal transforms so the operation can continue without reaching a screen edge.

## Object and asset commands

| Command | Shortcut |
| --- | --- |
| Create empty | Shift N |
| Create camera | Shift C |
| Create point light | Shift L |
| Open Add Object | Shift A |
| Reparent | Shift R |
| Rename hierarchy object or asset | Enter |
| Search hierarchy | Command Shift F |
| Search assets | Command Option F |
| Refresh asset database | Command Shift R |

Shift-click selects multiple hierarchy objects. Dropping OBJ, FBX, glTF, GLB, or DAE files from the Content Browser onto the viewport imports a model.

## Runtime and discovery

| Command | Shortcut |
| --- | --- |
| Play or pause | Command P |
| Stop | Command Shift L |
| Step one frame | Command Shift K |
| Global project search | Command F |
| Command palette | Command Shift P |

The command palette lists available commands and their shortcuts and supports keyboard filtering, arrow navigation, and Enter. Scripts are watched for changes and the editor reloads the runtime automatically when JavaScript or TypeScript files change.

## Packaging Atlas Engine

Run the macOS packer from the repository root:

```shell
./scripts/package_app.py --debug --macOS
./scripts/package_app.py --release --macOS
```

The equivalent `just` recipes are `just package-debug-macos` and `just package-release-macos`. Products are written below `dist/macOS/<configuration>` and intermediate files are written below `build/package`; both directories are ignored by version control.

The package is self-contained and includes Qt, the Atlas CLI, and `runtime.dylib`. On first launch, Atlas Engine offers to install the bundled CLI and runtime into `~/Library/Application Support/Atlas Engine/toolchains/alpha9` and registers them in `~/.atlas/config.json`. Installation does not require administrator access and preserves other configured Atlas versions.

Debug packages use an ad-hoc signature. A release intended for GitHub requires `ATLAS_SIGNING_IDENTITY` to name a Developer ID Application certificate and `ATLAS_NOTARY_PROFILE` to name a `notarytool` keychain profile. The packer creates, signs, notarizes, staples, mounts, and validates a drag-to-Applications DMG. It refuses to create an accidentally unnotarized release unless `ATLAS_ALLOW_UNNOTARIZED_RELEASE=1` is explicitly set; that local-only artifact is named `UNNOTARIZED`.

The packer builds the host architecture by default. Set `ATLAS_MACOS_ARCHITECTURES='arm64;x86_64'` when the selected Qt installation contains both architectures to create a universal archive. `ATLAS_MACOS_DEPLOYMENT_TARGET` changes the default macOS 14.0 deployment target, and `ATLAS_MACDEPLOYQT` can select a specific `macdeployqt` executable.
