import {
    ipcMain,
    BrowserWindow,
    Menu,
    MenuItemConstructorOptions,
    shell,
} from "electron";
import path from "node:path";
import { BUILDID, DEBUG } from "../shared/generated/build";
import { tasks } from "./tasks/register";
import { allWindows, engineBridge, mainWindow } from "./main";
import { makerRegistry } from "./windows";
import {
    ContextMenuItem,
    EditorControlMode,
    FileSystemCreateKind,
    WindowMaker,
} from "src/shared/types/ipc";
import { createProject } from "./tasks/create-project";
import { getProjects, runtimeLib } from "./tasks/startup";

type OnboardingDataPayload = {
    runtimePath: string | null;
    executablePath: string | null;
};

export let currentProjectPath: string | null = null;

type EditorViewportBounds = {
    x: number;
    y: number;
    width: number;
    height: number;
    scale: number;
};

type ObjectMenuResult =
    | { action: "create"; type: string }
    | { action: "rename" }
    | { action: "select" }
    | { action: "unparent" }
    | { action: "delete" }
    | null;

let editorViewportBounds: EditorViewportBounds | null = null;

const editorControlModes: Record<EditorControlMode, number> = {
    none: 0,
    move: 1,
    rotate: 2,
    scale: 3,
};

function positiveNumber(value: unknown, fallback: number) {
    return typeof value === "number" && Number.isFinite(value) && value > 0
        ? value
        : fallback;
}

function finiteNumber(value: unknown, fallback: number) {
    return typeof value === "number" && Number.isFinite(value)
        ? value
        : fallback;
}

function sanitizeViewportBounds(
    bounds: unknown,
    fallbackWidth = 1,
    fallbackHeight = 1,
    fallbackScale = 1,
): EditorViewportBounds {
    const candidate =
        bounds && typeof bounds === "object"
            ? (bounds as Partial<EditorViewportBounds>)
            : {};

    return {
        x: Math.round(finiteNumber(candidate.x, 0)),
        y: Math.round(finiteNumber(candidate.y, 0)),
        width: Math.round(positiveNumber(candidate.width, fallbackWidth)),
        height: Math.round(positiveNumber(candidate.height, fallbackHeight)),
        scale: positiveNumber(candidate.scale, fallbackScale),
    };
}

function emptyScene() {
    return { name: "Scene", objects: [], selectedId: -1 };
}

function requireProjectPath() {
    if (!currentProjectPath) {
        throw new Error("No project is open");
    }
    return path.resolve(currentProjectPath);
}

function projectPath(targetPath: string) {
    if (typeof targetPath !== "string" || targetPath.trim().length === 0) {
        throw new Error("Invalid path");
    }

    const projectRoot = requireProjectPath();
    const resolvedPath = path.resolve(targetPath);
    const relativePath = path.relative(projectRoot, resolvedPath);
    if (
        relativePath === ".." ||
        relativePath.startsWith(`..${path.sep}`) ||
        path.isAbsolute(relativePath)
    ) {
        throw new Error("Path is outside the current project");
    }

    return resolvedPath;
}

function entryName(name: string) {
    const value = String(name ?? "").trim();
    if (
        !value ||
        value === "." ||
        value === ".." ||
        value.includes("/") ||
        value.includes("\\") ||
        value.includes("\0")
    ) {
        throw new Error("Invalid name");
    }
    return value;
}

function ensureExtension(name: string, extension: string) {
    return path.extname(name) ? name : `${name}${extension}`;
}

function createEntryName(kind: FileSystemCreateKind, name: string) {
    const value = entryName(name);
    switch (kind) {
        case "scene":
            return ensureExtension(value, ".ascene");
        case "script":
            return ensureExtension(value, ".ts");
        case "material":
            return ensureExtension(value, ".amat");
        case "folder":
            return value;
    }
}

function sceneTemplate(name: string) {
    const stem = path.basename(name, path.extname(name));
    const id = stem
        .toLowerCase()
        .replace(/[^a-z0-9]+/g, "_")
        .replace(/^_+|_+$/g, "");

    return `${JSON.stringify(
        {
            name: stem,
            id: id || "scene",
            objects: [],
            lights: [{ type: "ambient", intensity: 0.2 }],
            camera: {
                position: [0.0, 0.0, -5.0],
                target: [0.0, 0.0, 0.0],
                fov: 60.0,
            },
            targets: [
                {
                    name: "Main Target",
                    type: "multisampled",
                    render: true,
                    display: true,
                },
            ],
            environment: {
                automaticAmbient: true,
                atmosphereSky: true,
            },
        },
        null,
        4,
    )}\n`;
}

function scriptTemplate(name: string) {
    const stem = path.basename(name, path.extname(name));
    const className =
        stem
            .replace(/[^a-zA-Z0-9]+/g, " ")
            .trim()
            .split(/\s+/)
            .map((part) => part.charAt(0).toUpperCase() + part.slice(1))
            .join("") || "Script";

    return `export class ${className} {\n    start() {\n    }\n\n    update() {\n    }\n}\n`;
}

function materialTemplate() {
    return `${JSON.stringify(
        {
            material: {
                albedo: [1.0, 1.0, 1.0, 1.0],
                metallic: 0.0,
                roughness: 0.5,
                ao: 1.0,
            },
        },
        null,
        4,
    )}\n`;
}

async function uniqueDestination(directory: string, sourcePath: string) {
    const { stat } = await import("fs/promises");
    const parsed = path.parse(path.basename(sourcePath));
    let candidate = path.join(directory, path.basename(sourcePath));
    let index = 2;

    while (true) {
        try {
            await stat(candidate);
            candidate = path.join(
                directory,
                `${parsed.name} ${index}${parsed.ext}`,
            );
            index += 1;
        } catch {
            return candidate;
        }
    }
}

async function writeProjectMainScene(scenePath: string) {
    const { readFile, writeFile } = await import("fs/promises");
    const projectRoot = requireProjectPath();
    const projectFile = path.join(projectRoot, "project.atlas");
    const relativeScenePath = path
        .relative(projectRoot, scenePath)
        .split(path.sep)
        .join("/");
    const content = await readFile(projectFile, "utf-8");
    const line = `main_scene = ${JSON.stringify(relativeScenePath)}`;

    if (/^main_scene\s*=.*$/m.test(content)) {
        await writeFile(projectFile, content.replace(/^main_scene\s*=.*$/m, line));
        return;
    }

    if (/^\[game\]\s*$/m.test(content)) {
        await writeFile(
            projectFile,
            content.replace(/^\[game\]\s*$/m, `[game]\n${line}`),
        );
        return;
    }

    await writeFile(projectFile, `${content.trimEnd()}\n\n[game]\n${line}\n`);
}

function getRuntimeSceneObjects() {
    const raw =
        typeof engineBridge.getSceneObjects === "function"
            ? engineBridge.getSceneObjects()
            : "";
    if (typeof raw !== "string" || raw.length === 0) {
        return emptyScene();
    }
    try {
        return JSON.parse(raw);
    } catch {
        return emptyScene();
    }
}

export function applyEditorViewportBounds(
    fallbackWidth = 1,
    fallbackHeight = 1,
    fallbackScale = 1,
) {
    const bounds =
        editorViewportBounds ??
        sanitizeViewportBounds(
            undefined,
            fallbackWidth,
            fallbackHeight,
            fallbackScale,
        );

    engineBridge.resizeEditor(
        bounds.x,
        bounds.y,
        bounds.width,
        bounds.height,
        bounds.scale,
    );
}

export function clearEditorViewportBounds() {
    editorViewportBounds = null;
}

function toElectronMenuItem(
    item: ContextMenuItem,
    sender: Electron.WebContents,
    select?: (action: string) => void,
): MenuItemConstructorOptions {
    if (item.kind === "separator") {
        return { type: "separator" };
    }

    if (item.kind === "submenu") {
        return {
            label: item.label,
            submenu: item.children.map((child) =>
                toElectronMenuItem(child, sender, select),
            ),
        };
    }

    return {
        label: item.label,
        enabled: item.enabled ?? true,
        click: () => {
            select?.(item.action);
            sender.send("context-menu:clicked", item.action);
        },
    };
}

export function registerIpcHandlers() {
    ipcMain.handle("app:get-info", () => {
        return {
            debug: DEBUG,
            buildId: BUILDID,
            platform: process.platform,
        };
    });

    ipcMain.handle("window:set-title", (event, title: string) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        win?.setTitle(title);
    });

    ipcMain.handle("window:minimize", (event) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        win?.minimize();
    });

    ipcMain.handle("window:toggle-maximize", (event) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        if (!win) {
            return;
        }

        if (win.isMaximized()) {
            win.unmaximize();
            return;
        }

        win.maximize();
    });

    ipcMain.handle("window:close", (event) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        win?.close();
    });

    ipcMain.handle("startup-task:start", () => {
        return tasks.start("startup-task");
    });

    ipcMain.handle(
        "store-onboarding-data",
        (_event, payload: OnboardingDataPayload) => {
            return tasks.start(
                "store-onboarding-data",
                payload.runtimePath,
                payload.executablePath,
            );
        },
    );

    ipcMain.on("window:show", (event, eventId: string) => {
        for (const { id, window } of allWindows) {
            if (eventId === id && !window.isDestroyed()) {
                window.show();
                return;
            }
        }

        if (eventId in makerRegistry) {
            (makerRegistry[eventId] as WindowMaker<BrowserWindow>)();
        }
    });

    ipcMain.on("window:hide", (event, eventId: string) => {
        for (const { id, window } of allWindows) {
            if (eventId === id && !window.isDestroyed()) {
                window.hide();
                return;
            }
        }
    });

    ipcMain.on("window:destroy", (event, eventId: string) => {
        for (const { id, window } of allWindows) {
            if (eventId === id && !window.isDestroyed()) {
                window.close();
                return;
            }
        }
    });

    ipcMain.handle("file-dialog", async (event, options) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        if (!win) {
            throw new Error("No window found for file dialog");
        }

        const { dialog } = await import("electron");
        const result = await dialog.showOpenDialog(win, options);
        return result.canceled ? undefined : result.filePaths;
    });

    ipcMain.handle("general:get-projects", async () => {
        return getProjects();
    });

    ipcMain.handle("general:create-project", async (_event, payload) => {
        return createProject(payload);
    });

    ipcMain.handle("general:open-project", async (_event, payload) => {
        currentProjectPath = payload.path;
    });

    ipcMain.handle("editor-controls:set-enabled", async (_event, enabled) => {
        engineBridge.setEditorControlsEnabled(Boolean(enabled));
    });

    ipcMain.handle("editor-controls:set-playing", async (_event, playing) => {
        engineBridge.setEditorSimulationEnabled(Boolean(playing));
    });

    ipcMain.handle("editor-controls:set-mode", async (_event, mode) => {
        const numericMode =
            editorControlModes[mode as EditorControlMode] ??
            editorControlModes.none;
        engineBridge.setEditorControlMode(numericMode);
    });

    ipcMain.handle("editor-controls:get-selection", async () => {
        return {
            id: engineBridge.getSelectedObjectId(),
            name: engineBridge.getSelectedObjectName(),
        };
    });

    ipcMain.handle("editor-controls:get-scene-objects", async () => {
        return getRuntimeSceneObjects();
    });

    ipcMain.handle(
        "editor-controls:select-object",
        async (_event, id, focus) => {
            return Boolean(
                engineBridge.selectObject(Number(id), Boolean(focus)),
            );
        },
    );

    ipcMain.handle(
        "editor-controls:rename-object",
        async (_event, id, name) => {
            if (typeof name !== "string") {
                return false;
            }
            return Boolean(engineBridge.renameObject(Number(id), name));
        },
    );

    ipcMain.handle(
        "editor-controls:set-object-parent",
        async (_event, childId, parentId) => {
            return Boolean(
                engineBridge.setObjectParent(
                    Number(childId),
                    parentId === null ? -1 : Number(parentId),
                ),
            );
        },
    );

    ipcMain.handle("editor-controls:delete-object", async (_event, id) => {
        return Boolean(engineBridge.deleteObject(Number(id)));
    });

    ipcMain.handle(
        "editor-controls:create-object",
        async (_event, type, name) => {
            if (typeof type !== "string") {
                return -1;
            }
            return Number(engineBridge.createObject(type, String(name ?? "")));
        },
    );

    ipcMain.handle(
        "editor-controls:show-object-menu",
        async (event, payload) => {
            const win = BrowserWindow.fromWebContents(event.sender);
            if (!win) {
                return null;
            }

            const hasObject =
                payload &&
                typeof payload === "object" &&
                typeof payload.id === "number" &&
                Number.isFinite(payload.id);

            return new Promise<ObjectMenuResult>((resolve) => {
                let settled = false;
                const finish = (result: ObjectMenuResult) => {
                    if (!settled) {
                        settled = true;
                        resolve(result);
                    }
                };

                const menu = Menu.buildFromTemplate([
                    {
                        label: "Add New Object",
                        submenu: [
                            {
                                label: "Cube",
                                click: () =>
                                    finish({ action: "create", type: "cube" }),
                            },
                            {
                                label: "Sphere",
                                click: () =>
                                    finish({
                                        action: "create",
                                        type: "sphere",
                                    }),
                            },
                            {
                                label: "Plane",
                                click: () =>
                                    finish({ action: "create", type: "plane" }),
                            },
                            {
                                label: "Pyramid",
                                click: () =>
                                    finish({
                                        action: "create",
                                        type: "pyramid",
                                    }),
                            },
                            {
                                label: "Capsule",
                                click: () =>
                                    finish({
                                        action: "create",
                                        type: "capsule",
                                    }),
                            },
                            { type: "separator" },
                            {
                                label: "Camera",
                                click: () =>
                                    finish({
                                        action: "create",
                                        type: "camera",
                                    }),
                            },
                            {
                                label: "Group",
                                click: () =>
                                    finish({
                                        action: "create",
                                        type: "group",
                                    }),
                            },
                            { type: "separator" },
                            {
                                label: "Lights",
                                submenu: [
                                    {
                                        label: "Point Light",
                                        click: () =>
                                            finish({
                                                action: "create",
                                                type: "pointLight",
                                            }),
                                    },
                                    {
                                        label: "Spot Light",
                                        click: () =>
                                            finish({
                                                action: "create",
                                                type: "spotLight",
                                            }),
                                    },
                                    {
                                        label: "Directional Light",
                                        click: () =>
                                            finish({
                                                action: "create",
                                                type: "directionalLight",
                                            }),
                                    },
                                    {
                                        label: "Area Light",
                                        click: () =>
                                            finish({
                                                action: "create",
                                                type: "areaLight",
                                            }),
                                    },
                                    {
                                        label: "Ambient Light",
                                        click: () =>
                                            finish({
                                                action: "create",
                                                type: "ambientLight",
                                            }),
                                    },
                                ],
                            },
                            {
                                label: "Effects",
                                submenu: [
                                    {
                                        label: "Particle Emitter",
                                        click: () =>
                                            finish({
                                                action: "create",
                                                type: "particleEmitter",
                                            }),
                                    },
                                    {
                                        label: "Terrain / Landscape",
                                        click: () =>
                                            finish({
                                                action: "create",
                                                type: "terrain",
                                            }),
                                    },
                                ],
                            },
                        ],
                    },
                    { type: "separator" },
                    {
                        label: "Rename",
                        enabled: hasObject,
                        click: () => finish({ action: "rename" }),
                    },
                    {
                        label: "Select and Frame",
                        enabled: hasObject,
                        click: () => finish({ action: "select" }),
                    },
                    {
                        label: "Clear Parent",
                        enabled: hasObject,
                        click: () => finish({ action: "unparent" }),
                    },
                    { type: "separator" },
                    {
                        label: "Delete",
                        enabled: hasObject,
                        click: () => finish({ action: "delete" }),
                    },
                ]);

                menu.popup({ window: win, callback: () => finish(null) });
            });
        },
    );

    ipcMain.handle("editor-controls:save-current-scene", async () => {
        return Boolean(engineBridge.saveCurrentScene());
    });

    ipcMain.on("editor-input:pointer", (event, payload) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        if (!win || !payload) {
            return;
        }

        const [contentWidthRaw, contentHeightRaw] = win.getContentSize();
        const contentWidth = positiveNumber(contentWidthRaw, 1);
        const contentHeight = positiveNumber(contentHeightRaw, 1);
        const viewportWidth = editorViewportBounds?.width ?? contentWidth;
        const viewportHeight = editorViewportBounds?.height ?? contentHeight;
        const scale = positiveNumber(
            payload.scale,
            editorViewportBounds?.scale ?? win.webContents.getZoomFactor(),
        );
        const x =
            typeof payload.x === "number" && Number.isFinite(payload.x)
                ? Math.max(0, Math.min(payload.x, viewportWidth))
                : 0;
        const y =
            typeof payload.y === "number" && Number.isFinite(payload.y)
                ? Math.max(0, Math.min(payload.y, viewportHeight))
                : 0;
        const action = Number.isFinite(payload.action) ? payload.action : 1;
        const button = Number.isFinite(payload.button) ? payload.button : 1;

        engineBridge.editorPointer(
            action,
            x,
            Math.max(0, viewportHeight - y),
            button,
            scale,
        );
    });

    ipcMain.on("editor-input:scroll", (event, delta, scale) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        const effectiveScale = positiveNumber(
            scale,
            editorViewportBounds?.scale ??
                win?.webContents.getZoomFactor() ??
                1,
        );

        engineBridge.editorScroll(delta, effectiveScale);
    });

    ipcMain.on("editor-input:set-viewport-bounds", (event, bounds) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        const [contentWidthRaw, contentHeightRaw] = win?.getContentSize() ?? [
            1, 1,
        ];
        editorViewportBounds = sanitizeViewportBounds(
            bounds,
            positiveNumber(contentWidthRaw, 1),
            positiveNumber(contentHeightRaw, 1),
            win?.webContents.getZoomFactor() ?? 1,
        );
        applyEditorViewportBounds();
    });

    ipcMain.on("editor-input:key", (_event, key, pressed) => {
        const numericKey = Number(key);
        if (!Number.isInteger(numericKey) || numericKey < 0 || numericKey > 5) {
            return;
        }

        engineBridge.editorKey(numericKey, Boolean(pressed));
    });

    ipcMain.handle("general:get-current-project", async () => {
        if (!currentProjectPath) {
            return null;
        }

        const projects = await getProjects();
        return (
            projects.find((project) => project.path === currentProjectPath) ??
            null
        );
    });

    ipcMain.handle("general:get-objects", async () => {
        return getRuntimeSceneObjects();
    });

    ipcMain.handle(
        "general:get-directory-information",
        async (_event, payload) => {
            const { readdir } = await import("fs/promises");
            const { extname } = await import("path");
            const directoryPath = projectPath(payload.path);

            const entries = await readdir(directoryPath, {
                withFileTypes: true,
            });

            return {
                name: directoryPath,
                type: "directory",
                children: entries
                    .filter((entry) => !entry.name.startsWith("."))
                    .sort((a, b) => {
                        if (a.isDirectory() !== b.isDirectory()) {
                            return a.isDirectory() ? -1 : 1;
                        }
                        return a.name.localeCompare(b.name);
                    })
                    .map((entry) => {
                        if (entry.isDirectory()) {
                            return {
                                name: entry.name,
                                type: "directory",
                                children: [],
                            };
                        }

                        return {
                            name: entry.name,
                            extension: extname(entry.name),
                            type: "file",
                        };
                    }),
            };
        },
    );

    ipcMain.handle("filesystem:create-entry", async (_event, payload) => {
        const { mkdir, writeFile } = await import("fs/promises");
        const directory = projectPath(payload.directory);
        const kind = payload.kind as FileSystemCreateKind;
        if (!["scene", "script", "material", "folder"].includes(kind)) {
            throw new Error("Invalid entry type");
        }

        const name = createEntryName(kind, payload.name);
        const destination = path.join(directory, name);
        projectPath(destination);

        if (kind === "folder") {
            await mkdir(destination);
        } else if (kind === "scene") {
            await writeFile(destination, sceneTemplate(name), { flag: "wx" });
        } else if (kind === "script") {
            await writeFile(destination, scriptTemplate(name), { flag: "wx" });
        } else {
            await writeFile(destination, materialTemplate(), { flag: "wx" });
        }

        return { path: destination, name };
    });

    ipcMain.handle("filesystem:rename-entry", async (_event, payload) => {
        const { rename } = await import("fs/promises");
        const source = projectPath(payload.path);
        if (source === requireProjectPath()) {
            throw new Error("Cannot rename the project root");
        }

        const name = entryName(payload.name);
        const destination = path.join(path.dirname(source), name);
        projectPath(destination);
        await rename(source, destination);
        return { path: destination, name };
    });

    ipcMain.handle("filesystem:delete-entry", async (_event, payload) => {
        const target = projectPath(payload.path);
        if (target === requireProjectPath()) {
            throw new Error("Cannot delete the project root");
        }
        await shell.trashItem(target);
        return true;
    });

    ipcMain.handle("filesystem:copy-external-entries", async (_event, payload) => {
        const { cp } = await import("fs/promises");
        const targetDirectory = projectPath(payload.targetDirectory);
        const sources = Array.isArray(payload.sources) ? payload.sources : [];
        const copied = [];

        for (const sourceRaw of sources) {
            if (typeof sourceRaw !== "string" || !sourceRaw) {
                continue;
            }

            const source = path.resolve(sourceRaw);
            const destination = await uniqueDestination(targetDirectory, source);
            projectPath(destination);
            await cp(source, destination, { recursive: true, errorOnExist: true });
            copied.push({ path: destination, name: path.basename(destination) });
        }

        return copied;
    });

    ipcMain.handle("filesystem:move-entry", async (_event, payload) => {
        const { lstat, rename } = await import("fs/promises");
        const source = projectPath(payload.source);
        const targetDirectory = projectPath(payload.targetDirectory);
        if (source === requireProjectPath()) {
            throw new Error("Cannot move the project root");
        }
        if (path.dirname(source) === targetDirectory) {
            return { path: source, name: path.basename(source) };
        }

        const sourceStats = await lstat(source);
        const relativeTarget = path.relative(source, targetDirectory);
        if (
            sourceStats.isDirectory() &&
            (relativeTarget === "" || !relativeTarget.startsWith(".."))
        ) {
            throw new Error("Cannot move a folder into itself");
        }

        const destination = await uniqueDestination(targetDirectory, source);
        projectPath(destination);
        await rename(source, destination);
        return { path: destination, name: path.basename(destination) };
    });

    ipcMain.handle("filesystem:reveal-in-finder", async (_event, payload) => {
        shell.showItemInFolder(projectPath(payload.path));
        return true;
    });

    ipcMain.handle("filesystem:open-scene", async (_event, payload) => {
        const scenePath = projectPath(payload.path);
        if (path.extname(scenePath).toLowerCase() !== ".ascene") {
            return false;
        }

        await writeProjectMainScene(scenePath);
        if (!runtimeLib || !mainWindow || mainWindow.isDestroyed()) {
            return true;
        }

        engineBridge.shutdown();
        engineBridge.loadLibrary(runtimeLib);
        engineBridge.attachToNativeWindow(
            path.join(requireProjectPath(), "project.atlas"),
            mainWindow.getNativeWindowHandle(),
        );
        applyEditorViewportBounds();
        return true;
    });

    ipcMain.handle(
        "context-menu:show",
        async (event, items: ContextMenuItem[]) => {
            const win = BrowserWindow.fromWebContents(event.sender);
            if (win == null) {
                return null;
            }

            return new Promise<string | null>((resolve) => {
                let settled = false;
                const finish = (action: string | null) => {
                    if (!settled) {
                        settled = true;
                        resolve(action);
                    }
                };

                const menu = Menu.buildFromTemplate(
                    items.map((item) =>
                        toElectronMenuItem(item, event.sender, finish),
                    ),
                );

                menu.popup({
                    window: win,
                    callback: () => finish(null),
                });
            });
        },
    );
}
