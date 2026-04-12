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

declare global {
  interface Window {
    app: WindowApi;
  }
}
