# Adding a Class to Atlas

This guide describes the complete path for exposing a new Atlas class to TypeScript, JavaScript, the native runtime, scene files, and the editor Inspector. These are separate contracts: adding a C++ class does not automatically make it scriptable, serializable, or visible in the Inspector.

## The contracts

An Inspector-editable component normally has these layers:

1. Native type: the C++ class owns runtime behavior and native state.
2. TypeScript declaration: `runtime/atlas.d.ts` describes the scripting API.
3. JavaScript implementation: `runtime/scripts/atlas.js` provides the runtime module surface.
4. Scene JSON: the component has a stable `type` token and property names.
5. Runtime parser: `runtime/lib/context.cpp` creates the component from JSON.
6. Runtime updater: the same file applies Inspector edits to an attached instance.
7. Editor schema: `editor/views/editor/inspector.cpp` supplies defaults and controls.
8. Editor menu: the component is listed in the Inspector Add Component menu.
9. Distribution: runtime script bundles and declaration copies include the new API.

The data flow is:

```text
Inspector schema -> component JSON -> Context::addObjectComponent
                                  -> attachComponent
Inspector edit  -> Context::setObjectProperty
                 -> updateAttachedComponent
Scene save/open  <-> editorComponentData
TypeScript       -> atlas.d.ts
JavaScript       -> atlas.js
```

## 1. Define the native class

Put the public declaration in the owning header. For a mesh component, that is usually `include/atlas/object.h`; for a physics component, use the corresponding domain header.

```cpp
enum class SubdivisionScheme {
    Simple,
    Loop,
};

class Subdivision : public TraitComponent<CoreObject> {
  public:
    unsigned int levels = 1;
    SubdivisionScheme scheme = SubdivisionScheme::Loop;

    void init() override;
    void subdivide();
};
```

Implement behavior in the owning source file. `init()` establishes initial runtime state. Keep editor parsing out of the class when possible; the Context owns JSON and editor lifecycle concerns.

## 2. Choose the JSON contract

Choose the stable component token and property names before writing parser code. The Subdivision contract is:

```json
{
  "type": "subdivision",
  "levels": 1,
  "scheme": "loop"
}
```

Prefer readable enum strings in scene JSON. Accept numeric enum values too when the TypeScript API exposes numeric enum values or compatibility requires it.

Use these shapes for common values:

```json
{
  "title": "Mesh detail",
  "enabled": true,
  "mode": "loop",
  "count": 3,
  "position": [0, 1, 0],
  "color": [1, 0.5, 0.1, 1],
  "settings": {
    "name": "high detail",
    "direction": [0, 1, 0]
  },
  "items": [
    { "name": "first", "position": [0, 0, 0] }
  ]
}
```

The generic Inspector understands booleans, numbers, strings, numeric arrays, colors, nested objects, and structured arrays. A numeric array with two or three values becomes a vector editor. Four numeric values become a color when the property name identifies a color. Arrays whose entries are objects become expandable structured arrays.

## 3. Add the TypeScript declaration

Add the type to `runtime/atlas.d.ts` inside the correct `declare module`. Import reusable types instead of redefining them.

```ts
import { Position3d, Color } from "atlas/units";

export enum SubdivisionScheme {
    Simple,
    Loop,
}

export class Subdivision extends Component {
    levels: number;
    scheme: SubdivisionScheme;
}

export type ExampleSettings = {
    title: string;
    enabled: boolean;
    position: Position3d;
    color: Color;
};
```

Use `string`, `number`, and `boolean` for primitive fields. Use imported unit types for vectors and colors. Use a union or enum for finite choices. Add `override` to lifecycle methods inherited from `Component`.

If test projects carry checked-in declaration copies, update them through the project’s normal distribution step and keep their public API consistent with `runtime/atlas.d.ts`.

## 4. Implement the JavaScript surface

Add the corresponding export in `runtime/scripts/atlas.js`. Initialize every public field that scripts can read.

```js
export const SubdivisionScheme = Object.freeze({
    Simple: 0,
    Loop: 1,
});

export class Subdivision extends Component {
    constructor(levels = 1, scheme = SubdivisionScheme.Loop) {
        super();
        this.levels = levels;
        this.scheme = scheme;
    }
}
```

Strings are passed directly:

```js
this.title = title;
```

Enums use frozen constants:

```js
this.mode = Mode.Loop;
```

Vectors and colors use existing unit constructors:

```js
this.position = Position3d.zero();
this.color = Color.white();
```

For a method backed by native behavior, forward through a dedicated `globalThis.__atlas...` bridge and define the matching native binding. Do not expose a JavaScript-only method that implies native state changed unless the bridge performs that change.

## 5. Parse JSON in the runtime

Add the component token to the supported set in `Context::addObjectComponent`, then add a branch in `attachComponent`.

Use the existing helpers:

```cpp
std::string title;
tryReadStringAny(data, {"title"}, title);

bool enabled = true;
tryReadBoolAny(data, {"enabled"}, enabled);

int count = 1;
tryReadIntAny(data, {"count"}, count);

Position3d position;
tryReadVec3Any(data, {"position"}, position);
```

For an enum string, normalize the token and map it explicitly:

```cpp
std::string scheme;
if (tryReadStringAny(data, {"scheme"}, scheme)) {
    component->scheme = normalizeToken(scheme) == "simple"
                            ? SubdivisionScheme::Simple
                            : SubdivisionScheme::Loop;
}
```

For a color, use the existing color parser. For a nested object, use `findField` and parse its fields. For an array, verify `is_array()` before iterating. For resource or object references, resolve relative paths against `pending.baseDir` and use the existing lookup helpers.

Attach the component to the object and return it through `finish(component)`. This records the runtime component at the same index as its JSON entry, which is required for later Inspector edits.

## 6. Update an attached component

Adding a component only handles initial creation. Inspector edits call `Context::setObjectProperty`, which updates `editorComponentData` and then calls `updateAttachedComponent`.

Add a `dynamic_pointer_cast` branch for the new class. Re-read the complete component JSON, not only the changed field, so nested edits remain coherent. Rebuild or refresh native state when required.

For Subdivision, changing `levels` or `scheme` calls `subdivide()` again. Components whose operation is destructive should restore their source mesh or recreate derived state before applying new settings; otherwise repeated Inspector edits can apply the operation repeatedly to already-derived data.

## 7. Add the Inspector schema and controls

In `componentSchema`, return defaults that define the initial Inspector fields:

```cpp
if (normalized == "subdivision") {
    return {{"levels", 1}, {"scheme", "loop"}};
}
```

The generic Inspector maps JSON shapes automatically:

```cpp
{{"name", ""},
 {"enabled", false},
 {"position", QJsonArray{0.0, 0.0, 0.0}},
 {"color", QJsonArray{1.0, 1.0, 1.0, 1.0}},
 {"settings", QJsonObject{{"speed", 1.0}}}}
```

Add enum choices in `choicesFor`. It receives the full property path, so use the path when two components have fields with the same final key:

```cpp
if (key == "scheme" && path.contains("subdivision"))
    return {"simple", "loop"};
```

Keep property paths stable because undo, property synchronization, and runtime updates use them. Use a custom editor only when generic JSON controls cannot represent the value.

## 8. Add the component menu item

Add a label and the exact JSON token to the `componentTypes` list in the Inspector:

```cpp
{"Subdivision", "subdivision"},
```

The existing action passes `componentSchema(componentType)` to `addRuntimeObjectComponent`, so the menu item, schema, supported runtime token, and parser must agree.

## 9. Validate the complete path

1. Check the changed C++ files with the project’s configured diagnostics.
2. Open a CoreObject in the editor and choose Add Component > Subdivision.
3. Confirm the Inspector shows `levels` and a `scheme` picker with `simple` and `loop`.
4. Change both fields and confirm the scene snapshot changes.
5. Save and reopen the scene; confirm JSON still contains `type`, `levels`, and `scheme`.
6. Start the runtime and confirm subdivision is applied to a triangle mesh.
7. Confirm an invalid or non-triangle mesh reports the existing runtime error without crashing the editor.

## Subdivision end-to-end example

```text
Add Component menu
    -> componentSchema("subdivision")
    -> {"type":"subdivision","levels":1,"scheme":"loop"}
    -> Context::addObjectComponent
    -> attachComponent
    -> Subdivision::init / subdivide

Inspector levels or scheme edit
    -> Context::setObjectProperty
    -> updateAttachedComponent
    -> Subdivision::subdivide
```

Subdivision is intended for `CoreObject` triangle meshes. `Simple` splits each triangle into four triangles using edge midpoints. `Loop` applies Loop subdivision rules. `levels` is the number of passes, and `0` leaves the mesh unchanged.