import { TaskDefinition } from "src/shared/types/ipc";
import { startupTask } from "./startup";
import { ElectronTaskManager } from "../scheduler";
import { mainWindow } from "../main";

export const taskDefinitions: TaskDefinition<unknown, unknown>[] = [
    {
        name: "startup-task",
        concurrency: "join",
        run: startupTask,
    },
];

export function registerTasks(taskManager: {
    register<TUpdate, TResult>(
        definition: TaskDefinition<TUpdate, TResult>,
    ): void;
}) {
    for (const def of taskDefinitions) {
        taskManager.register(def);
    }
}

export const tasks = new ElectronTaskManager(() => mainWindow);

export function registerAllTasks() {
    registerTasks(tasks);
}
