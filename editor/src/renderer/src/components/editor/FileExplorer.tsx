import {
    ArrowLeft,
    ArrowRight,
    ArrowUp,
    Blocks,
    Code,
    File,
    FilePlus2,
    Folder,
    FolderCode,
    FolderPlus,
    Home,
    ImageIcon,
    LandPlot,
    Package,
    RefreshCw,
} from "lucide-react";
import {
    type ReactNode,
    useCallback,
    useEffect,
    useMemo,
    useRef,
    useState,
} from "react";
import type {
    ContextMenuItem,
    DirectoryInformation,
    FileInformation,
    FileSystemCreateKind,
} from "src/shared/types/ipc";

type ExplorerChild = DirectoryInformation | FileInformation;

type PendingCreate = {
    kind: FileSystemCreateKind;
    name: string;
};

type PendingRename = {
    path: string;
    name: string;
};

type DraggedEntry = {
    path: string;
    type: ExplorerChild["type"];
};

const internalDragType = "application/x-atlas-file-entry";

const defaultNames: Record<FileSystemCreateKind, string> = {
    scene: "New Scene.ascene",
    script: "New Script.ts",
    material: "New Material.amat",
    folder: "New Folder",
};

function basenameFromPath(value: string) {
    return value.split(/[/\\]/).pop() ?? "";
}

function NameTextArea({
    value,
    onChange,
    onConfirm,
    onCancel,
}: {
    value: string;
    onChange(value: string): void;
    onConfirm(): void;
    onCancel(): void;
}) {
    const ref = useRef<HTMLTextAreaElement>(null);

    useEffect(() => {
        ref.current?.focus();
        ref.current?.select();
    }, []);

    return (
        <textarea
            ref={ref}
            rows={1}
            value={value}
            className="mt-2 min-h-7 w-full resize-none rounded border border-blue-400 bg-white px-1 py-0.5 text-center text-xs text-slate-900 outline-none focus:ring-2 focus:ring-blue-200"
            onChange={(event) => onChange(event.target.value)}
            onClick={(event) => event.stopPropagation()}
            onDoubleClick={(event) => event.stopPropagation()}
            onKeyDown={(event) => {
                event.stopPropagation();
                if (event.key === "Enter" && !event.shiftKey) {
                    event.preventDefault();
                    onConfirm();
                } else if (event.key === "Escape") {
                    event.preventDefault();
                    onCancel();
                }
            }}
            onBlur={onCancel}
        />
    );
}

function ToolbarButton({
    children,
    disabled,
    title,
    onClick,
}: {
    children: ReactNode;
    disabled?: boolean;
    title: string;
    onClick(): void;
}) {
    return (
        <button
            className="flex h-8 w-8 items-center justify-center rounded-lg border border-slate-200 bg-white text-slate-600 shadow-sm transition hover:border-sky-200 hover:bg-sky-50 hover:text-sky-700 disabled:cursor-not-allowed disabled:opacity-35 disabled:hover:border-slate-200 disabled:hover:bg-white disabled:hover:text-slate-600"
            disabled={disabled}
            title={title}
            onClick={(event) => {
                event.stopPropagation();
                onClick();
            }}
        >
            {children}
        </button>
    );
}

export default function FileExplorer() {
    const [projectRoot, setProjectRoot] = useState("");
    const [path, setPath] = useState("");
    const [backStack, setBackStack] = useState<string[]>([]);
    const [forwardStack, setForwardStack] = useState<string[]>([]);
    const [dirInfo, setDirInfo] = useState<DirectoryInformation | null>(null);
    const [pendingCreate, setPendingCreate] = useState<PendingCreate | null>(
        null,
    );
    const [pendingRename, setPendingRename] = useState<PendingRename | null>(
        null,
    );
    const [dragOverDirectory, setDragOverDirectory] = useState<string | null>(
        null,
    );

    const fileExplorerMenuGeneric = useMemo<ContextMenuItem[]>(
        () => [
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
        ],
        [],
    );

    const fileExplorerMenuForDirectory = useMemo<ContextMenuItem[]>(
        () => [
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
        ],
        [],
    );

    const fileExplorerMenuForFile = useMemo<ContextMenuItem[]>(
        () => [
            {
                label: "Open Scene",
                action: "open-scene",
                kind: "item",
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
        ],
        [],
    );

    useEffect(() => {
        window.tasks.getCurrentProject().then((proj) => {
            const root = proj?.path ?? "/";
            setProjectRoot(root);
            setPath(root);
            setBackStack([]);
            setForwardStack([]);
        });
    }, []);

    const refreshDirectory = useCallback(async () => {
        if (!path) return;

        try {
            const info = await window.tasks.getDirectoryInformation({ path });
            setDirInfo(info);
        } catch (error) {
            setDirInfo(null);
            if (projectRoot && path !== projectRoot) {
                setPath(projectRoot);
            } else {
                console.error(error);
            }
        }
    }, [path, projectRoot]);

    useEffect(() => {
        void refreshDirectory();
    }, [refreshDirectory]);

    useEffect(() => {
        function handleFocus() {
            void refreshDirectory();
        }

        function handleVisibilityChange() {
            if (!document.hidden) {
                void refreshDirectory();
            }
        }

        window.addEventListener("focus", handleFocus);
        document.addEventListener("visibilitychange", handleVisibilityChange);

        return () => {
            window.removeEventListener("focus", handleFocus);
            document.removeEventListener(
                "visibilitychange",
                handleVisibilityChange,
            );
        };
    }, [refreshDirectory]);

    function navigateTo(nextPath: string) {
        if (!nextPath || nextPath === path) {
            return;
        }

        setBackStack((prev) => [...prev, path]);
        setForwardStack([]);
        setPath(nextPath);
        setPendingCreate(null);
        setPendingRename(null);
    }

    function goBack() {
        setBackStack((prev) => {
            const next = prev.at(-1);
            if (!next) {
                return prev;
            }

            setForwardStack((forward) => [path, ...forward]);
            setPath(next);
            setPendingCreate(null);
            setPendingRename(null);
            return prev.slice(0, -1);
        });
    }

    function goForward() {
        setForwardStack((prev) => {
            const next = prev[0];
            if (!next) {
                return prev;
            }

            setBackStack((back) => [...back, path]);
            setPath(next);
            setPendingCreate(null);
            setPendingRename(null);
            return prev.slice(1);
        });
    }

    function parentPath(currentPath: string) {
        if (!projectRoot || currentPath === projectRoot) {
            return null;
        }

        const normalized = currentPath.replace(/\/$/, "");
        const parent = normalized.slice(0, normalized.lastIndexOf("/"));
        if (!parent || parent.length < projectRoot.length) {
            return projectRoot;
        }
        return parent;
    }

    function goUp() {
        const parent = parentPath(path);
        if (parent) {
            navigateTo(parent);
        }
    }

    function relativePathParts() {
        if (!projectRoot || !path.startsWith(projectRoot)) {
            return [];
        }

        return path
            .slice(projectRoot.length)
            .split("/")
            .filter(Boolean);
    }

    function pathForPart(index: number) {
        const parts = relativePathParts().slice(0, index + 1);
        return [projectRoot, ...parts].join("/").replace(/\/+/g, "/");
    }

    function childPath(child: ExplorerChild) {
        return `${path.replace(/\/$/, "")}/${child.name}`;
    }

    function getIconForFileType(type: string) {
        if (type === ".ascene") {
            return <LandPlot className="h-12 w-12 text-slate-500" />;
        } else if (type === ".png" || type === ".jpg" || type === ".jpeg") {
            return <ImageIcon className="h-12 w-12 text-slate-500" />;
        } else if (type === ".ts") {
            return <Code className="h-12 w-12 text-slate-500" />;
        } else if (type === ".atlas") {
            return <Package className="h-12 w-12 text-slate-500" />;
        }
        return <File className="h-12 w-12 text-slate-500" />;
    }

    function getIconForDirName(dirName: string) {
        if (dirName === "assets") {
            return <Blocks className="h-12 w-12 text-blue-500" />;
        } else if (dirName === "scripts") {
            return <FolderCode className="h-12 w-12 text-blue-500" />;
        }
        return <Folder className="h-12 w-12 text-blue-500" />;
    }

    function getIconForCreateKind(kind: FileSystemCreateKind) {
        if (kind === "folder") {
            return getIconForDirName("");
        } else if (kind === "scene") {
            return getIconForFileType(".ascene");
        } else if (kind === "script") {
            return getIconForFileType(".ts");
        }
        return getIconForFileType(".amat");
    }

    function uniqueDefaultName(kind: FileSystemCreateKind) {
        const baseName = defaultNames[kind];
        const names = new Set(dirInfo?.children.map((child) => child.name));
        if (!names.has(baseName)) {
            return baseName;
        }

        const dot = baseName.lastIndexOf(".");
        const stem = dot > 0 ? baseName.slice(0, dot) : baseName;
        const extension = dot > 0 ? baseName.slice(dot) : "";
        let index = 2;
        let nextName = `${stem} ${index}${extension}`;
        while (names.has(nextName)) {
            index += 1;
            nextName = `${stem} ${index}${extension}`;
        }
        return nextName;
    }

    function beginCreate(kind: FileSystemCreateKind) {
        setPendingRename(null);
        setPendingCreate({ kind, name: uniqueDefaultName(kind) });
    }

    async function confirmCreate() {
        if (!pendingCreate) return;

        const createRequest = pendingCreate;
        setPendingCreate(null);
        try {
            await window.fileSystem.createEntry({
                directory: path,
                kind: createRequest.kind,
                name: createRequest.name,
            });
            await refreshDirectory();
        } catch (error) {
            window.alert(error instanceof Error ? error.message : String(error));
            setPendingCreate(createRequest);
        }
    }

    async function confirmRename() {
        if (!pendingRename) return;

        const renameRequest = pendingRename;
        setPendingRename(null);
        try {
            await window.fileSystem.renameEntry(renameRequest);
            await refreshDirectory();
        } catch (error) {
            window.alert(error instanceof Error ? error.message : String(error));
            setPendingRename(renameRequest);
        }
    }

    async function runAction(action: string | null, targetPath: string) {
        if (!action) return;

        switch (action) {
            case "new-scene":
                beginCreate("scene");
                break;
            case "new-script":
                beginCreate("script");
                break;
            case "new-material":
                beginCreate("material");
                break;
            case "new-folder":
                beginCreate("folder");
                break;
            case "open-scene":
                await window.fileSystem.openScene({ path: targetPath });
                await refreshDirectory();
                break;
            case "rename":
                setPendingCreate(null);
                setPendingRename({
                    path: targetPath,
                    name: basenameFromPath(targetPath),
                });
                break;
            case "delete":
                if (!window.confirm(`Delete "${basenameFromPath(targetPath)}"?`)) {
                    break;
                }
                await window.fileSystem.deleteEntry({ path: targetPath });
                await refreshDirectory();
                break;
            case "reveal-in-finder":
                await window.fileSystem.revealInFinder({ path: targetPath });
                break;
            case "copy-path":
                await navigator.clipboard.writeText(targetPath);
                break;
        }
    }

    async function handleBackgroundContextMenu(event: React.MouseEvent) {
        event.preventDefault();
        const action = await window.contextMenu.show(fileExplorerMenuGeneric);
        await runAction(action, path);
    }

    async function handleChildContextMenu(
        event: React.MouseEvent,
        child: ExplorerChild,
    ) {
        event.preventDefault();
        event.stopPropagation();

        const targetPath = childPath(child);
        const menu =
            child.type === "directory"
                ? fileExplorerMenuForDirectory
                : fileExplorerMenuForFile.map((item) =>
                      item.kind === "item" && item.action === "open-scene"
                          ? {
                                ...item,
                                enabled: child.extension === ".ascene",
                            }
                          : item,
                  );
        const action = await window.contextMenu.show(menu);
        await runAction(action, targetPath);
    }

    function getInternalDrag(event: React.DragEvent) {
        const raw = event.dataTransfer.getData(internalDragType);
        if (!raw) {
            return null;
        }

        try {
            return JSON.parse(raw) as DraggedEntry;
        } catch {
            return null;
        }
    }

    function getDroppedPaths(event: React.DragEvent) {
        return Array.from(event.dataTransfer.files)
            .map((file) => window.fileSystem.getDroppedFilePath(file))
            .filter(Boolean);
    }

    async function handleDrop(event: React.DragEvent, targetDirectory: string) {
        event.preventDefault();
        event.stopPropagation();
        setDragOverDirectory(null);

        try {
            const internalDrag = getInternalDrag(event);
            if (internalDrag) {
                await window.fileSystem.moveEntry({
                    source: internalDrag.path,
                    targetDirectory,
                });
            } else {
                const sources = getDroppedPaths(event);
                if (sources.length > 0) {
                    await window.fileSystem.copyExternalEntries({
                        sources,
                        targetDirectory,
                    });
                }
            }
            await refreshDirectory();
        } catch (error) {
            window.alert(error instanceof Error ? error.message : String(error));
        }
    }

    function handleDragOver(event: React.DragEvent, targetDirectory: string) {
        event.preventDefault();
        event.stopPropagation();
        event.dataTransfer.dropEffect = getInternalDrag(event) ? "move" : "copy";
        setDragOverDirectory(targetDirectory);
    }

    function handleChildClick(child: ExplorerChild) {
        if (child.type === "directory") {
            navigateTo(childPath(child));
        }
    }

    function handleChildDoubleClick(child: ExplorerChild) {
        if (child.type === "file" && child.extension === ".ascene") {
            void window.fileSystem.openScene({ path: childPath(child) });
        }
    }

    function renderName(child: ExplorerChild) {
        const targetPath = childPath(child);
        if (pendingRename?.path === targetPath) {
            return (
                <NameTextArea
                    value={pendingRename.name}
                    onChange={(name) =>
                        setPendingRename((prev) =>
                            prev ? { ...prev, name } : prev,
                        )
                    }
                    onConfirm={() => void confirmRename()}
                    onCancel={() => setPendingRename(null)}
                />
            );
        }

        return (
            <span className="mt-2 w-full truncate text-center text-xs text-slate-700">
                {child.name}
            </span>
        );
    }

    const pathParts = relativePathParts();
    const directoryCount =
        dirInfo?.children.filter((child) => child.type === "directory").length ??
        0;
    const fileCount =
        dirInfo?.children.filter((child) => child.type === "file").length ?? 0;

    return (
        <div className="flex h-full w-full flex-col border-t border-slate-200 bg-slate-50 text-slate-950 shadow-[0_-16px_20px_rgba(15,23,42,0.25)] [clip-path:inset(-3rem_0_0_0)]">
            <div className="flex min-h-16 items-center gap-3 border-b border-slate-200 bg-white px-4">
                <div className="flex items-center gap-1">
                    <ToolbarButton
                        title="Back"
                        disabled={backStack.length === 0}
                        onClick={goBack}
                    >
                        <ArrowLeft className="h-4 w-4" />
                    </ToolbarButton>
                    <ToolbarButton
                        title="Forward"
                        disabled={forwardStack.length === 0}
                        onClick={goForward}
                    >
                        <ArrowRight className="h-4 w-4" />
                    </ToolbarButton>
                    <ToolbarButton
                        title="Up"
                        disabled={!parentPath(path)}
                        onClick={goUp}
                    >
                        <ArrowUp className="h-4 w-4" />
                    </ToolbarButton>
                    <ToolbarButton title="Refresh" onClick={refreshDirectory}>
                        <RefreshCw className="h-4 w-4" />
                    </ToolbarButton>
                </div>

                <div className="flex min-w-0 flex-1 items-center gap-1 overflow-hidden rounded-xl border border-slate-200 bg-slate-50 px-2 py-1 shadow-inner">
                    <button
                        className="flex h-7 items-center gap-1 rounded-lg px-2 text-xs font-bold text-slate-600 transition hover:bg-white hover:text-sky-700"
                        onClick={() => navigateTo(projectRoot)}
                        title={projectRoot}
                    >
                        <Home className="h-3.5 w-3.5" />
                        <span className="max-w-32 truncate">
                            {basenameFromPath(projectRoot) || "Project"}
                        </span>
                    </button>
                    {pathParts.map((part, index) => (
                        <button
                            key={`${part}-${index}`}
                            className="flex min-w-0 items-center gap-1 rounded-lg px-1.5 py-1 text-xs font-semibold text-slate-500 transition hover:bg-white hover:text-sky-700"
                            onClick={() => navigateTo(pathForPart(index))}
                            title={pathForPart(index)}
                        >
                            <span className="text-slate-300">/</span>
                            <span className="truncate">{part}</span>
                        </button>
                    ))}
                </div>

                <div className="hidden shrink-0 items-center gap-2 text-[11px] font-semibold text-slate-400 md:flex">
                    <span>{directoryCount} folders</span>
                    <span className="h-1 w-1 rounded-full bg-slate-300" />
                    <span>{fileCount} files</span>
                </div>

                <div className="flex items-center gap-1">
                    <ToolbarButton
                        title="New Scene"
                        onClick={() => beginCreate("scene")}
                    >
                        <FilePlus2 className="h-4 w-4" />
                    </ToolbarButton>
                    <ToolbarButton
                        title="New Folder"
                        onClick={() => beginCreate("folder")}
                    >
                        <FolderPlus className="h-4 w-4" />
                    </ToolbarButton>
                </div>
            </div>
            {dirInfo ? (
                <div
                    className="grid flex-1 content-start gap-3 overflow-auto p-4 [grid-template-columns:repeat(auto-fill,minmax(118px,1fr))]"
                    onContextMenu={handleBackgroundContextMenu}
                    onDragOver={(event) => handleDragOver(event, path)}
                    onDragLeave={() => setDragOverDirectory(null)}
                    onDrop={(event) => handleDrop(event, path)}
                >
                    {pendingCreate && (
                        <div className="flex flex-col items-center rounded-lg bg-blue-50 p-3">
                            {getIconForCreateKind(pendingCreate.kind)}
                            <NameTextArea
                                value={pendingCreate.name}
                                onChange={(name) =>
                                    setPendingCreate((prev) =>
                                        prev ? { ...prev, name } : prev,
                                    )
                                }
                                onConfirm={() => void confirmCreate()}
                                onCancel={() => setPendingCreate(null)}
                            />
                        </div>
                    )}
                    {dirInfo.children.map((child) => {
                        const targetPath = childPath(child);
                        const isDirectory = child.type === "directory";
                        const isDropTarget = dragOverDirectory === targetPath;
                        const fileExtension =
                            child.type === "file"
                                ? child.extension.replace(".", "").toUpperCase()
                                : "Folder";

                        return (
                            <div
                                key={child.name}
                                draggable={!pendingRename}
                                className={`group flex min-h-28 cursor-pointer flex-col items-center justify-between rounded-xl border p-3 shadow-sm transition ${
                                    isDropTarget
                                        ? "border-sky-300 bg-sky-50 ring-2 ring-sky-200"
                                        : "border-slate-200 bg-white hover:border-sky-200 hover:bg-sky-50/60 hover:shadow-md"
                                }`}
                                onClick={() => handleChildClick(child)}
                                onDoubleClick={() => handleChildDoubleClick(child)}
                                onContextMenu={(event) =>
                                    handleChildContextMenu(event, child)
                                }
                                onDragStart={(event) => {
                                    event.dataTransfer.effectAllowed = "move";
                                    event.dataTransfer.setData(
                                        internalDragType,
                                        JSON.stringify({
                                            path: targetPath,
                                            type: child.type,
                                        } satisfies DraggedEntry),
                                    );
                                }}
                                onDragOver={
                                    isDirectory
                                        ? (event) =>
                                              handleDragOver(event, targetPath)
                                        : undefined
                                }
                                onDragLeave={
                                    isDirectory
                                        ? () => setDragOverDirectory(null)
                                        : undefined
                                }
                                onDrop={
                                    isDirectory
                                        ? (event) =>
                                              handleDrop(event, targetPath)
                                        : undefined
                                }
                            >
                                {isDirectory
                                    ? getIconForDirName(child.name)
                                    : getIconForFileType(child.extension)}
                                {renderName(child)}
                                <span className="mt-1 max-w-full truncate rounded-full bg-slate-100 px-2 py-0.5 text-[10px] font-bold uppercase text-slate-400 transition group-hover:bg-white group-hover:text-sky-500">
                                    {fileExtension || "File"}
                                </span>
                            </div>
                        );
                    })}
                </div>
            ) : (
                <div className="flex flex-1 items-center justify-center text-sm text-slate-500">
                    Loading...
                </div>
            )}
        </div>
    );
}
