import { useEffect, useEffectEvent, useRef, useState } from "react";
import type { PointerEvent, WheelEvent } from "react";
import TopSelector from "../components/editor/TopSelector";
import Hierarchy from "../components/editor/Hierarchy";
import { Project } from "src/shared/types/atlas";
import { AppInfo } from "../model/app";
import FileExplorer from "../components/editor/FileExplorer";
import Inspector from "../components/editor/Inspector";

export default function EditorOverlay() {
    const activePointerButton = useRef(0);
    const shellRef = useRef<HTMLElement>(null);
    const viewportRef = useRef<HTMLDivElement>(null);
    const savedSceneSignatureRef = useRef<string | null>(null);
    const [project, setProject] = useState<Project | null>(null);
    const [sceneDirty, setSceneDirty] = useState(false);
    const [appInfo, setAppInfo] = useState<AppInfo>({
        debug: false,
        buildId: "",
        platform: "",
    });

    useEffect(() => {
        window.tasks.getCurrentProject().then((project) => {
            setProject(project);
        });

        window.app.getAppInfo().then(setAppInfo);
    }, []);

    useEffect(() => {
        const keyMap: Record<string, 0 | 1 | 2 | 3> = {
            ArrowUp: 0,
            ArrowDown: 1,
            ArrowLeft: 2,
            ArrowRight: 3,
        };

        function handleKeyDown(event: KeyboardEvent) {
            const key = keyMap[event.code];
            if (key === undefined || event.repeat) {
                return;
            }
            event.preventDefault();
            void window.editorInput.key(key, true);
        }

        function handleKeyUp(event: KeyboardEvent) {
            const key = keyMap[event.code];
            if (key === undefined) {
                return;
            }
            event.preventDefault();
            void window.editorInput.key(key, false);
        }

        window.addEventListener("keydown", handleKeyDown);
        window.addEventListener("keyup", handleKeyUp);

        return () => {
            window.removeEventListener("keydown", handleKeyDown);
            window.removeEventListener("keyup", handleKeyUp);
            for (const key of [0, 1, 2, 3] as const) {
                void window.editorInput.key(key, false);
            }
        };
    }, []);

    function sceneSignature(scene: unknown) {
        if (!scene || typeof scene !== "object") {
            return "";
        }
        const objects = "objects" in scene ? scene.objects : [];
        return JSON.stringify(objects);
    }

    async function saveCurrentScene() {
        const saved = await window.editorControls.saveCurrentScene();
        if (!saved) {
            return;
        }
        const scene = await window.editorControls.getSceneObjects();
        savedSceneSignatureRef.current = sceneSignature(scene);
        setSceneDirty(false);
    }
    const saveCurrentSceneEvent = useEffectEvent(saveCurrentScene);

    useEffect(() => {
        savedSceneSignatureRef.current = null;

        let active = true;
        const checkSceneDirty = async () => {
            const scene = await window.editorControls.getSceneObjects();
            if (!active) {
                return;
            }
            const signature = sceneSignature(scene);
            if (savedSceneSignatureRef.current === null) {
                savedSceneSignatureRef.current = signature;
                setSceneDirty(false);
                return;
            }
            setSceneDirty(signature !== savedSceneSignatureRef.current);
        };

        checkSceneDirty();
        const interval = window.setInterval(checkSceneDirty, 700);
        return () => {
            active = false;
            window.clearInterval(interval);
        };
    }, [project?.id]);

    useEffect(() => {
        function handleSaveShortcut(event: KeyboardEvent) {
            if (
                !(event.metaKey || event.ctrlKey) ||
                event.key.toLowerCase() !== "s"
            ) {
                return;
            }
            event.preventDefault();
            void saveCurrentSceneEvent();
        }

        window.addEventListener("keydown", handleSaveShortcut, true);
        return () => {
            window.removeEventListener("keydown", handleSaveShortcut, true);
        };
    }, []);

    useEffect(() => {
        function handleDeleteShortcut(event: KeyboardEvent) {
            if (event.key !== "Delete" && event.key !== "Backspace") {
                return;
            }

            const target = event.target as HTMLElement | null;
            if (target?.closest("input, textarea, [contenteditable='true']")) {
                return;
            }

            event.preventDefault();
            void (async () => {
                const selection = await window.editorControls.getSelection();
                if (!selection || selection.id < 0) {
                    return;
                }
                await window.editorControls.deleteObject(selection.id);
            })();
        }

        window.addEventListener("keydown", handleDeleteShortcut, true);
        return () => {
            window.removeEventListener("keydown", handleDeleteShortcut, true);
        };
    }, []);

    useEffect(() => {
        function publishViewportBounds() {
            const viewport = viewportRef.current;
            if (!viewport) {
                return;
            }

            const rect = viewport.getBoundingClientRect();
            void window.editorInput.setViewportBounds({
                x: rect.left,
                y: rect.top,
                width: rect.width,
                height: rect.height,
                scale: window.devicePixelRatio,
            });
        }

        publishViewportBounds();

        const observer = new ResizeObserver(publishViewportBounds);
        if (viewportRef.current) {
            observer.observe(viewportRef.current);
        }
        if (shellRef.current) {
            observer.observe(shellRef.current);
        }

        const mutationObserver = new MutationObserver(publishViewportBounds);
        if (shellRef.current) {
            mutationObserver.observe(shellRef.current, {
                attributes: true,
                childList: true,
                subtree: true,
            });
        }

        window.addEventListener("resize", publishViewportBounds);

        return () => {
            observer.disconnect();
            mutationObserver.disconnect();
            window.removeEventListener("resize", publishViewportBounds);
        };
    }, []);

    function editorButton(button: number) {
        return button + 1;
    }

    function sendPointer(
        event: PointerEvent<HTMLDivElement>,
        action: 0 | 1 | 2,
        button: number,
    ) {
        const rect = event.currentTarget.getBoundingClientRect();
        void window.editorInput.pointer({
            action,
            x: event.clientX - rect.left,
            y: event.clientY - rect.top,
            button,
            scale: window.devicePixelRatio,
        });
    }

    function onViewportPointerDown(event: PointerEvent<HTMLDivElement>) {
        event.currentTarget.focus({ preventScroll: true });
        activePointerButton.current = editorButton(event.button);
        event.currentTarget.setPointerCapture(event.pointerId);
        sendPointer(event, 0, activePointerButton.current);
    }

    function onViewportPointerMove(event: PointerEvent<HTMLDivElement>) {
        sendPointer(event, 1, activePointerButton.current);
    }

    function onViewportPointerUp(event: PointerEvent<HTMLDivElement>) {
        sendPointer(
            event,
            2,
            activePointerButton.current || editorButton(event.button),
        );
        activePointerButton.current = 0;
        if (event.currentTarget.hasPointerCapture(event.pointerId)) {
            event.currentTarget.releasePointerCapture(event.pointerId);
        }
    }

    function onViewportWheel(event: WheelEvent<HTMLDivElement>) {
        event.preventDefault();
        void window.editorInput.scroll(-event.deltaY * 0.12);
    }

    return (
        <main
            ref={shellRef}
            className="fixed inset-0 grid grid-cols-[18rem_minmax(0,1fr)_18rem] grid-rows-[32.5rem_minmax(0,1fr)] bg-transparent text-white select-none"
        >
            <div className="w-full h-8 bg-white absolute z-50 flex items-center justify-center text-black">
                <p className="text-xs font-bold">
                    {project?.name ?? "Atlas"}
                    {sceneDirty ? " (not saved)" : ""} - Atlas Engine Alpha 9{" "}
                    {appInfo.debug &&
                        "(Development Build - " + appInfo.buildId + ")"}
                </p>
            </div>
            <Hierarchy />

            <section ref={viewportRef} className="relative min-w-0">
                <div
                    className="absolute inset-0 z-0 outline-none"
                    tabIndex={0}
                    onPointerDown={onViewportPointerDown}
                    onPointerMove={onViewportPointerMove}
                    onPointerUp={onViewportPointerUp}
                    onPointerCancel={onViewportPointerUp}
                    onWheel={onViewportWheel}
                    onContextMenu={(event) => event.preventDefault()}
                />

                <TopSelector />
            </section>
            <div className="relative col-span-3 h-full min-w-0">
                <FileExplorer />
                <div className="pointer-events-none absolute top-0 right-72 left-72 z-40 h-full shadow-[0_-16px_20px_rgba(15,23,42,0.25)] [clip-path:inset(-3rem_0_0_0)]" />
            </div>

            <div className="relative col-start-3 row-start-1 h-full min-h-0">
                <Inspector />
                <div className="pointer-events-none absolute top-0 left-0 z-40 h-full w-full shadow-[-16px_0_20px_rgba(15,23,42,0.25)] [clip-path:inset(0_0_0_-3rem)]" />
            </div>
        </main>
    );
}
