import { useEffect, useRef, useState } from "react";
import type { PointerEvent, WheelEvent } from "react";
import { Play, Move3D, RotateCw, Scale, Square } from "lucide-react";
import type { EditorControlMode } from "src/shared/types/ipc";

type SelectionState = {
    id: number;
    name: string;
};

const modes: Array<{
    mode: EditorControlMode;
    label: string;
    icon: typeof Move3D;
}> = [
    { mode: "move", label: "Move", icon: Move3D },
    { mode: "rotate", label: "Rotate", icon: RotateCw },
    { mode: "scale", label: "Scale", icon: Scale },
];

export default function EditorOverlay() {
    const [playing, setPlaying] = useState(false);
    const [mode, setMode] = useState<EditorControlMode>("move");
    const [selection, setSelection] = useState<SelectionState>({
        id: -1,
        name: "",
    });
    const activePointerButton = useRef(0);

    useEffect(() => {
        const interval = window.setInterval(() => {
            void window.editorControls.getSelection().then(setSelection);
        }, 250);

        return () => {
            window.clearInterval(interval);
        };
    }, []);

    async function togglePlaying() {
        const next = !playing;
        setPlaying(next);
        await window.editorControls.setPlaying(next);
    }

    async function selectMode(nextMode: EditorControlMode) {
        setMode(nextMode);
        await window.editorControls.setMode(nextMode);
    }

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
        sendPointer(event, 2, activePointerButton.current || editorButton(event.button));
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
                className="absolute inset-0"
                onPointerDown={onViewportPointerDown}
                onPointerMove={onViewportPointerMove}
                onPointerUp={onViewportPointerUp}
                onPointerCancel={onViewportPointerUp}
                onWheel={onViewportWheel}
                onContextMenu={(event) => event.preventDefault()}
            />
            <div className="absolute inset-x-0 top-0 z-10 h-14 [app-region:drag]">
                <div className="ml-[76px] mr-3 mt-3 flex h-11 items-center rounded-2xl border border-white/12 bg-black/38 px-3 backdrop-blur-xl shadow-[0_12px_40px_rgba(0,0,0,0.28)]">
                    <div className="flex items-center gap-2">
                        <div className="h-2.5 w-2.5 rounded-full bg-[#00c2ff]" />
                        <span className="text-[12px] font-semibold tracking-[0.22em] text-white/80 uppercase">
                            Atlas Editor
                        </span>
                    </div>

                    <div className="ml-4 flex items-center gap-1 rounded-xl border border-white/10 bg-white/6 p-1 [app-region:no-drag]">
                        <button
                            type="button"
                            onClick={() => {
                                void togglePlaying();
                            }}
                            className={`flex h-8 items-center gap-2 rounded-lg px-3 text-[12px] font-medium transition ${
                                playing
                                    ? "bg-[#00c2ff] text-black"
                                    : "text-white/80 hover:bg-white/10 hover:text-white"
                            }`}
                        >
                            {playing ? (
                                <Square size={13} strokeWidth={2.2} />
                            ) : (
                                <Play size={13} strokeWidth={2.2} />
                            )}
                            {playing ? "Stop" : "Play"}
                        </button>

                        {modes.map(({ mode: nextMode, label, icon: Icon }) => (
                            <button
                                key={nextMode}
                                type="button"
                                onClick={() => {
                                    void selectMode(nextMode);
                                }}
                                className={`flex h-8 items-center gap-2 rounded-lg px-3 text-[12px] font-medium transition ${
                                    mode === nextMode
                                        ? "bg-white text-black"
                                        : "text-white/80 hover:bg-white/10 hover:text-white"
                                }`}
                            >
                                <Icon size={13} strokeWidth={2.2} />
                                {label}
                            </button>
                        ))}
                    </div>

                    <div className="ml-auto flex items-center gap-2 rounded-xl border border-white/10 bg-white/6 px-3 py-1.5 text-[12px] [app-region:no-drag]">
                        <span className="text-white/45">Selection</span>
                        <span className="font-medium text-white/90">
                            {selection.id >= 0
                                ? selection.name || `Object ${selection.id}`
                                : "None"}
                        </span>
                    </div>
                </div>
            </div>
        </main>
    );
}
