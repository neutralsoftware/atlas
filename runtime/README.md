# How does the scene packing format work?

Atlas divides the important parts of a scene in files. In this case we have two formats for this first version:

* Scene format files (`.ascene`). These are JSON files that basically pack in it the information of the scene, such as the entities, components and values for those components.

* Material files (`.amat`). These are JSON files that pack the information of the materials, such as the textures, colors and other properties.

* Graphite Theme files (`.gtheme`). These are JSON files that pack the information of the Graphite themes, such as the colors, fonts and other properties.

* Terrain Generator files (`.atgen`). These are JSON files that pack the information of the terrain generators, such as the heightmap, textures and other properties.

* Biome files (`.abiome`). These are JSON files that pack the information of the biomes, such as the terrain generator, materials and other properties.

The idea is that the scene format files will reference the material files, so that we can reuse materials across different scenes. This way, we can have a more modular and efficient way of managing our assets.

## How do I understand a scene format file?
To understand a scene format file, start looking through the documentation at this directory and `/docs`, and then look at the code in this directory and `/tests`.

Each scene format file has these main sections:
* `name`: The name of the scene, which is a string that can be used to identify the scene.
* `id`: The ID of the scene, which is a string that can be used to identify the scene. The ID is used internally by the engine to reference the scene, while the name is used for display purposes and debugging.
* `objects`: An array of objects that are present in the scene, which can be of different types (solid, compound, model, particle emitter, terrain, etc.). Each object is defined as an object with a `type` property that specifies the type of the object, and other properties that define the values for that object.
* `lights`: An array of lights that are present in the scene, which can be of different types (point light, directional light, spotlight, etc.). Each light is defined as an object with a `type` property that specifies the type of the light, and other properties that define the values for that light.
* `camera`: An object that defines the properties of the camera in the scene, such as its position, rotation, field of view, etc. The camera is defined as an object with a `type` property that specifies the type of the camera (e.g., perspective, orthographic, etc.), and other properties that define the values for that camera.
* `property_syncs`: An array of persistent property bindings. Each entry contains a `target` endpoint and a `source` endpoint. Atlas resolves these bindings whenever the scene is loaded, after objects exist and before components are initialized, so physics and scripts receive the synchronized values from their first frame.

Property endpoints use `section` (`object`, `camera`, or `environment`) and a JSON-pointer `path`. Object endpoints also store the stable object reference, component type, and component index. The special `bounds` component exposes the rendered object size. For example, a rigidbody collider can follow its object's bounds:

```json
"property_syncs": [
    {
        "target": {
            "section": "object",
            "object": "Cube",
            "component": "rigidbody",
            "componentIndex": 0,
            "path": "/collider/size"
        },
        "source": {
            "section": "object",
            "object": "Cube",
            "component": "bounds",
            "componentIndex": -1,
            "path": ""
        }
    }
]
```
