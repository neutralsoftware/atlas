# Atlas editor design

The current direction is a minimal developer workspace inspired by Vercel's Geist design system. It replaces the rejected glass-and-metal treatment. Visual quality comes from alignment, typography, spacing, clear states, and consistent icons.

## Surfaces and controls

Use neutral charcoal backgrounds, one-pixel solid dividers, and six-pixel control corners. Ordinary panels are flat and adjacent. Avoid gradients, metallic rims, inset shines, tinted cards, decorative shadows, oversized corners, and floating tool islands.

| Role | Value | Use |
| --- | --- | --- |
| Canvas | `#0A0A0A` | Main window and ordinary panels |
| Secondary surface | `#141414` | Inputs and restrained secondary regions |
| Hover | `#202020` | Pointer feedback |
| Selection | `#292929` | Active controls and selected rows |
| Border | `#2E2E2E` | Fields and dividers |
| Strong border | `#525252` | Hover and selected boundaries |
| Text | `#EDEDED` | Primary copy and primary action fill |
| Secondary text | `#A1A1A1` | Labels and supporting copy |
| Disabled text | `#707070` | Unavailable controls |
| Focus | `#52A8FF` | Keyboard focus outline |

Reserve color for focus, meaningful status, content previews, and the transform axes. Primary actions use light text-color fill with dark ink. Ordinary actions are outlined or transparent. Hover, pressed, checked, disabled, and focus states must remain distinct. Selection must survive pointer exit and loss of keyboard focus.

## Typography and icons

Geist is bundled locally and applied to the whole editor, with platform font fallbacks. Use 13 px for controls, smaller secondary labels, and larger text only for meaningful titles. Keep project names and paths elidable, with full text available in tooltips.

Lucide replaces Phosphor. Every existing `styling::Icon` identifier maps to a bundled SVG so callers retain their action semantics. Keep the original 24-unit outlines and consistent strokes. Cache rendered icon sets by symbol and color. Preserve disabled variants and content color swatches. Source versions and licenses are recorded in `editor/assets/THIRD_PARTY.md`.

## Composition

The top bar contains project identity, command discovery, and execution actions. A single horizontal navigation row selects Scene, Shading, Effects, or Interface and exposes search and panel visibility. The active workspace has a simple underline. This replaces the tall workspace rail and promotional subtitles.

Keep docked content spacious through compact chrome. Toolbars sit directly above their content with a divider. Collection and Assets retain separate search rows. Inspector identity is plain text and an icon. Its collapsible sections have a restrained outline and inset form contents. Keep the content canvas visually dominant.

Inspector properties use a full-width, wrapping native Qt label above each editor. Vector axes share one compact row with shrinkable numeric controls and a sync action. Native section headers replace the custom button text painter. Input text and selection colors are explicit within the inspector. On selection rebuild, hide outgoing widgets before deferred deletion and let the scroll content recompute its size constraints.

The reported one-pixel numeric text was reproduced in an isolated Qt 6.12.0 widget probe using the application stylesheet: a 468-pixel spinbox received a one-pixel internal line editor when `NoButtons` was combined with stylesheet styling. Keep `UpDownArrows` as the native layout mode and hide both arrow subcontrols with zero width and height in QSS. Do not restore `NoButtons` in the installer or scrubbable controls. The revised probe checked 360, 560, and 800 pixel panel widths at 100%, 150%, and 200% scaling; internal text widths were 63, 129, and 209 pixels, and both arrow hit areas were empty. This verifies the isolated Qt geometry, not a compiled Atlas runtime.

## Implementation and behavior boundaries

- `editor/styling/dark.qss` defines shared controls and all existing panel, browser, dialog, search, and splash surfaces.
- `editor/styling/theme.cpp` supplies the neutral fallback palette.
- `editor/styling/workbench.cpp` paints flat buttons and selection states.
- `editor/styling/icons.cpp` maps the existing icon API to Lucide.
- `editor/main.cpp` loads the bundled Geist font.
- `editor/views/editor/editor.cpp` arranges the shell; existing actions, signals, dock identifiers, and saved-layout versions remain in use.

Keep runtime, scene editing, property updates, saves, playback, docking, and keyboard shortcuts outside presentation changes. Native viewport embedding must remain opaque and unobstructed. Do not add background blur or whole-window opacity. Do not migrate saved layouts merely because the visual theme changed.

The generated `include/editor/core/themes.h` must match the QSS source through `scripts/pack_qt_themes.py`.

## Validation and completion

Static checks cover icon-enum mapping, SVG validity, local resource references, font/license presence, generated theme parity, action-connection preservation, whitespace, and the absence of decorative gradients in the shared theme and button painter.

A build and runtime visual review have not been performed under the current no-build instruction. Do not claim perfect behavior from static checks. Before visual acceptance, inspect every workspace, project browser, menus, dialogs, empty states, and detached docks at 100%, 150%, and 200% scaling and the minimum supported window size. Exercise selection, undo/redo, property editing, scene save/switch, search, playback, and dock restoration. Verify the embedded viewport separately on Windows.

## References

- [Geist introduction](https://vercel.com/geist/introduction)
- [Geist colors](https://vercel.com/geist/colors): neutral defaults, limited background differentiation, distinct control states, and solid border hierarchy.
- [Geist buttons](https://vercel.com/geist/button): clear action affordances and accessible icon-only controls.
- [Lucide](https://github.com/lucide-icons/lucide)

These references guide the Qt implementation; Atlas does not embed Vercel's web components.

## Windows application audit

The local `build/bin/AtlasEditor.exe` inspected on 2026-10-23 contains the exact current QSS, Geist, and the Lucide component and panel-right resources. Phosphor is absent, and the deployed SVG icon engine and Qt SVG library are present. No running Atlas process was available to establish which executable the user launched. These findings establish embedded resource presence, not visual acceptance.

The asset browser now uses compact list rows with neutral Lucide icons. Disable implicit item editing so double-click only opens folders or assets. Enter opens the current item; F2 and the Rename menu retain explicit renaming. Ignore activation without a valid selection. An isolated Qt 6.12 event probe verified double-click emits one open with no inline editor, Enter opens, and F2 invokes rename. Startup diagnostics report the executable path, design identity, selected font family, and icon availability. The application was not built under the no-build instruction.

Folder navigation on Windows must use project-relative path containment. Qt cleans paths with forward slashes, so appending the native backslash separator to a root rejects valid descendants. Reject absolute relative results, `..`, and `../` prefixes instead. Clear the asset search before mapping the destination through the filter model. An isolated Windows Qt probe reproduced the old rejection and checked eight containment cases.

Lucide icons render directly with QSvgRenderer, linked through Qt6::Svg, rather than depending on an SVG QIcon plugin. Invalid resources produce a warning naming the asset. All 75 mapped SVGs were loaded and rendered in an isolated Qt probe. Full application build and visual verification remain outstanding under the no-build instruction.
