import { Move, Play, Rotate3d, Scale3d } from "lucide-react";
import { useEffect, useState } from "react";

type Mode = "move" | "scale" | "rotate";

export default function TopSelector() {
    const [mode, setMode] = useState<Mode>("move");
    const [isPlaying, setIsPlaying] = useState(false);

    const modes = [
        { id: "move", icon: Move },
        { id: "scale", icon: Scale3d },
        { id: "rotate", icon: Rotate3d },
    ] as const;

    const selectedIndex = modes.findIndex((m) => m.id === mode);

    useEffect(() => {
        void window.editorControls.setMode(mode);
    }, [mode]);

    useEffect(() => {
        void window.editorControls.setPlaying(isPlaying);
    }, [isPlaying]);

    const buttonClass =
        "relative z-10 flex h-8 w-8 shrink-0 items-center justify-center rounded-full transition-all duration-300";

    return (
        <main className="absolute left-0 top-0 mt-6 flex w-full items-center justify-center">
            <div className="flex items-center rounded-full bg-white/70 p-1.5 shadow backdrop-blur-lg">
                <button
                    onClick={() => setIsPlaying((v) => !v)}
                    className={`${buttonClass} ${
                        isPlaying
                            ? "bg-linear-to-r from-purple-200 via-pink-200 to-blue-200 shadow-[0_0_10px_rgba(180,160,255,0.4)]"
                            : "bg-white"
                    }`}
                >
                    <Play className="h-4 w-4 text-black" strokeWidth={2.4} />
                </button>

                <div className="mx-2 h-5 w-px bg-gray-400/40" />

                <div className="relative flex rounded-full bg-white/40">
                    <div
                        className="absolute left-0 top-0 h-8 w-8 rounded-full bg-blue-300 transition-transform duration-300 ease-out"
                        style={{
                            transform: `translateX(${selectedIndex * 2}rem)`,
                        }}
                    />

                    {modes.map(({ id, icon: Icon }) => (
                        <button
                            key={id}
                            onClick={() => setMode(id)}
                            className={`${buttonClass} ${
                                mode === id
                                    ? "text-black"
                                    : "text-gray-500 hover:text-black"
                            }`}
                        >
                            <Icon className="h-4 w-4" strokeWidth={2.4} />
                        </button>
                    ))}
                </div>
            </div>
        </main>
    );
}
