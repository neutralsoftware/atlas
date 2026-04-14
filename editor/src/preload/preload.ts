import { contextBridge, ipcRenderer } from "electron";
import type {
    StartupTask,
    StartupTaskUpdate,
    WindowApi,
} from "../shared/types/ipc";

const api: WindowApi = {
    getAppInfo: () => ipcRenderer.invoke("app:get-info"),
    setTitle: (title) => ipcRenderer.invoke("window:set-title", title),
    onThemeChanged: (callback) => {
        const listener = (_event: unknown, theme: "light" | "dark") =>
            callback(theme);
        ipcRenderer.on("theme:changed", listener);

        return () => {
            ipcRenderer.removeListener("theme:changed", listener);
        };
    },
    showWindow: (id) => ipcRenderer.send("window:show", id),
    hideWindow: (id) => ipcRenderer.send("window:hide", id),
    destroyWindow: (id) => ipcRenderer.send("window:destroy", id),
};

const startupTask: StartupTask = {
    start: () => ipcRenderer.invoke("startup-task:start"),

    onUpdate: (callback) => {
        const listener = (
            _event: Electron.IpcRendererEvent,
            payload: { runId: string; update: StartupTaskUpdate },
        ) => {
            callback(payload.update);
        };

        ipcRenderer.on("startup-task:update", listener);

        return () => {
            ipcRenderer.removeListener("startup-task:update", listener);
        };
    },
};

contextBridge.exposeInMainWorld("app", api);
contextBridge.exposeInMainWorld("startupTask", startupTask);
