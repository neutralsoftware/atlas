# Atlas editor: Liquid Graphite

Atlas should feel like a carefully made creative instrument: quiet enough for long sessions, tactile enough to invite exploration. The new direction combines the clarity of a modern IDE, the spatial organization of Blender, and the luminous edges of glass and machined metal.

## Visual language

The canvas is deep blue graphite. Panels sit above it as rounded, opaque smoked-glass surfaces. Controls have a restrained vertical sheen, a fine silver rim, and a dark lower edge. Pale ice-blue selection makes the active tool unmistakable. Primary actions use a pearl finish with dark ink. Color belongs to content, selection, and meaningful status.

| Role | Value | Use |
| --- | --- | --- |
| Canvas | `#10141D` | Window, gaps, surrounding chrome |
| Surface | `#1B2230` | Docks, inspector, content browser |
| Raised surface | `#252F40` | Inputs and floating control groups |
| Edge | `#46556D` | Metal rims and surface boundaries |
| Text | `#E6EDF7` | Primary copy |
| Secondary text | `#A5B2C7` | Labels, paths, shortcuts |
| Selection | `#354E70` | Active rows and tools |
| Focus | `#9ACBFF` | Keyboard focus |
| Pearl | `#DFEDFF` | Primary action face |

Use 12–16 px panel corners, 10 px button corners, and 8 px field corners. Use a 4 px spacing rhythm, with 8–12 px between groups and 12–16 px around surfaces. Keep ordinary controls compact enough for an editor: approximately 34 px tall. Small icon controls retain their existing sizing where dock layouts require it.

Typography stays readable and understated: 13 px controls, 11–12 px supporting labels, larger titles only at major entry points. Prefer Manrope, then platform UI fonts. Do not replace meaningful labels with unexplained icons.

## Interaction and stability

Every control needs distinct resting, hover, pressed, checked, disabled, and keyboard-focus states. Hover brightens the material; pressing darkens it without shifting the layout. Checked state remains visible when the pointer leaves. Focus is a separate blue ring. Disabled primary actions must not look enabled.

Depth comes from painted gradients, inset highlights, and deliberate spacing. Use Qt's device-independent painting and existing widgets. Glass is an optical treatment on an opaque backing, so text contrast does not depend on whatever is behind the window. Avoid whole-window opacity, platform-specific backdrop APIs, and graphics effects around the embedded native viewport. Keep native menus, docking, resizing, input routing, and persisted layouts working.

Never animate layout geometry or obscure scene content with decorative overlays. Future motion should be short, interruptible, and optional. A viewport should receive the available space; decorative elements must not introduce new minimum-size constraints.

## Implementation map

- `editor/styling/dark.qss`: application surfaces, ordinary Qt controls, docks, project browser, inspector, dialogs, search, and splash.
- `editor/styling/theme.cpp`: fallback palette for native and unstyled widgets.
- `editor/styling/workbench.cpp`: shared painted buttons, item selections, and keyboard focus.
- `editor/views/editor/editor.cpp`: shell spacing and workspace navigation.
- `editor/views/editor/viewportTools.cpp`: inset viewport tool surface.

The QSS source is embedded by `scripts/pack_qt_themes.py` during the existing build workflow. Do not maintain a separate handwritten copy of the generated theme header.

## Redesign sequence

1. Establish the shared materials and states across the app; reshape shell spacing and workspace controls.
2. Rework hierarchy, inspector, and asset browsing around clearer grouping and progressive disclosure.
3. Redesign material, environment, and Graphite workspaces using the same components.
4. Refine project onboarding, empty states, command search, and dialogs.
5. Validate the complete application at multiple window sizes, display scales, and input methods.

The first pass changes the visual foundation across existing screens. It begins the redesign; it does not claim every workflow has been redesigned.

## Structural redesign audit

A new palette alone does not qualify as a new design. A redesigned screen must change its composition, information hierarchy, and control grouping while keeping its working actions accessible.

The second pass replaces the horizontal workspace switcher with a persistent left rail. Scene, Shading, Effects, and Interface each have an icon and label. Panel visibility controls sit at the foot of that rail. Project identity moves to the upper left, command discovery becomes a visible launcher in the center, and execution remains at the upper right. The active workspace has its own context heading.

Collection now separates object creation from object search. Asset browsing separates location navigation from filtering and creation. Inspector selection becomes an identity card, followed by inset, collapsible component cards. These changes alter the screen composition rather than relying on color differences.

| Area | Implemented | Still required for a complete redesign |
| --- | --- | --- |
| Editor shell | Vertical workspace navigation, command launcher, project identity, contextual heading | Visual sizing and dock restoration checks |
| Collection | Dedicated creation header and full-width search | Empty-state and large-scene review |
| Inspector | Selection identity card and inset component groups | Dense-property, camera, and multi-selection review |
| Assets | Separate location and action rows | New empty states, preview composition, and grid/list review |
| Viewport | Inset tool surface and shared control materials | Tool grouping and narrow-width visual review |
| Shading, Effects, Interface | Shared visual materials and new navigation | Dedicated workspace composition redesigns |
| Project browser, dialogs, search | Shared visual materials | Dedicated layout and onboarding redesigns |

Completion requires all rows to pass the visual and interaction checks below. The new shell is implemented in source; the entire application is not yet certified as a finished new design. Do not substitute a mockup, an old executable, or a stylesheet inspection for testing the changed application.

## Acceptance checks

Before calling any pass visually verified, launch the newly built editor and inspect project selection, scene editing, all workspaces, dialogs, and detached docks. Check 100%, 150%, and 200% scaling, the minimum supported window size, long project names, keyboard traversal, selected rows without focus, disabled controls, and pressed menu buttons.

Exercise docking and layout restoration, scene switching, selection and property editing, runtime playback, and return from playback. On Windows, separately verify embedded viewport rendering and empty-scene behavior. Styling checks alone cannot prove GPU or native-window correctness.

No build or runtime validation was performed for the initial pass, in accordance with the current repository instructions. Static review is only a preliminary check; visual acceptance remains open.
