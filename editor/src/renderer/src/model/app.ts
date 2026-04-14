export type AppInfo = {
    debug: boolean;
    buildId: string;
    platform: string;
};

export const onboardingData = {
    runtimePath: null as string | null,
    executablePath: null as string | null,
};

export function setOnboardingData(runtimePath: string, executablePath: string) {
    onboardingData.runtimePath = runtimePath;
    onboardingData.executablePath = executablePath;
}
