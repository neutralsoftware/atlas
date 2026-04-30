import {
    ApertureIcon,
    AppWindowIcon,
    ArchiveIcon,
    BoxIcon,
    ChevronDown,
} from "lucide-react";
import { useEffect, useState } from "react";
import { Scene } from "src/shared/types/atlas";

export default function Hierarchy() {
    const [scene, setScene] = useState<Scene>();

    useEffect(() => {
        window.tasks.getObjects().then(setScene);
    }, []);

    const objectTypes: Record<string, React.ReactNode> = {
        camera: <ApertureIcon className="h-full"></ApertureIcon>,
        emptyGameObject: <ArchiveIcon className="h-full"></ArchiveIcon>,
        gameObject: <BoxIcon className="h-full"></BoxIcon>,
    };

    return (
        <aside className="relative z-40 flex h-full w-60 shrink-0 flex-col gap-2 border-r bg-white p-3 pt-10 text-sm text-black shadow-[18px_0_50px_rgba(0,0,0,0.18)]">
            <h1 className="font-black flex flex-row items-center gap-2">
                <img
                    src="../../assets/atlasBall.png"
                    alt="Atlas Engine Logo"
                    className="w-5 h-5"
                />
                {scene?.name}
            </h1>
            <div className="flex flex-col gap-1">
                {scene?.objects.map((object) => (
                    <>
                        <div
                            key={object.viewportId}
                            className="rounded-md px-2 py-1 text-xs font-medium text-slate-700 hover:bg-slate-100 flex flex-row items-center gap-2 cursor-pointer"
                        >
                            {object.children && (
                                <ChevronDown className="w-4 h-4 shrink-0 text-slate-500"></ChevronDown>
                            )}
                            <span className="w-4 h-4 shrink-0 text-slate-500">
                                {objectTypes[object.type] || <AppWindowIcon />}
                            </span>
                            {object.name}
                        </div>

                        {object.children?.map((child) => (
                            <div
                                key={child.viewportId || child.name}
                                className="rounded-md px-2 py-1 text-xs font-medium text-slate-700 hover:bg-slate-100 flex flex-row items-center gap-2 cursor-pointer ml-4"
                            >
                                <span className="w-4 h-4 shrink-0 text-slate-500">
                                    {objectTypes[child.type] || (
                                        <AppWindowIcon />
                                    )}
                                </span>
                                {child.name}
                            </div>
                        ))}
                    </>
                ))}
            </div>
        </aside>
    );
}
