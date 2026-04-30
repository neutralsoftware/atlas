import { Scene } from "src/shared/types/atlas";

export function getScene(): Scene {
    return {
        name: "MyTestScene",
        objects: [
            {
                name: "Camera",
                type: "camera",
                viewportId: 0,
            },
            {
                name: "Cube",
                type: "gameObject",
                viewportId: 1,
            },
        ],
    };
}
