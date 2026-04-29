import { ipcMain, BrowserWindow } from "electron";
import { BUILDID, DEBUG } from "../shared/generated/build";
import { tasks } from "./tasks/register";
import { allWindows, engineBridge } from "./main";
import { makerRegistry } from "./windows";
import { EditorControlMode, WindowMaker } from "src/shared/types/ipc";
import { createProject } from "./tasks/create-project";
import { getProjects } from "./tasks/startup";

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
            editorViewportBounds?.scale ?? win?.webContents.getZoomFactor() ?? 1,
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
}
