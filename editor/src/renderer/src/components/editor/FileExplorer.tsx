import {
    Blocks,
    Code,
    File,
    Folder,
    FolderCode,
    ImageIcon,
    LandPlot,
    Package,
} from "lucide-react";
import { useEffect, useState } from "react";
import {
    ContextMenuItem,
    DirectoryInformation,
    FileInformation,
} from "src/shared/types/ipc";

export default function FileExplorer() {
    const [path, setPath] = useState("");
    const [dirInfo, setDirInfo] = useState<DirectoryInformation | null>(null);

    useEffect(() => {
        window.tasks
            .getCurrentProject()
            .then((proj) => setPath(proj?.path ?? "/"));
    }, []);

    useEffect(() => {
        if (!path) return;

        window.tasks
            .getDirectoryInformation({path})
            .then((info) => setDirInfo(info));
    }, [path]);

    function getIconForFileType(type: string) {
        if (type === ".ascene") {
            return <LandPlot className="h-12 w-12 text-slate-500"></LandPlot>;
        } else if (type === ".png" || type === ".jpg" || type === ".jpeg") {
            return <ImageIcon className="h-12 w-12 text-slate-500"></ImageIcon>;
        } else if (type === ".ts") {
            return <Code className="h-12 w-12 text-slate-500"></Code>;
        } else if (type === ".atlas") {
            return <Package className="h-12 w-12 text-slate-500"></Package>;
        }
        return <File className="h-12 w-12 text-slate-500"></File>;
    }

    function getIconForDirName(dirName: string) {
        if (dirName === "assets") {
            return <Blocks className="h-12 w-12 text-blue-500"></Blocks>;
        } else if (dirName === "scripts") {
            return (
                <FolderCode className="h-12 w-12 text-blue-500"></FolderCode>
            );
        }
        return <Folder className="h-12 w-12 text-blue-500"></Folder>;
    }

    function handleClickOnChild(child: DirectoryInformation | FileInformation) {
        if (child.type === "directory") {
            setPath((prev) => `${prev}/${child.name}`);
        } else if (child.type === "file") {
            return;
        }
    }

    const fileExplorerMenuGeneric: ContextMenuItem[] = [
        {
            kind: "submenu",
            label: "Create",
            children: [
                {
                    kind: "item",
                    label: "Scene",
                    action: "new-scene",
                },
                {
                    kind: "item",
                    label: "Script",
                    action: "new-script",
                },
                {
                    kind: "item",
                    label: "Material",
                    action: "new-material",
                },
            ],
        },
        {
            kind: "item",
            label: "New Folder",
            action: "new-folder",
        },
        {
            kind: "separator",
        },
        {
            label: "Reveal in Finder",
            action: "reveal-in-finder",
            kind: "item",
        },
        {
            label: "Copy Path",
            action: "copy-path",
            kind: "item",
        },
    ];

    const fileExplorerMenuForFile: ContextMenuItem[] =
        fileExplorerMenuGeneric.concat([
            {
                kind: "separator",
            },
            {
                label: "Rename",
                action: "rename",
                kind: "item",
            },
            {
                label: "Delete",
                action: "delete",
                kind: "item",
            },
        ]);

    function handleBackgroundContextMenu(event: React.MouseEvent) {
        event.preventDefault();

        window.contextMenu.show(fileExplorerMenuGeneric);
    }

    function handleChildContextMenu(
        event: React.MouseEvent,
        child: DirectoryInformation | FileInformation,
    ) {
        event.preventDefault();
        event.stopPropagation();

        window.contextMenu.show(fileExplorerMenuForFile);
    }

    useEffect(() => {
        window.contextMenu.onClick((action) => {
            switch (action) {
                case "new-scene":
                    console.log("New Scene");
                    break;
                case "new-script":
                    console.log("Create script");
                    break;

                case "new-folder":
                    console.log("Create folder");
                    break;

                case "rename":
                    console.log("Rename");
                    break;

                case "delete":
                    console.log("Delete");
                    break;

                case "copy-path":
                    navigator.clipboard.writeText(path);
                    break;
            }
        })
    }, [path]);

    return (
        <div className="h-full w-full border-t border-slate-200 bg-white">
            <div className="flex h-10 items-center gap-2 px-4 text-sm text-slate-500">
                {path}
            </div>
            {dirInfo ? (
                <div
                    className="grid grid-cols-[repeat(auto-fill,minmax(96px,1fr))] gap-3 overflow-auto p-4"
                    onContextMenu={handleBackgroundContextMenu}
                >
                    {dirInfo.children.map((child) => (
                        <div
                            key={child.name}
                            className="flex cursor-pointer flex-col items-center rounded-lg p-3 hover:bg-slate-100"
                            onClick={() => handleClickOnChild(child)}
                            onContextMenu={(event) => handleChildContextMenu(event, child)}
                        >
                            {child.type === "directory"
                                ? getIconForDirName(child.name)
                                : getIconForFileType(child.extension)}

                            <span className="mt-2 w-full truncate text-center text-xs text-slate-700">
                                {child.name}
                            </span>
                        </div>
                    ))}
                </div>
            ) : (
                <div className="flex h-full w-full items-center justify-center text-sm text-slate-500">
                    Loading...
                </div>
            )}
        </div>
    );
}
