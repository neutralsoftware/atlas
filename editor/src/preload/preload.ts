import { contextBridge, ipcRenderer } from "electron";
import type {
    EditorControlsApi,
    EditorInputApi,
    GeneralTask,
    StartupTask,
    StartupTaskUpdate,
    WindowApi,
} from "../shared/types/ipc";
import type { Scene } from "src/shared/types/atlas";

const api: WindowApi = {
    getAppInfo: () => ipcRenderer.invoke("app:get-info"),
    setTitle: (title) => ipcRenderer.invoke("window:set-title", title),
    minimize: () => ipcRenderer.invoke("window:minimize"),
    toggleMaximize: () => ipcRenderer.invoke("window:toggle-maximize"),
    close: () => ipcRenderer.invoke("window:close"),
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
    getCurrentProject: () => ipcRenderer.invoke("general:get-current-project"),
    getObjects: (): Promise<Scene> => ipcRenderer.invoke("general:get-objects"),
};

const editorControls: EditorControlsApi = {
    setEnabled: (enabled) =>
        ipcRenderer.invoke("editor-controls:set-enabled", enabled),
    setPlaying: (playing) =>
        ipcRenderer.invoke("editor-controls:set-playing", playing),
    setMode: (mode) => ipcRenderer.invoke("editor-controls:set-mode", mode),
    getSelection: () => ipcRenderer.invoke("editor-controls:get-selection"),
    getSceneObjects: () =>
        ipcRenderer.invoke("editor-controls:get-scene-objects"),
    selectObject: (id, focus = false) =>
        ipcRenderer.invoke("editor-controls:select-object", id, focus),
    renameObject: (id, name) =>
        ipcRenderer.invoke("editor-controls:rename-object", id, name),
    createObject: (type, name = "") =>
        ipcRenderer.invoke("editor-controls:create-object", type, name),
    showObjectMenu: (payload) =>
        ipcRenderer.invoke("editor-controls:show-object-menu", payload),
    saveCurrentScene: () =>
        ipcRenderer.invoke("editor-controls:save-current-scene"),
};

const editorInput: EditorInputApi = {
    pointer: (payload) => {
        ipcRenderer.send("editor-input:pointer", payload);
        return Promise.resolve();
    },
    scroll: (delta, scale) => {
        ipcRenderer.send("editor-input:scroll", delta, scale);
        return Promise.resolve();
    },
    key: (key, pressed) => {
        ipcRenderer.send("editor-input:key", key, pressed);
        return Promise.resolve();
    },
    setViewportBounds: (bounds) => {
        ipcRenderer.send("editor-input:set-viewport-bounds", bounds);
        return Promise.resolve();
    },
};

contextBridge.exposeInMainWorld("app", api);
contextBridge.exposeInMainWorld("startupTask", startupTask);
contextBridge.exposeInMainWorld("tasks", generalTasks);
contextBridge.exposeInMainWorld("editorControls", editorControls);
contextBridge.exposeInMainWorld("editorInput", editorInput);
