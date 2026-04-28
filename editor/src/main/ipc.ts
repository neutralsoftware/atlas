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

type InteractiveRegion = {
    x: number;
    y: number;
    width: number;
    height: number;
};

export let currentProjectPath: string | null = null;
export const windowInteractiveRegions = new WeakMap<
    BrowserWindow,
    InteractiveRegion[]
>();

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

    ipcMain.handle("window:set-mouse-passthrough", (event, ignore: boolean) => {
        const win = BrowserWindow.fromWebContents(event.sender);
        if (!win) {
            return;
        }

        if (ignore) {
            win.setIgnoreMouseEvents(true, { forward: true });
            return;
        }

        win.setIgnoreMouseEvents(false);
    });

    ipcMain.handle(
        "window:set-interactive-regions",
        (
            event,
            regions: Array<{
                x: number;
                y: number;
                width: number;
                height: number;
            }>,
        ) => {
            const win = BrowserWindow.fromWebContents(event.sender);
            if (!win) {
                return;
            }

            const sanitizedRegions = Array.isArray(regions)
                ? regions
                      .map((region) => ({
                          x: Number.isFinite(region?.x) ? region.x : 0,
                          y: Number.isFinite(region?.y) ? region.y : 0,
                          width:
                              Number.isFinite(region?.width) &&
                              region.width > 0
                                  ? region.width
                                  : 0,
                          height:
                              Number.isFinite(region?.height) &&
                              region.height > 0
                                  ? region.height
                                  : 0,
                      }))
                      .filter(
                          (region) => region.width > 0 && region.height > 0,
                      )
                : [];

            windowInteractiveRegions.set(win, sanitizedRegions);
        },
    );

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
}
