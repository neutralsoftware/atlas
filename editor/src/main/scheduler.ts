import { randomUUID } from "node:crypto";
import { TaskDefinition } from "src/shared/types/ipc";
import { BrowserWindow } from "electron";

type RunningTask<TResult = unknown> = {
    promise: Promise<TResult>;
    controller: AbortController;
    runId: string;
};

type QueuedJob<TResult = unknown> = {
    resolve: (value: TResult | PromiseLike<TResult>) => void;
    reject: (reason?: unknown) => void;
};

type StoredTaskDefinition = TaskDefinition<unknown, unknown>;
type StoredRunningTask = RunningTask<unknown>;
type StoredQueuedJob = QueuedJob<unknown>;

export class TaskManager {
    private definitions = new Map<string, StoredTaskDefinition>();
    private running = new Map<string, StoredRunningTask>();
    private queues = new Map<string, StoredQueuedJob[]>();

    register<TUpdate, TResult>(definition: TaskDefinition<TUpdate, TResult>) {
        if (this.definitions.has(definition.name)) {
            throw new Error(`Task "${definition.name}" is already registered`);
        }
        this.definitions.set(definition.name, definition);
    }

    start<TResult = void>(name: string): Promise<TResult | undefined> {
        const definition = this.definitions.get(name);
        if (!definition) {
            throw new Error(`Unknown task "${name}"`);
        }

        const current = this.running.get(name);

        switch (definition.concurrency) {
            case "join": {
                if (current) {
                    return current.promise as Promise<TResult>;
                }
                return this.startFresh(
                    name,
                    definition as TaskDefinition<unknown, TResult>,
                );
            }

            case "skip": {
                if (current) {
                    return Promise.resolve(undefined);
                }
                return this.startFresh(
                    name,
                    definition as TaskDefinition<unknown, TResult>,
                );
            }

            case "parallel": {
                return this.startDetached(
                    name,
                    definition as TaskDefinition<unknown, TResult>,
                );
            }

            case "queue": {
                if (!current) {
                    return this.startFresh(
                        name,
                        definition as TaskDefinition<unknown, TResult>,
                    );
                }

                return new Promise<TResult>((resolve, reject) => {
                    const queue = this.queues.get(name) ?? [];
                    const queuedJob: StoredQueuedJob = {
                        resolve: (value) => {
                            resolve(value as TResult);
                        },
                        reject,
                    };
                    queue.push(queuedJob);
                    this.queues.set(name, queue);
                });
            }
        }
    }

    isRunning(name: string): boolean {
        return this.running.has(name);
    }

    cancel(name: string): boolean {
        const current = this.running.get(name);
        if (!current) return false;
        current.controller.abort();
        return true;
    }

    private startFresh<TResult>(
        name: string,
        definition: TaskDefinition<unknown, TResult>,
    ): Promise<TResult> {
        const controller = new AbortController();
        const runId = randomUUID();

        const promise = (async () => {
            try {
                return await definition.run({
                    runId,
                    signal: controller.signal,
                    emit: (update) => {
                        this.emit(name, runId, update);
                    },
                });
            } finally {
                const stillCurrent = this.running.get(name);
                if (stillCurrent?.runId === runId) {
                    this.running.delete(name);
                    void this.drainQueue(name, definition);
                }
            }
        })();

        this.running.set(name, {
            promise,
            controller,
            runId,
        });

        return promise;
    }

    private async drainQueue<TResult>(
        name: string,
        definition: TaskDefinition<unknown, TResult>,
    ) {
        const queue = this.queues.get(name);
        if (!queue?.length) return;

        const next = queue.shift();
        if (!next) return;

        if (queue.length === 0) {
            this.queues.delete(name);
        }

        try {
            const result = await this.startFresh(name, definition);
            next.resolve(result);
        } catch (err) {
            next.reject(err);
        }
    }

    private async startDetached<TResult>(
        name: string,
        definition: TaskDefinition<unknown, TResult>,
    ): Promise<TResult> {
        const controller = new AbortController();
        const runId = randomUUID();

        return definition.run({
            runId,
            signal: controller.signal,
            emit: (update) => {
                this.emit(name, runId, update);
            },
        });
    }

    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    protected emit(_taskName: string, _runId: string, _update: unknown): void {}
}

export class ElectronTaskManager extends TaskManager {
    constructor(private readonly getWindow: () => BrowserWindow | null) {
        super();
    }

    protected override emit(taskName: string, runId: string, update: unknown) {
        const win = this.getWindow();
        if (!win) return;

        console.log(
            `Emitting update for task "${taskName}" (runId: ${runId}):`,
            update,
        );

        win.webContents.send(`${taskName}:update`, {
            runId,
            update,
        });
    }
}
