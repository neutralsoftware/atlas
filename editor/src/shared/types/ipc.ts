import { Project } from "./atlas";

export type AppPlatform =
    | "aix"
    | "darwin"
    | "freebsd"
    | "linux"
    | "openbsd"
    | "sunos"
    | "win32"
    | "cygwin"
    | "netbsd";

export interface AppInfo {
    debug: boolean;
    buildId: string;
    platform: AppPlatform;
}

export interface FileDialogOptions {
    title?: string;
    buttonLabel?: string;
    defaultPath?: string;
    filters?: Array<{
        name: string;
        extensions: string[];
    }>;
    properties?: Array<
        | "openFile"
        | "openDirectory"
        | "multiSelections"
        | "showHiddenFiles"
        | "createDirectory"
        | "promptToCreate"
        | "noResolveAliases"
        | "treatPackageAsDirectory"
        | "dontAddToRecent"
    >;
}

export interface WindowApi {
    getAppInfo(): Promise<AppInfo>;
    setTitle(title: string): Promise<void>;
    minimize(): Promise<void>;
    toggleMaximize(): Promise<void>;
    close(): Promise<void>;
    onThemeChanged(callback: (theme: "light" | "dark") => void): () => void;
    showWindow(id: string): void;
    hideWindow(id: string): void;
    destroyWindow(id: string): void;
    fileDialog(options: FileDialogOptions): Promise<string[] | undefined>;
    storeOnboardingData(onBoardingData: {
        runtimePath: string | null;
        executablePath: string | null;
    }): Promise<void>;
}

export type CreateProjectStyle = "pbr" | "pathtracing" | "pbr-gi";
export type EditorControlMode = "none" | "move" | "rotate" | "scale";

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

export interface GeneralTask {
    getProjects(): Promise<Project[]>;
    createProject(payload: {
        name: string;
        location: string;
        style: CreateProjectStyle;
    }): Promise<Project>;
    openProject(payload: { path: string }): Promise<void>;
    getCurrentProject(): Promise<Project | null>;
}

export interface EditorControlsApi {
    setEnabled(enabled: boolean): Promise<void>;
    setPlaying(playing: boolean): Promise<void>;
    setMode(mode: EditorControlMode): Promise<void>;
    getSelection(): Promise<{ id: number; name: string }>;
    saveCurrentScene(): Promise<boolean>;
}

export interface EditorInputApi {
    pointer(payload: {
        action: 0 | 1 | 2;
        x: number;
        y: number;
        button: number;
        scale?: number;
    }): Promise<void>;
    scroll(delta: number, scale?: number): Promise<void>;
    key(key: 0 | 1 | 2 | 3 | 4 | 5, pressed: boolean): Promise<void>;
    setViewportBounds(bounds: {
        x: number;
        y: number;
        width: number;
        height: number;
        scale?: number;
    }): Promise<void>;
}

export type WindowHandle<TWindow = unknown> = {
    id: string;
    window: TWindow;
};

export type WindowMaker<TWindow = unknown> = () => Promise<
    WindowHandle<TWindow>
>;

declare global {
    interface Window {
        app: WindowApi;
        startupTask: StartupTask;
        tasks: GeneralTask;
        editorControls: EditorControlsApi;
        editorInput: EditorInputApi;
    }
}
