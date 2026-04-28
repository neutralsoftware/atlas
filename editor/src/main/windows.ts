import { app, BrowserWindow, screen } from "electron";
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
import { currentProjectPath, windowInteractiveRegions } from "./ipc";

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
        backgroundColor: "#000000",
        title: "Atlas Editor - " + currentProjectPath?.split("/").pop(),
        frame: false,
        skipTaskbar: true,

        show: false,
        ...(windowIcon ? { icon: windowIcon } : {}),

        webPreferences: {
            preload: getPreloadPath(),
            contextIsolation: true,
            nodeIntegration: false,
            sandbox: true,
        },
    });

    function resizeEditorToWindow(window: BrowserWindow) {
        const [width, height] = window.getContentSize();
        const scale = window.webContents.getZoomFactor();
        engineBridge.resizeEditor(width, height, scale);
    }

    function scrollEditor(delta: number) {
        if (!Number.isFinite(delta) || Math.abs(delta) < 0.0001) {
            return;
        }
        try {
            engineBridge.editorScroll(delta, 1);
        } catch (err) {
            console.error("Failed to scroll editor viewport:", err);
        }
    }

    let overlayWindow: BrowserWindow | null = null;
    let overlayMouseTrackingTimer: NodeJS.Timeout | null = null;
    let overlayCapturesMouse = false;
    let syncingViewportToOverlay = false;

    function moveOverlayToFront() {
        if (!overlayWindow || overlayWindow.isDestroyed()) {
            return;
        }

        try {
            overlayWindow.moveTop();
        } catch {
            return;
        }
    }

    function syncViewportToOverlay() {
        if (!overlayWindow || overlayWindow.isDestroyed()) {
            return;
        }

        syncingViewportToOverlay = true;

        try {
            const bounds = overlayWindow.getContentBounds();
            win.setBounds({
                x: Number.isFinite(bounds.x) ? bounds.x : 0,
                y: Number.isFinite(bounds.y) ? bounds.y : 0,
                width:
                    Number.isFinite(bounds.width) && bounds.width > 0
                        ? bounds.width
                        : 1,
                height:
                    Number.isFinite(bounds.height) && bounds.height > 0
                        ? bounds.height
                        : 1,
            });

            if (overlayWindow.isVisible() && !overlayWindow.isMinimized()) {
                win.showInactive();
                moveOverlayToFront();
            } else if (win.isVisible()) {
                win.hide();
            }
        } finally {
            syncingViewportToOverlay = false;
        }
    }

    setMainWindow(win);
    allWindows.push({
        id: "editor",
        window: win,
    });

    win.on("focus", moveOverlayToFront);
    win.on("show", moveOverlayToFront);

    win.on("closed", () => {
        if (overlayWindow && !overlayWindow.isDestroyed()) {
            overlayWindow.close();
            overlayWindow = null;
        }
        if (overlayMouseTrackingTimer) {
            clearInterval(overlayMouseTrackingTimer);
            overlayMouseTrackingTimer = null;
        }
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

    win.webContents.on("before-mouse-event", (_event, input) => {
        const wheelInput = input as typeof input & {
            type?: string;
            deltaY?: number;
            wheelDeltaY?: number;
            wheelTicksY?: number;
            hasPreciseScrollingDeltas?: boolean;
        };
        if (wheelInput.type !== "mouseWheel") {
            return;
        }

        let rawDelta =
            typeof wheelInput.deltaY === "number"
                ? -wheelInput.deltaY
                : typeof wheelInput.wheelDeltaY === "number"
                  ? wheelInput.wheelDeltaY
                  : typeof wheelInput.wheelTicksY === "number"
                    ? wheelInput.wheelTicksY
                    : 0;
        if (wheelInput.hasPreciseScrollingDeltas) {
            rawDelta *= 0.75;
        }
        scrollEditor(rawDelta * 0.12);
    });

    const devServerUrl = "http://localhost:5173/#/editorSplash";
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
        await win.loadFile(getRendererIndexPath(), { hash: "/editorSplash" });
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

    overlayWindow = new BrowserWindow({
        width: 1200,
        height: 800,
        title: "Atlas Editor - " + currentProjectPath?.split("/").pop(),
        frame: true,
        titleBarStyle: "hiddenInset",
        acceptFirstMouse: true,
        transparent: true,
        backgroundColor: "#00000000",
        show: false,
        hasShadow: true,
        resizable: true,
        minimizable: true,
        maximizable: true,
        fullscreenable: false,
        webPreferences: {
            preload: getPreloadPath(),
            contextIsolation: true,
            nodeIntegration: false,
            sandbox: true,
            backgroundThrottling: false,
        },
    });

    overlayWindow.setContentBounds(win.getBounds());
    overlayWindow.setAlwaysOnTop(true, "normal");
    overlayWindow.setIgnoreMouseEvents(true, { forward: true });
    setMainWindow(overlayWindow);

    const syncOverlayMouseRouting = () => {
        if (!overlayWindow || overlayWindow.isDestroyed()) {
            return;
        }

        if (
            !overlayWindow.isVisible() ||
            overlayWindow.isMinimized()
        ) {
            if (overlayCapturesMouse) {
                overlayWindow.setIgnoreMouseEvents(true, { forward: true });
                overlayCapturesMouse = false;
            }
            return;
        }

        const cursor = screen.getCursorScreenPoint();
        const bounds = overlayWindow.getBounds();
        const contentBounds = overlayWindow.getContentBounds();
        const insideWindow =
            cursor.x >= bounds.x &&
            cursor.y >= bounds.y &&
            cursor.x < bounds.x + bounds.width &&
            cursor.y < bounds.y + bounds.height;

        if (!insideWindow) {
            if (overlayCapturesMouse) {
                overlayWindow.setIgnoreMouseEvents(true, { forward: true });
                overlayCapturesMouse = false;
            }
            return;
        }

        const contentLocalX = cursor.x - contentBounds.x;
        const contentLocalY = cursor.y - contentBounds.y;
        const interactiveRegions =
            windowInteractiveRegions.get(overlayWindow) ?? [];
        const chromeCaptureHeight =
            Math.max(0, contentBounds.y - bounds.y) + 56;
        const shouldCaptureMouse =
            cursor.y - bounds.y < chromeCaptureHeight ||
            interactiveRegions.some(
                (region) =>
                    contentLocalX >= region.x &&
                    contentLocalY >= region.y &&
                    contentLocalX < region.x + region.width &&
                    contentLocalY < region.y + region.height,
            );

        if (shouldCaptureMouse === overlayCapturesMouse) {
            return;
        }

        overlayWindow.setIgnoreMouseEvents(!shouldCaptureMouse, {
            forward: true,
        });
        overlayCapturesMouse = shouldCaptureMouse;
    };

    const overlayDevServerUrl = "http://localhost:5173/#/editorOverlay";
    let overlayLoaded = false;

    if (!app.isPackaged && DEBUG) {
        try {
            await overlayWindow.loadURL(overlayDevServerUrl);
            overlayLoaded = true;
        } catch {
            // Fallback to built renderer when the dev server is unavailable.
        }
    }

    if (!overlayLoaded) {
        await overlayWindow.loadFile(getRendererIndexPath(), {
            hash: "/editorOverlay",
        });
    }

    syncViewportToOverlay();
    if (!overlayWindow.isVisible()) {
        overlayWindow.show();
    }
    syncViewportToOverlay();

    overlayWindow.on("move", syncViewportToOverlay);
    overlayWindow.on("resize", syncViewportToOverlay);
    overlayWindow.on("show", syncViewportToOverlay);
    overlayWindow.on("restore", syncViewportToOverlay);
    overlayWindow.on("maximize", () => {
        syncViewportToOverlay();
    });
    overlayWindow.on("unmaximize", () => {
        syncViewportToOverlay();
    });
    overlayWindow.on("enter-full-screen", syncViewportToOverlay);
    overlayWindow.on("leave-full-screen", syncViewportToOverlay);
    overlayWindow.on("minimize", () => {
        win.hide();
    });
    overlayWindow.on("hide", () => {
        win.hide();
    });
    overlayWindow.on("closed", () => {
        const closedOverlay = overlayWindow;
        if (mainWindow === closedOverlay) {
            setMainWindow(null);
        }
        if (!win.isDestroyed()) {
            win.close();
        }
        overlayWindow = null;
    });

    overlayMouseTrackingTimer = setInterval(syncOverlayMouseRouting, 16);

    const targetEditorFps = 60;
    frameTimer = setInterval(() => {
        try {
            engineBridge.step();
        } catch (err) {
            console.error("Failed to step engine frame:", err);
        }
    }, 1000 / targetEditorFps);

    win.on("resize", () => {
        if (syncingViewportToOverlay) {
            resizeEditorToWindow(win);
            return;
        }

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
