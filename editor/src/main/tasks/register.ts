import { StartupTaskUpdate, TaskDefinition } from "src/shared/types/ipc";
import { startupTask } from "./startup";
import { ElectronTaskManager } from "../scheduler";
import { mainWindow } from "../main";
import { storeOnboarding } from "./store-onboarding";

const startupTaskDefinition: TaskDefinition<StartupTaskUpdate, void, []> = {
    name: "startup-task",
    concurrency: "join",
    run: startupTask,
};

const storeOnboardingTaskDefinition: TaskDefinition<
    StartupTaskUpdate,
    void,
    [string | null, string | null]
> = {
    name: "store-onboarding-data",
    concurrency: "join",
    run: storeOnboarding,
};

export const taskDefinitions = [
    startupTaskDefinition,
    storeOnboardingTaskDefinition,
];

export function registerTasks(taskManager: {
    register<TUpdate, TResult, TArgs extends unknown[]>(
        definition: TaskDefinition<TUpdate, TResult, TArgs>,
    ): void;
}) {
    taskManager.register(startupTaskDefinition);
    taskManager.register(storeOnboardingTaskDefinition);
}

export const tasks = new ElectronTaskManager(() => mainWindow);

export function registerAllTasks() {
    registerTasks(tasks);
}
