import { app, BrowserWindow } from "electron";
import { WindowMaker } from "src/shared/types/ipc";
import {
    allWindows,
    engineBridge,
    getPreloadPath,
    getRendererIndexPath,
    getWindowIcon,
    mainWindow,
    setMainWindow,
} from "./main";
import { DEBUG } from "../shared/generated/build";
import { runtimeLib } from "./tasks/startup";
import { currentProjectPath } from "./ipc";

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
        width: 560,
        height: 680,
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

export let frameTimer: NodeJS.Timeout | null = null;

export const viewport: WindowMaker<BrowserWindow> = async () => {
    const windowIcon = getWindowIcon();

    const win = new BrowserWindow({
        width: 1200,
        height: 800,
        backgroundColor: "#00000000",
        title: "Atlas Editor - " + currentProjectPath?.split("/").pop(),
        frame: false,
        transparent: true,
        hasShadow: true,

        show: false,
        ...(windowIcon ? { icon: windowIcon } : {}),

        webPreferences: {
            preload: getPreloadPath(),
            contextIsolation: true,
            nodeIntegration: false,
            sandbox: true,
            backgroundThrottling: false,
        },
    });

    function resizeEditorToWindow(window: BrowserWindow) {
        const [width, height] = window.getContentSize();
        const scale = window.webContents.getZoomFactor();
        engineBridge.resizeEditor(width, height, scale);
    }

    setMainWindow(win);
    allWindows.push({
        id: "editor",
        window: win,
    });

    win.on("closed", () => {
        if (frameTimer) {
            clearInterval(frameTimer);
            frameTimer = null;
        }
        try {
            engineBridge.shutdown();
        } catch (err) {
            console.error("Error during engine shutdown:", err);
        }
        if (mainWindow === win) {
            setMainWindow(null);
        }
    });

    const devServerUrl = "http://localhost:5173/#/editorOverlay";
    let rendererLoaded = false;

    if (!app.isPackaged && DEBUG) {
        try {
            await win.loadURL(devServerUrl);
            rendererLoaded = true;
        } catch {
            // Fallback to built renderer when the dev server is unavailable.
        }
    }

    if (!rendererLoaded) {
        await win.loadFile(getRendererIndexPath(), { hash: "/editorOverlay" });
    }

    await new Promise<void>((resolve) => setTimeout(resolve, 50));

    const dylibPath = runtimeLib as string;
    try {
        engineBridge.loadLibrary(dylibPath);
    } catch (err) {
        console.error("Failed to load engine library:", err);
        throw err;
    }

    const nativeHandle: Buffer = win.getNativeWindowHandle();
    try {
        engineBridge.attachToNativeWindow(
            currentProjectPath + "/project.atlas",
            nativeHandle,
        );
    } catch (err) {
        console.error("Failed to attach to native window:", err);
        throw err;
    }

    resizeEditorToWindow(win);
    win.show();

    const targetEditorFps = 60;
    frameTimer = setInterval(() => {
        try {
            engineBridge.step();
        } catch (err) {
            console.error("Failed to step engine frame:", err);
        }
    }, 1000 / targetEditorFps);

    win.on("resize", () => {
        resizeEditorToWindow(win);
        try {
            engineBridge.step();
        } catch (err) {
            console.error("Failed to step engine frame after resize:", err);
        }
    });

    return { id: "editor", window: win };
};

export const makerRegistry: Record<string, WindowMaker<BrowserWindow>> = {
    onboarding: createOnboardingWindow,
    projects: createProjectsWindow,
    createProject: createNewProjectModal,
    editor: viewport,
};
