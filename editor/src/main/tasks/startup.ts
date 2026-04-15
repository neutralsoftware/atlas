import { app } from "electron";
import path from "node:path";
import { readFile, stat } from "node:fs/promises";
import type { StartupTaskUpdate, TaskContext } from "../../shared/types/ipc";
import { VERSION_ID } from "../main";

export let runtimeLib: string | null = null;
export let atlasExecutablePath: string | null = null;

export async function startupTask(
    ctx: TaskContext<StartupTaskUpdate>,
): Promise<void> {
    ctx.emit("starting");
    ctx.emit("locating-config-file");

    const configFilePath = path.join(
        app.getPath("home"),
        ".atlas",
        "config.json",
    );

    let configContent: string;
    try {
        configContent = await readFile(configFilePath, "utf-8");
    } catch {
        ctx.emit("needs-config");
        return;
    }

    ctx.emit("config-file-found");

    const config = JSON.parse(configContent)[VERSION_ID].onboardingData;

    if (!config.atlasExecutablePath) {
        ctx.emit("needs-config");
        return;
    }

    if (!config.runtimeLib) {
        ctx.emit("needs-config");
        return;
    }

    atlasExecutablePath = config.atlasExecutablePath;
    runtimeLib = config.runtimeLib;

    ctx.emit("checking-executable");

    try {
        await stat(atlasExecutablePath as string);

        // Check if
    } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        ctx.emit({
            type: "error",
            error: `Failed to locate atlas executable at path "${atlasExecutablePath}": ${message}`,
        });
        return;
    }

    if (ctx.signal.aborted) return;

    ctx.emit("loading-runtimelib");

    try {
        await stat(runtimeLib as string);
    } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        ctx.emit({
            type: "error",
            error: `Failed to locate runtimelib at path "${runtimeLib}": ${message}`,
        });
        return;
    }

    if (ctx.signal.aborted) return;

    ctx.emit("done");
}
