import { app, BrowserWindow } from "electron";
import { existsSync } from "node:fs";
import path from "node:path";
import { DEBUG } from "../shared/generated/build";
import { registerIpcHandlers } from "./ipc";
import { registerAllTasks } from "./tasks/register";
import { WindowHandle } from "src/shared/types/ipc";

export let mainWindow: BrowserWindow | null = null;
export const allWindows: WindowHandle[] = [];

export function setMainWindow(win: BrowserWindow | null) {
    mainWindow = win;
}

export function getWindowIcon() {
    const packagedExt = process.platform === "win32" ? "ico" : "png";

    if (app.isPackaged) {
        const packagedIconPath = path.join(
            process.resourcesPath,
            `icon.${packagedExt}`,
        );
        return existsSync(packagedIconPath) ? packagedIconPath : undefined;
    }

    const iconPath =
        process.platform === "win32"
            ? path.join(
                  process.cwd(),
                  DEBUG
                      ? "build/icons/dev/icon.ico"
                      : "build/icons/release/icon.ico",
              )
            : path.join(
                  process.cwd(),
                  DEBUG
                      ? "build/icons/dev/icon.png"
                      : "build/icons/release/icon.png",
              );

    return existsSync(iconPath) ? iconPath : undefined;
}

function getDockIconPath() {
    if (process.platform !== "darwin") {
        return undefined;
    }

    if (app.isPackaged) {
        const packagedDockIconPath = path.join(
            process.resourcesPath,
            "icon.icns",
        );
        return existsSync(packagedDockIconPath)
            ? packagedDockIconPath
            : undefined;
    }

    const devDockIconPath = path.join(
        process.cwd(),
        DEBUG ? "build/icons/dev/icon.png" : "build/icons/release/icon.png",
    );

    return existsSync(devDockIconPath) ? devDockIconPath : undefined;
}

export function getRendererIndexPath() {
    return path.join(app.getAppPath(), "dist", "renderer", "index.html");
}

export function getPreloadPath() {
    return path.join(
        app.getAppPath(),
        "dist-electron",
        "preload",
        "preload.js",
    );
}

async function createMainWindow() {
    const windowIcon = getWindowIcon();

    const win = new BrowserWindow({
        width: 1080,
        height: 720,

        resizable: false,
        minimizable: false,
        maximizable: false,
        fullscreenable: false,

        frame: false,
        hasShadow: true,
        transparent: true,

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

    mainWindow = win;
    allWindows.push({
        id: "splash",
        window: win,
    });

    win.once("ready-to-show", () => {
        win.show();
    });

    win.on("closed", () => {
        if (mainWindow === win) {
            mainWindow = null;
        }
    });

    const devServerUrl = "http://localhost:5173/#/splash";

    if (!app.isPackaged && DEBUG) {
        try {
            await win.loadURL(devServerUrl);
            return win;
        } catch {
            // Fallback to built renderer when the dev server is unavailable.
        }
    }

    await win.loadFile(getRendererIndexPath(), { hash: "/splash" });

    return win;
}

app.whenReady().then(async () => {
    if (process.platform === "darwin") {
        const dockIconPath = getDockIconPath();
        if (dockIconPath) {
            try {
                app.dock?.setIcon(dockIconPath);
            } catch {
                // Keep startup resilient when icon setup fails.
            }
        }
    }

    registerIpcHandlers();
    registerAllTasks();

    await createMainWindow();

    app.on("activate", async () => {
        if (BrowserWindow.getAllWindows().length === 0) {
            await createMainWindow();
        }
    });
});

app.on("window-all-closed", () => {
    if (process.platform !== "darwin") {
        app.quit();
    }
});
