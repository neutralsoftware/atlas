export interface AppInfo {
    debug: boolean;
    buildId: string;
    platform: NodeJS.Platform;
}

export interface WindowApi {
    getAppInfo(): Promise<AppInfo>;
    setTitle(title: string): Promise<void>;
    onThemeChanged(callback: (theme: "light" | "dark") => void): () => void;
}

export type StartupTaskUpdate =
    | "starting"
    | "locating-config-file"
    | "config-file-found"
    | "checking-executable"
    | "loading-runtimelib"
    | "needs-config"
    | "done"
    | { type: "error"; error: string };

export type TaskConcurrency = "join" | "skip" | "parallel" | "queue";

export interface TaskContext<TUpdate = unknown> {
    emit(update: TUpdate): void;
    signal: AbortSignal;
    runId: string;
}

export type TaskRunner<TUpdate = unknown, TResult = void> = (
    ctx: TaskContext<TUpdate>,
) => Promise<TResult>;

export interface TaskDefinition<TUpdate = unknown, TResult = void> {
    name: string;
    concurrency: TaskConcurrency;
    run: TaskRunner<TUpdate, TResult>;
}

export interface StartupTask {
    onUpdate(callback: (update: StartupTaskUpdate) => void): () => void;
    start(): Promise<void>;
}

declare global {
    interface Window {
        app: WindowApi;
        startupTask: StartupTask;
    }
}
