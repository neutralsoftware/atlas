import { mkdir, writeFile } from "node:fs/promises";
import path from "node:path";

const mode = process.env.APP_MODE ?? "development";
const debug = mode !== "release";

const now = new Date();
const yyyy = String(now.getFullYear());
const mm = String(now.getMonth() + 1).padStart(2, "0");
const dd = String(now.getDate()).padStart(2, "0");
const buildId = `${yyyy}${mm}${dd}`;

const content = `export const DEBUG = ${debug} as const;
export const BUILDID = "${buildId}" as const;
export const APP_MODE = "${mode}" as const;
`;

const outDir = path.resolve("src/shared/generated");
await mkdir(outDir, { recursive: true });
await writeFile(path.join(outDir, "build.ts"), content, "utf8");

console.log("Generated build constants:", { mode, debug, buildId });
