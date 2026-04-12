"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const electron_1 = require("electron");
const node_fs_1 = require("node:fs");
const node_path_1 = __importDefault(require("node:path"));
const build_1 = require("../shared/generated/build");
const ipc_1 = require("./ipc");
let mainWindow = null;
function getWindowIcon() {
    const packagedExt = process.platform === "win32" ? "ico" : "png";
    if (electron_1.app.isPackaged) {
        const packagedIconPath = node_path_1.default.join(process.resourcesPath, `icon.${packagedExt}`);
        return (0, node_fs_1.existsSync)(packagedIconPath) ? packagedIconPath : undefined;
    }
    const iconPath = process.platform === "win32"
        ? node_path_1.default.join(process.cwd(), build_1.DEBUG
            ? "build/icons/dev/icon.ico"
            : "build/icons/release/icon.ico")
        : node_path_1.default.join(process.cwd(), build_1.DEBUG
            ? "build/icons/dev/icon.png"
            : "build/icons/release/icon.png");
    return (0, node_fs_1.existsSync)(iconPath) ? iconPath : undefined;
}
function getDockIconPath() {
    if (process.platform !== "darwin") {
        return undefined;
    }
    if (electron_1.app.isPackaged) {
        const packagedDockIconPath = node_path_1.default.join(process.resourcesPath, "icon.icns");
        return (0, node_fs_1.existsSync)(packagedDockIconPath)
            ? packagedDockIconPath
            : undefined;
    }
    const devDockIconPath = node_path_1.default.join(process.cwd(), build_1.DEBUG ? "build/icons/dev/icon.png" : "build/icons/release/icon.png");
    return (0, node_fs_1.existsSync)(devDockIconPath) ? devDockIconPath : undefined;
}
function getRendererIndexPath() {
    return node_path_1.default.join(electron_1.app.getAppPath(), "dist", "renderer", "index.html");
}
function getPreloadPath() {
    return node_path_1.default.join(electron_1.app.getAppPath(), "dist-electron", "preload", "preload.js");
}
async function createMainWindow() {
    const windowIcon = getWindowIcon();
    const win = new electron_1.BrowserWindow({
        width: 1280,
        height: 800,
        minWidth: 960,
        minHeight: 640,
        show: true,
        ...(windowIcon ? { icon: windowIcon } : {}),
        titleBarStyle: process.platform === "darwin" ? "hiddenInset" : "default",
        webPreferences: {
            preload: getPreloadPath(),
            contextIsolation: true,
            nodeIntegration: false,
            sandbox: true,
        },
    });
    mainWindow = win;
    win.once("ready-to-show", () => {
        win.show();
    });
    win.on("closed", () => {
        if (mainWindow === win) {
            mainWindow = null;
        }
    });
    const devServerUrl = "http://localhost:5173/#/test";
    if (!electron_1.app.isPackaged && build_1.DEBUG) {
        try {
            await win.loadURL(devServerUrl);
            return win;
        }
        catch {
            // Fallback to built renderer when the dev server is unavailable.
        }
    }
    await win.loadFile(getRendererIndexPath(), { hash: "/test" });
    return win;
}
electron_1.app.whenReady().then(async () => {
    if (process.platform === "darwin") {
        const dockIconPath = getDockIconPath();
        if (dockIconPath) {
            try {
                electron_1.app.dock?.setIcon(dockIconPath);
            }
            catch {
                // Keep startup resilient when icon setup fails.
            }
        }
    }
    (0, ipc_1.registerIpcHandlers)();
    await createMainWindow();
    electron_1.app.on("activate", async () => {
        if (electron_1.BrowserWindow.getAllWindows().length === 0) {
            await createMainWindow();
        }
    });
});
electron_1.app.on("window-all-closed", () => {
    if (process.platform !== "darwin") {
        electron_1.app.quit();
    }
});
