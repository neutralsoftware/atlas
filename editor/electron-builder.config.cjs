// eslint-disable-next-line @typescript-eslint/no-require-imports, no-undef
const path = require("node:path");

// eslint-disable-next-line no-undef
const mode = process.env.APP_MODE ?? "development";
const isRelease = mode === "release";

// eslint-disable-next-line no-undef
module.exports = {
    appId: isRelease ? "org.atlas.editor" : "org.atlas.editor.dev",
    productName: isRelease
        ? "Atlas Engine"
        : "Atlas Engine (Development Build)",

    directories: {
        output: isRelease ? "release" : "release-dev",
    },

    files: ["dist/**/*", "dist-electron/**/*", "build/Release/engine_bridge.node", "package.json"],

    mac: {
        icon: path.resolve(
            isRelease
                ? "build/icons/release/icon.icns"
                : "build/icons/dev/icon.icns",
        ),
        identity: "-",
    },

    dmg: {
        icon: path.resolve(
            isRelease
                ? "build/icons/release/icon.icns"
                : "build/icons/dev/icon.icns",
        ),
    },

    win: {
        target: "nsis",
        icon: path.resolve(
            isRelease
                ? "build/icons/release/icon.ico"
                : "build/icons/dev/icon.ico",
        ),
    },

    linux: {
        target: ["AppImage"],
        icon: path.resolve(
            isRelease
                ? "build/icons/release/icon.png"
                : "build/icons/dev/icon.png",
        ),
    },
};
