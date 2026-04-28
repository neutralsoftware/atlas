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

        frame: true,
        titleBarStyle: "hiddenInset",

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
    let syncingOverlayFromViewport = false;
    let syncingViewportFromOverlay = false;
    let overlayMouseTrackingTimer: NodeJS.Timeout | null = null;
    let overlayCapturesMouse = false;

    function getViewportContentBounds() {
        const bounds = win.getContentBounds();

        return {
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
        };
    }

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

    function syncOverlayToViewport() {
        if (
            !overlayWindow ||
            overlayWindow.isDestroyed() ||
            syncingViewportFromOverlay
        ) {
            return;
        }

        syncingOverlayFromViewport = true;

        try {
            overlayWindow.setBounds(getViewportContentBounds());

            if (win.isVisible() && !win.isMinimized()) {
                overlayWindow.showInactive();
                moveOverlayToFront();
            } else if (overlayWindow.isVisible()) {
                overlayWindow.hide();
            }
        } finally {
            syncingOverlayFromViewport = false;
        }
    }

    function syncViewportToOverlay() {
        if (
            !overlayWindow ||
            overlayWindow.isDestroyed() ||
            syncingOverlayFromViewport
        ) {
            return;
        }

        syncingViewportFromOverlay = true;

        try {
            if (win.isMaximized()) {
                win.unmaximize();
            }
            win.setContentBounds(overlayWindow.getBounds());
        } finally {
            syncingViewportFromOverlay = false;
        }
    }

    setMainWindow(win);
    allWindows.push({
        id: "editor",
        window: win,
    });

    win.once("ready-to-show", () => {
        win.show();
    });

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

    if (!win.isVisible()) {
        win.show();
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
        parent: win,
        width: 1200,
        height: 800,
        frame: false,
        acceptFirstMouse: true,
        transparent: true,
        backgroundColor: "#00000000",
        show: false,
        hasShadow: false,
        resizable: true,
        minimizable: false,
        maximizable: true,
        fullscreenable: false,
        skipTaskbar: true,
        webPreferences: {
            preload: getPreloadPath(),
            contextIsolation: true,
            nodeIntegration: false,
            sandbox: true,
            backgroundThrottling: false,
        },
    });

    overlayWindow.setIgnoreMouseEvents(true, { forward: true });
    syncOverlayToViewport();

    const syncOverlayMouseRouting = () => {
        if (!overlayWindow || overlayWindow.isDestroyed()) {
            return;
        }

        if (
            !overlayWindow.isVisible() ||
            win.isMinimized() ||
            !win.isVisible()
        ) {
            if (overlayCapturesMouse) {
                overlayWindow.setIgnoreMouseEvents(true, { forward: true });
                overlayCapturesMouse = false;
            }
            return;
        }

        const cursor = screen.getCursorScreenPoint();
        const bounds = overlayWindow.getBounds();
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

        const localX = cursor.x - bounds.x;
        const localY = cursor.y - bounds.y;
        const interactiveRegions =
            windowInteractiveRegions.get(overlayWindow) ?? [];
        const shouldCaptureMouse = interactiveRegions.some(
            (region) =>
                localX >= region.x &&
                localY >= region.y &&
                localX < region.x + region.width &&
                localY < region.y + region.height,
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

    overlayWindow.once("ready-to-show", () => {
        syncOverlayToViewport();
    });

    syncOverlayToViewport();

    win.on("resize", syncOverlayToViewport);
    win.on("move", syncOverlayToViewport);
    win.on("show", syncOverlayToViewport);
    win.on("restore", syncOverlayToViewport);
    win.on("maximize", syncOverlayToViewport);
    win.on("unmaximize", syncOverlayToViewport);
    win.on("enter-full-screen", syncOverlayToViewport);
    win.on("leave-full-screen", syncOverlayToViewport);
    win.on("minimize", () => {
        overlayWindow?.hide();
    });
    win.on("hide", () => {
        overlayWindow?.hide();
    });

    overlayWindow.on("move", syncViewportToOverlay);
    overlayWindow.on("resize", syncViewportToOverlay);
    overlayWindow.on("maximize", () => {
        if (syncingOverlayFromViewport) {
            return;
        }

        syncingViewportFromOverlay = true;

        try {
            win.maximize();
        } finally {
            syncingViewportFromOverlay = false;
        }

        syncOverlayToViewport();
    });
    overlayWindow.on("unmaximize", () => {
        if (syncingOverlayFromViewport) {
            return;
        }

        syncingViewportFromOverlay = true;

        try {
            if (win.isMaximized()) {
                win.unmaximize();
            }
        } finally {
            syncingViewportFromOverlay = false;
        }

        syncOverlayToViewport();
    });
    overlayWindow.on("closed", () => {
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
