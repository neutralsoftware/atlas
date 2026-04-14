import { app } from "electron";
import { readFile, mkdir, writeFile } from "node:fs/promises";
import path from "node:path";
import { StartupTaskUpdate, TaskContext } from "src/shared/types/ipc";
import { VERSION_ID } from "../main";

export async function storeOnboarding(
    ctx: TaskContext<StartupTaskUpdate>,
    runtimePath: string | null,
    executablePath: string | null,
): Promise<void> {
    const configFilePath = path.join(
        app.getPath("home"),
        ".atlas",
        "config.json",
    );

    const configDir = path.dirname(configFilePath);

    const onboardingData = {
        atlasExecutablePath: executablePath,
        runtimeLib: runtimePath,
    };

    try {
        await mkdir(configDir, { recursive: true });
    } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        ctx.emit({
            type: "error",
            error: `Failed to create config directory: ${message}`,
        });
        return;
    }

    let jsonContent: Record<string, unknown> = {};

    try {
        const configContent = await readFile(configFilePath, "utf-8");
        jsonContent = JSON.parse(configContent) as Record<string, unknown>;
    } catch (err) {
        const nodeErr = err as NodeJS.ErrnoException;
        if (nodeErr.code !== "ENOENT") {
            const message = err instanceof Error ? err.message : String(err);
            ctx.emit({
                type: "error",
                error: `Failed to read config file: ${message}`,
            });
            return;
        }
    }

    jsonContent[VERSION_ID] = {
        onboardingData,
    };

    try {
        await writeFile(
            configFilePath,
            JSON.stringify(jsonContent, null, 2),
            "utf-8",
        );
    } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        ctx.emit({
            type: "error",
            error: `Failed to write onboarding data: ${message}`,
        });
    }
}
