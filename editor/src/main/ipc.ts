import { ipcMain, BrowserWindow } from "electron";
import { BUILDID, DEBUG } from "../shared/generated/build";

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
}
