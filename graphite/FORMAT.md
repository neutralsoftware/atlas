# Atlas Graphite UI format

Graphite UI documents use the `.aui` extension and JSON encoding. Version 1 documents have this shape:

```json
{
  "format": "atlas.graphite.ui",
  "version": 1,
  "name": "Main HUD",
  "canvas": {
    "width": 1280,
    "height": 720,
    "background": [0.035, 0.04, 0.055, 1.0]
  },
  "defaultFont": {
    "source": "fonts/Inter-Regular.ttf",
    "size": 24
  },
  "elements": [
    {
      "id": "title",
      "name": "Title",
      "type": "text",
      "position": [48, 48],
      "content": "Atlas",
      "fontSize": 42,
      "color": [1, 1, 1, 1],
      "components": []
    }
  ]
}
```

A scene enables one or more UI documents through its top-level `ui` array. Paths are relative to the scene file.

```json
{
  "name": "Main",
  "objects": [],
  "ui": [
    "ui/Main HUD.aui",
    { "source": "ui/Pause Menu.aui", "enabled": true }
  ]
}
```

In a running game, Graphite renders after the scene. In the editor scene workspace it renders only while looking through the scene camera, keeping world editing unobstructed.

## Document fields

- `format`: `atlas.graphite.ui`.
- `version`: currently `1`.
- `name`: display name used by the editor.
- `canvas`: reference width, height, and preview background.
- `defaultFont`: font resource inherited by text-capable elements. `source` is relative to the `.aui` file.
- `elements`: top-level element array. `root` can be used instead when a single layout owns the document.

## Elements

Every element accepts `id`, `name`, `type`, `position`, `style`, and `components`. Layout elements also accept `children`.

Supported types are:

- `text`: `content`, `font`, `fontSize`, `color`.
- `image`: `source`, `size`, `tint`.
- `button`: `label`, `size`, `padding`, `font`, `fontSize`, `enabled`.
- `checkbox`: `label`, `checked`, `enabled`, `boxSize`, `spacing`, `padding`, `font`, `fontSize`.
- `textField`: `text`, `placeholder`, `size`, `maximumWidth`, `padding`, `font`, `fontSize`.
- `column` and `row`: `children`, `size`, `padding`, `spacing`, `alignment`, `anchor`.
- `stack`: `children`, `size`, `padding`, `horizontalAlignment`, `verticalAlignment`, `anchor`.

Colors use normalized RGBA arrays such as `[0.1, 0.2, 0.3, 1.0]`. RGB and 0–255 arrays are accepted as well. Positions and sizes use two-number arrays.

## Styles

The `style` object can contain `normal`, `hovered`, `pressed`, `focused`, `disabled`, and `checked` variants. A variant supports `padding`, `cornerRadius`, `background`, `borderWidth`, `border`, `foreground`, `tint`, and `fontSize`.

```json
{
  "style": {
    "normal": {
      "background": [0.12, 0.13, 0.17, 0.96],
      "foreground": [1, 1, 1, 1],
      "borderWidth": 1,
      "border": [1, 1, 1, 0.16],
      "cornerRadius": 10
    },
    "hovered": {
      "background": [0.18, 0.2, 0.26, 1]
    }
  }
}
```

## Script components

Each UI element uses the same component array as a scene object. Script components therefore participate in the normal Atlas script lifecycle, variable serialization, `init`, `update`, and `atAttach` callbacks.

```json
{
  "type": "button",
  "name": "Continue Button",
  "label": "Continue",
  "position": [80, 420],
  "size": [240, 56],
  "components": [
    {
      "type": "script",
      "source": "scripts/ContinueButton.ts",
      "className": "ContinueButton",
      "variables": {
        "scene": "Level One"
      }
    }
  ]
}
```

The editor stores script paths relative to the `.aui` document and uses the shared `atlas script compile` output when the project runs or previews.
