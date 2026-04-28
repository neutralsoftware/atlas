import { useRef } from "react";
import type { CSSProperties, PointerEvent, WheelEvent } from "react";

const noDragRegionStyle = {
    WebkitAppRegion: "no-drag",
} as CSSProperties;

export default function EditorOverlay() {
    const activePointerButton = useRef(0);
    const draggingWindow = useRef(false);
    const pendingDragPoint = useRef<{ screenX: number; screenY: number } | null>(
        null,
    );
    const dragAnimationFrame = useRef<number | null>(null);

    function editorButton(button: number) {
        return button + 1;
    }

    function sendPointer(
        event: PointerEvent<HTMLDivElement>,
        action: 0 | 1 | 2,
        button: number,
    ) {
        void window.editorInput.pointer({
            action,
            x: event.clientX,
            y: event.clientY,
            button,
        });
    }

    function onViewportPointerDown(event: PointerEvent<HTMLDivElement>) {
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

    function onTitlebarPointerDown(event: PointerEvent<HTMLDivElement>) {
        if (event.button !== 0) {
            return;
        }

        draggingWindow.current = true;
        event.currentTarget.setPointerCapture(event.pointerId);
        window.app.startDrag(event.screenX, event.screenY);
    }

    function onTitlebarPointerMove(event: PointerEvent<HTMLDivElement>) {
        if (!draggingWindow.current) {
            return;
        }

        pendingDragPoint.current = {
            screenX: event.screenX,
            screenY: event.screenY,
        };

        if (dragAnimationFrame.current !== null) {
            return;
        }

        dragAnimationFrame.current = window.requestAnimationFrame(() => {
            dragAnimationFrame.current = null;
            const point = pendingDragPoint.current;
            if (!point || !draggingWindow.current) {
                return;
            }

            window.app.drag(point.screenX, point.screenY);
        });
    }

    function onTitlebarPointerUp(event: PointerEvent<HTMLDivElement>) {
        if (!draggingWindow.current) {
            return;
        }

        draggingWindow.current = false;
        if (dragAnimationFrame.current !== null) {
            window.cancelAnimationFrame(dragAnimationFrame.current);
            dragAnimationFrame.current = null;
        }
        if (pendingDragPoint.current) {
            window.app.drag(
                pendingDragPoint.current.screenX,
                pendingDragPoint.current.screenY,
            );
            pendingDragPoint.current = null;
        }
        if (event.currentTarget.hasPointerCapture(event.pointerId)) {
            event.currentTarget.releasePointerCapture(event.pointerId);
        }
        window.app.endDrag();
    }

    return (
        <main className="fixed inset-0 bg-transparent text-white select-none">
            <div
                className="absolute inset-0"
                onPointerDown={onViewportPointerDown}
                onPointerMove={onViewportPointerMove}
                onPointerUp={onViewportPointerUp}
                onPointerCancel={onViewportPointerUp}
                onWheel={onViewportWheel}
                onContextMenu={(event) => event.preventDefault()}
            />
            <div
                className="absolute top-0 z-50 flex h-10 w-full items-center bg-black/30 backdrop-blur"
                onPointerDown={onTitlebarPointerDown}
                onPointerMove={onTitlebarPointerMove}
                onPointerUp={onTitlebarPointerUp}
                onPointerCancel={onTitlebarPointerUp}
            >
                <div
                    className="ml-3 flex items-center gap-2"
                    style={noDragRegionStyle}
                    onPointerDown={(event) => event.stopPropagation()}
                >
                    <button
                        type="button"
                        aria-label="Close"
                        className="h-3.5 w-3.5 rounded-full bg-[#ff5f57] transition hover:brightness-110"
                        onClick={() => {
                            void window.app.close();
                        }}
                    />
                    <button
                        type="button"
                        aria-label="Minimize"
                        className="h-3.5 w-3.5 rounded-full bg-[#febc2e] transition hover:brightness-110"
                        onClick={() => {
                            void window.app.minimize();
                        }}
                    />
                    <button
                        type="button"
                        aria-label="Maximize"
                        className="h-3.5 w-3.5 rounded-full bg-[#28c840] transition hover:brightness-110"
                        onClick={() => {
                            void window.app.toggleMaximize();
                        }}
                    />
                </div>
                <div className="pointer-events-none ml-4 text-[11px] font-semibold uppercase tracking-[0.18em] text-white/70">
                    Atlas Editor
                </div>
            </div>
        </main>
    );
}
