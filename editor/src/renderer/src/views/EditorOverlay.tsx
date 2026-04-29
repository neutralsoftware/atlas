import { useRef } from "react";
import type { PointerEvent, WheelEvent } from "react";

export default function EditorOverlay() {
    const activePointerButton = useRef(0);

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

    return (
        <main className="fixed inset-0 bg-transparent text-white select-none">
            <div
                className="absolute inset-x-0 bottom-0 top-10"
                onPointerDown={onViewportPointerDown}
                onPointerMove={onViewportPointerMove}
                onPointerUp={onViewportPointerUp}
                onPointerCancel={onViewportPointerUp}
                onWheel={onViewportWheel}
                onContextMenu={(event) => event.preventDefault()}
            />
            <div className="pointer-events-none absolute left-0 top-0 z-50 flex h-10 w-full items-center backdrop-blur">
                <div className="ml-20 text-[11px] font-semibold uppercase tracking-[0.18em] text-white/70">
                    Atlas Editor
                </div>
            </div>
        </main>
    );
}
