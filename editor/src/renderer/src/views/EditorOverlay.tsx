import { useEffect, useRef } from "react";
import type { PointerEvent, WheelEvent } from "react";
import TopSelector from "../components/editor/TopSelector";
import Hierarchy from "../components/editor/Hierarchy";

export default function EditorOverlay() {
    const activePointerButton = useRef(0);
    const shellRef = useRef<HTMLElement>(null);
    const viewportRef = useRef<HTMLDivElement>(null);

    useEffect(() => {
        const keyMap: Record<string, 0 | 1 | 2 | 3 | 4 | 5> = {
            ArrowUp: 0,
            KeyW: 0,
            ArrowDown: 1,
            KeyS: 1,
            ArrowLeft: 2,
            KeyA: 2,
            ArrowRight: 3,
            KeyD: 3,
            Space: 4,
            ShiftLeft: 5,
            ShiftRight: 5,
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
            for (const key of [0, 1, 2, 3, 4, 5] as const) {
                void window.editorInput.key(key, false);
            }
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
            className="fixed inset-0 flex bg-transparent text-white select-none"
        >
            <Hierarchy />

            <section ref={viewportRef} className="relative min-w-0 flex-1">
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
        </main>
    );
}
