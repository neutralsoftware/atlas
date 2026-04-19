import { app, BrowserWindow } from "electron";
import { WindowMaker } from "src/shared/types/ipc";
import {
    allWindows,
    getPreloadPath,
    getRendererIndexPath,
    getWindowIcon,
    mainWindow,
    setMainWindow,
} from "./main";
import { DEBUG } from "../shared/generated/build";

export const createOnboardingWindow: WindowMaker<BrowserWindow> = async () => {
    const windowIcon = getWindowIcon();

    const win = new BrowserWindow({
        width: 557,
        height: 557,

        resizable: false,
        minimizable: false,
        maximizable: false,
        fullscreenable: false,

        frame: false,
        hasShadow: true,

        alwaysOnTop: true,

        center: true,
        skipTaskbar: true,

        show: true,
        ...(windowIcon ? { icon: windowIcon } : {}),

        webPreferences: {
            preload: getPreloadPath(),
            contextIsolation: true,
            nodeIntegration: false,
            sandbox: true,
        },
    });

    setMainWindow(win);
    allWindows.push({
        id: "onboarding",
        window: win,
    });

    win.once("ready-to-show", () => {
        win.show();
    });

    win.on("closed", () => {
        if (mainWindow === win) {
            setMainWindow(null);
        }
    });

    const devServerUrl = "http://localhost:5173/#/onboarding";

    if (!app.isPackaged && DEBUG) {
        try {
            await win.loadURL(devServerUrl);
            return { id: "onboarding", window: win };
        } catch {
            // Fallback to built renderer when the dev server is unavailable.
        }
    }

    await win.loadFile(getRendererIndexPath(), { hash: "/onboarding" });

    return { id: "onboarding", window: win };
};

export const createProjectsWindow: WindowMaker<BrowserWindow> = async () => {
    const windowIcon = getWindowIcon();

    const win = new BrowserWindow({
        width: 1000,
        height: 557,

        resizable: true,
        minimizable: true,
        maximizable: true,
        fullscreenable: false,

        frame: true,
        titleBarStyle: "hiddenInset",

        hasShadow: true,

        center: true,

        show: true,
        ...(windowIcon ? { icon: windowIcon } : {}),

        webPreferences: {
            preload: getPreloadPath(),
            contextIsolation: true,
            nodeIntegration: false,
            sandbox: true,
        },
    });

    setMainWindow(win);
    allWindows.push({
        id: "projects",
        window: win,
    });

    win.once("ready-to-show", () => {
        win.show();
    });

    win.on("closed", () => {
        if (mainWindow === win) {
            setMainWindow(null);
        }
    });

    const devServerUrl = "http://localhost:5173/#/projects";

    if (!app.isPackaged && DEBUG) {
        try {
            await win.loadURL(devServerUrl);
            return { id: "projects", window: win };
        } catch {
            // Fallback to built renderer when the dev server is unavailable.
        }
    }

    await win.loadFile(getRendererIndexPath(), { hash: "/projects" });

    return { id: "projects", window: win };
};

export const createNewProjectModal: WindowMaker<BrowserWindow> = async () => {
    const windowIcon = getWindowIcon();

    const win = new BrowserWindow({
        width: 600,
        height: 557,
        parent: mainWindow!,
        modal: true,

        frame: true,
        titleBarStyle: "default",

        hasShadow: true,

        center: true,

        show: true,
        ...(windowIcon ? { icon: windowIcon } : {}),

        webPreferences: {
            preload: getPreloadPath(),
            contextIsolation: true,
            nodeIntegration: false,
            sandbox: true,
        },
    });

    setMainWindow(win);
    allWindows.push({
        id: "createProject",
        window: win,
    });

    win.once("ready-to-show", () => {
        win.show();
    });

    win.on("closed", () => {
        if (mainWindow === win) {
            setMainWindow(null);
        }
    });

    const devServerUrl = "http://localhost:5173/#/createProject";

    if (!app.isPackaged && DEBUG) {
        try {
            await win.loadURL(devServerUrl);
            return { id: "createProject", window: win };
        } catch {
            // Fallback to built renderer when the dev server is unavailable.
        }
    }

    await win.loadFile(getRendererIndexPath(), { hash: "/createProject" });

    return { id: "createProject", window: win };
};

export const makerRegistry: Record<string, WindowMaker<BrowserWindow>> = {
    onboarding: createOnboardingWindow,
    projects: createProjectsWindow,
    createProject: createNewProjectModal,
};
