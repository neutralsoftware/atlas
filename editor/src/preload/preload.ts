import { contextBridge, ipcRenderer } from "electron";
import type {
    GeneralTask,
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
    fileDialog: (options) => ipcRenderer.invoke("file-dialog", options),
    storeOnboardingData: (onBoardingData) =>
        ipcRenderer.invoke("store-onboarding-data", onBoardingData),
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

const generalTasks: GeneralTask = {
    getProjects: () => ipcRenderer.invoke("general:get-projects"),
    createProject: (payload) =>
        ipcRenderer.invoke("general:create-project", payload),
    openProject: (payload) =>
        ipcRenderer.invoke("general:open-project", payload),
};

contextBridge.exposeInMainWorld("app", api);
contextBridge.exposeInMainWorld("startupTask", startupTask);
contextBridge.exposeInMainWorld("tasks", generalTasks);
