import { ipcMain, BrowserWindow } from "electron";
import { BUILDID, DEBUG } from "../shared/generated/build";
import { tasks } from "./tasks/register";
import { allWindows } from "./main";
import { makerRegistry } from "./windows";
import { WindowMaker } from "src/shared/types/ipc";

type OnboardingDataPayload = {
    runtimePath: string | null;
    executablePath: string | null;
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
}
