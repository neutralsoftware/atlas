import { app } from "electron";
import { randomUUID } from "node:crypto";
import { execFile } from "node:child_process";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import path from "node:path";
import { promisify } from "node:util";
import type { Project } from "src/shared/types/atlas";
import type { CreateProjectStyle } from "src/shared/types/ipc";
import { VERSION_ID } from "../main";
import { atlasExecutablePath } from "./startup";

const execFileAsync = promisify(execFile);

type CreateProjectPayload = {
    name: string;
    location: string;
    style: CreateProjectStyle;
};

function getConfigFilePath() {
    return path.join(app.getPath("home"), ".atlas", "config.json");
}

function getVersionEntry(
    config: Record<string, unknown>,
): Record<string, unknown> {
    const entry = config[VERSION_ID];

    if (entry && typeof entry === "object" && !Array.isArray(entry)) {
        return entry as Record<string, unknown>;
    }

    return {};
}

function getConfiguredExecutablePath(versionEntry: Record<string, unknown>) {
    if (atlasExecutablePath) {
        return atlasExecutablePath;
    }

    const onboardingData = versionEntry.onboardingData;
    if (
        onboardingData &&
        typeof onboardingData === "object" &&
        !Array.isArray(onboardingData)
    ) {
        const executablePath = (
            onboardingData as Record<string, unknown>
        ).atlasExecutablePath;

        if (typeof executablePath === "string" && executablePath.trim()) {
            return executablePath;
        }
    }

    return null;
}

function toStoredProjects(versionEntry: Record<string, unknown>) {
    return Array.isArray(versionEntry.projects) ? versionEntry.projects : [];
}

function toProject(project: {
    id: string;
    name: string;
    path: string;
    starred: boolean;
    modified: string;
}): Project {
    return {
        ...project,
        modified: new Date(project.modified),
    };
}

function getCreateArgs(
    projectPath: string,
    name: string,
    style: CreateProjectStyle,
) {
    const args = [
        "create",
        name,
        "--path",
        projectPath,
        "--version",
        VERSION_ID,
    ];

    if (style === "pathtracing") {
        args.push("--renderer", "pathtracing");
        return args;
    }

    args.push("--renderer", "deferred");

    if (style === "pbr-gi") {
        args.push("--global-illumination");
    }

    return args;
}

function getFailureMessage(error: unknown) {
    if (error && typeof error === "object") {
        const stderr = "stderr" in error ? error.stderr : undefined;
        if (typeof stderr === "string" && stderr.trim()) {
            return stderr.trim();
        }

        const message = "message" in error ? error.message : undefined;
        if (typeof message === "string" && message.trim()) {
            return message.trim();
        }
    }

    return "Failed to create project.";
}

export async function createProject(
    payload: CreateProjectPayload,
): Promise<Project> {
    const name = payload.name.trim();
    const location = payload.location.trim();

    if (!name) {
        throw new Error("Project name cannot be empty.");
    }

    if (!location) {
        throw new Error("Project location cannot be empty.");
    }

    const configFilePath = getConfigFilePath();
    const configDir = path.dirname(configFilePath);

    await mkdir(configDir, { recursive: true });

    let config: Record<string, unknown> = {};

    try {
        const configContent = await readFile(configFilePath, "utf-8");
        config = JSON.parse(configContent) as Record<string, unknown>;
    } catch (error) {
        const nodeError = error as NodeJS.ErrnoException;
        if (nodeError.code !== "ENOENT") {
            throw error;
        }
    }

    const versionEntry = getVersionEntry(config);
    const executablePath = getConfiguredExecutablePath(versionEntry);

    if (!executablePath) {
        throw new Error("Atlas executable is not configured.");
    }

    const projectPath = path.join(location, name);

    try {
        await execFileAsync(
            executablePath,
            getCreateArgs(projectPath, name, payload.style),
            {
                windowsHide: true,
            },
        );
    } catch (error) {
        throw new Error(getFailureMessage(error), { cause: error });
    }

    const storedProject = {
        id: randomUUID(),
        name,
        path: projectPath,
        starred: false,
        modified: new Date().toISOString(),
    };

    const existingProjects = toStoredProjects(versionEntry).filter((project) => {
        if (!project || typeof project !== "object" || Array.isArray(project)) {
            return true;
        }

        return (project as Record<string, unknown>).path !== projectPath;
    });

    config[VERSION_ID] = {
        ...versionEntry,
        projects: [storedProject, ...existingProjects],
    };

    await writeFile(configFilePath, JSON.stringify(config, null, 2), "utf-8");

    return toProject(storedProject);
}
