export interface AppInfo {
    debug: boolean;
    buildId: string;
    platform: NodeJS.Platform;
}

export interface WindowApi {
    getAppInfo(): Promise<AppInfo>;
    setTitle(title: string): Promise<void>;
    onThemeChanged(callback: (theme: "light" | "dark") => void): () => void;
    showWindow(id: string): void;
    hideWindow(id: string): void;
    destroyWindow(id: string): void;
    fileDialog(
        options: Electron.OpenDialogOptions,
    ): Promise<string[] | undefined>;
    storeOnboardingData(onBoardingData: {
        runtimePath: string | null;
        executablePath: string | null;
    }): Promise<void>;
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

export type TaskRunner<
    TUpdate = unknown,
    TResult = void,
    TArgs extends unknown[] = [],
> = (ctx: TaskContext<TUpdate>, ...args: TArgs) => Promise<TResult>;

export interface TaskDefinition<
    TUpdate = unknown,
    TResult = void,
    TArgs extends unknown[] = [],
> {
    name: string;
    concurrency: TaskConcurrency;
    run: TaskRunner<TUpdate, TResult, TArgs>;
}

export interface StartupTask {
    onUpdate(callback: (update: StartupTaskUpdate) => void): () => void;
    start(): Promise<void>;
}

export type WindowHandle = {
    id: string;
    window: Electron.BrowserWindow;
};

export type WindowMaker = () => Promise<WindowHandle>;

declare global {
    interface Window {
        app: WindowApi;
        startupTask: StartupTask;
    }
}
