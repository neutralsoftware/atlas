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

type WindowDragState = {
    startScreenX: number;
    startScreenY: number;
    startWindowX: number;
    startWindowY: number;
};

export let currentProjectPath: string | null = null;
const windowDragStates = new WeakMap<BrowserWindow, WindowDragState>();

const editorControlModes: Record<EditorControlMode, number> = {
    none: 0,
    move: 1,
    rotate: 2,
    scale: 3,
};

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

    ipcMain.on("window:start-drag", (event, screenX, screenY) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        if (!win || win.isMaximized() || win.isFullScreen()) {
            return;
        }

        const bounds = win.getBounds();
        windowDragStates.set(win, {
            startScreenX:
                typeof screenX === "number" && Number.isFinite(screenX)
                    ? screenX
                    : bounds.x,
            startScreenY:
                typeof screenY === "number" && Number.isFinite(screenY)
                    ? screenY
                    : bounds.y,
            startWindowX: bounds.x,
            startWindowY: bounds.y,
        });
    });

    ipcMain.on("window:drag", (event, screenX, screenY) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        if (!win) {
            return;
        }

        const dragState = windowDragStates.get(win);
        if (!dragState) {
            return;
        }

        const nextScreenX =
            typeof screenX === "number" && Number.isFinite(screenX)
                ? screenX
                : dragState.startScreenX;
        const nextScreenY =
            typeof screenY === "number" && Number.isFinite(screenY)
                ? screenY
                : dragState.startScreenY;

        win.setPosition(
            Math.round(
                dragState.startWindowX + nextScreenX - dragState.startScreenX,
            ),
            Math.round(
                dragState.startWindowY + nextScreenY - dragState.startScreenY,
            ),
        );
    });

    ipcMain.on("window:end-drag", (event) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        if (win) {
            windowDragStates.delete(win);
        }
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

    ipcMain.handle("editor-input:pointer", async (event, payload) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        if (!win || !payload) {
            return;
        }

        const [contentWidthRaw, contentHeightRaw] = win.getContentSize();
        const contentWidth =
            typeof contentWidthRaw === "number" &&
            Number.isFinite(contentWidthRaw) &&
            contentWidthRaw > 0
                ? contentWidthRaw
                : 1;
        const contentHeight =
            typeof contentHeightRaw === "number" &&
            Number.isFinite(contentHeightRaw) &&
            contentHeightRaw > 0
                ? contentHeightRaw
                : 1;
        const scale =
            typeof payload.scale === "number" && payload.scale > 0
                ? payload.scale
                : win.webContents.getZoomFactor();
        const x =
            typeof payload.x === "number" && Number.isFinite(payload.x)
                ? Math.max(0, Math.min(payload.x, contentWidth))
                : 0;
        const y =
            typeof payload.y === "number" && Number.isFinite(payload.y)
                ? Math.max(0, Math.min(payload.y, contentHeight))
                : 0;
        const action = Number.isFinite(payload.action) ? payload.action : 1;
        const button = Number.isFinite(payload.button) ? payload.button : 1;

        engineBridge.editorPointer(
            action,
            x,
            Math.max(0, contentHeight - y),
            button,
            scale,
        );
    });

    ipcMain.handle("editor-input:scroll", async (event, delta, scale) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        const effectiveScale =
            typeof scale === "number" && scale > 0
                ? scale
                : (win?.webContents.getZoomFactor() ?? 1);

        engineBridge.editorScroll(delta, effectiveScale);
    });
}
