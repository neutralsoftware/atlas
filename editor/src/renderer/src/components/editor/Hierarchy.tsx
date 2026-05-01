import {
    ApertureIcon,
    AppWindowIcon,
    ArchiveIcon,
    AtomIcon,
    BlocksIcon,
    BoxIcon,
    ChevronDown,
    MountainIcon,
    PlusIcon,
    TorusIcon,
} from "lucide-react";
import {
    Fragment,
    type DragEvent,
    type MouseEvent,
    type ReactNode,
    useEffect,
    useState,
} from "react";
import type { GameObject, Scene } from "src/shared/types/atlas";

const createLabels: Record<string, string> = {
    cube: "Cube",
    sphere: "Sphere",
    plane: "Plane",
    pyramid: "Pyramid",
    capsule: "Capsule",
    camera: "Camera",
    group: "Group",
};

const objectTypes: Record<string, ReactNode> = {
    camera: <ApertureIcon className="h-3.5 w-3.5" />,
    emptyGameObject: <ArchiveIcon className="h-3.5 w-3.5" />,
    emptygameobject: <ArchiveIcon className="h-3.5 w-3.5" />,
    gameObject: <BoxIcon className="h-3.5 w-3.5" />,
    gameobject: <BoxIcon className="h-3.5 w-3.5" />,
    coreobject: <BoxIcon className="h-3.5 w-3.5" />,
    solid: <BoxIcon className="h-3.5 w-3.5" />,
    cube: <BoxIcon className="h-3.5 w-3.5" />,
    sphere: <BoxIcon className="h-3.5 w-3.5" />,
    plane: <BoxIcon className="h-3.5 w-3.5" />,
    pyramid: <BoxIcon className="h-3.5 w-3.5" />,
    capsule: <BoxIcon className="h-3.5 w-3.5" />,
    group: <ArchiveIcon className="h-3.5 w-3.5" />,
    model: <TorusIcon className="h-3.5 w-3.5" />,
    compound: <ArchiveIcon className="h-3.5 w-3.5" />,
    terrain: <MountainIcon className="h-3.5 w-3.5" />,
    particleEmitter: <AtomIcon className="h-3.5 w-3.5" />,
    particleemitter: <AtomIcon className="h-3.5 w-3.5" />,
    fluid: <AppWindowIcon className="h-3.5 w-3.5" />,
    uiObject: <BlocksIcon className="h-3.5 w-3.5" />,
    uiobject: <BlocksIcon className="h-3.5 w-3.5" />,
};

function objectId(object: GameObject) {
    return object.id ?? object.viewportId;
}

function emptyScene(): Scene {
    return { name: "Scene", objects: [], selectedId: -1 };
}

export default function Hierarchy() {
    const [scene, setScene] = useState<Scene>(emptyScene);
    const [editingId, setEditingId] = useState<number | null>(null);
    const [editingName, setEditingName] = useState("");
    const [draggingId, setDraggingId] = useState<number | null>(null);

    async function refreshScene() {
        const nextScene = await window.editorControls.getSceneObjects();
        setScene(nextScene ?? emptyScene());
    }

    useEffect(() => {
        let active = true;
        const load = async () => {
            const nextScene = await window.editorControls.getSceneObjects();
            if (active) {
                setScene(nextScene ?? emptyScene());
            }
        };

        load();
        const interval = window.setInterval(load, 350);
        return () => {
            active = false;
            window.clearInterval(interval);
        };
    }, []);

    useEffect(() => {
        if (!scene.selectedId || scene.selectedId < 0) {
            return;
        }

        const node = document.querySelector(
            `[data-hierarchy-object-id="${scene.selectedId}"]`,
        );
        node?.scrollIntoView({ block: "nearest" });
    }, [scene.selectedId]);

    async function selectObject(object: GameObject, focusCamera = true) {
        const id = objectId(object);
        await window.editorControls.selectObject(id, focusCamera);
        await refreshScene();
    }

    function startRename(object: GameObject) {
        setEditingId(objectId(object));
        setEditingName(object.name);
    }

    async function commitRename(object: GameObject) {
        const id = objectId(object);
        const name = editingName.trim();
        setEditingId(null);
        if (name.length > 0 && name !== object.name) {
            await window.editorControls.renameObject(id, name);
            await refreshScene();
        }
    }

    async function openMenu(event: MouseEvent, object?: GameObject) {
        event.preventDefault();
        event.stopPropagation();

        const result = await window.editorControls.showObjectMenu(
            object
                ? {
                      id: objectId(object),
                      name: object.name,
                  }
                : undefined,
        );

        if (!result) {
            return;
        }

        if (result.action === "create") {
            const label = createLabels[result.type] ?? result.type;
            const id = await window.editorControls.createObject(
                result.type,
                label,
            );
            if (id >= 0) {
                await window.editorControls.selectObject(id, true);
            }
            await refreshScene();
            return;
        }

        if (!object) {
            return;
        }

        if (result.action === "rename") {
            startRename(object);
            return;
        }

        if (result.action === "select") {
            await selectObject(object, true);
            return;
        }

        if (result.action === "unparent") {
            await window.editorControls.setObjectParent(objectId(object), null);
            await refreshScene();
            return;
        }

        if (result.action === "delete") {
            await window.editorControls.deleteObject(objectId(object));
            await refreshScene();
        }
    }

    async function dropOnObject(event: DragEvent, target: GameObject) {
        event.preventDefault();
        event.stopPropagation();
        const sourceId = Number(event.dataTransfer.getData("text/plain"));
        const targetId = objectId(target);
        setDraggingId(null);
        if (!Number.isFinite(sourceId) || sourceId === targetId) {
            return;
        }
        await window.editorControls.setObjectParent(sourceId, targetId);
        await refreshScene();
    }

    async function dropOnRoot(event: DragEvent) {
        event.preventDefault();
        const sourceId = Number(event.dataTransfer.getData("text/plain"));
        setDraggingId(null);
        if (!Number.isFinite(sourceId)) {
            return;
        }
        await window.editorControls.setObjectParent(sourceId, null);
        await refreshScene();
    }

    function renderObject(object: GameObject, depth = 0): ReactNode {
        const id = objectId(object);
        const selected = scene.selectedId === id;
        const dragging = draggingId === id;
        const hasChildren = Boolean(object.children?.length);
        const icon = objectTypes[object.type] ?? (
            <AppWindowIcon className="h-3.5 w-3.5" />
        );

        return (
            <Fragment key={id}>
                <div
                    data-hierarchy-object-id={id}
                    draggable={editingId !== id}
                    className={[
                        "group flex h-8 cursor-default items-center gap-2 rounded-xl border px-2 text-xs transition",
                        selected
                            ? "border-sky-300 bg-sky-100 text-sky-950 shadow-[0_10px_22px_rgba(2,132,199,0.16)]"
                            : "border-transparent text-slate-700 hover:border-slate-200 hover:bg-white hover:text-slate-950",
                        dragging ? "opacity-45" : "",
                    ].join(" ")}
                    style={{ paddingLeft: `${8 + depth * 18}px` }}
                    onClick={() => selectObject(object, true)}
                    onContextMenu={(event) => openMenu(event, object)}
                    onDoubleClick={() => startRename(object)}
                    onDragStart={(event) => {
                        event.dataTransfer.setData("text/plain", String(id));
                        event.dataTransfer.effectAllowed = "move";
                        setDraggingId(id);
                    }}
                    onDragEnd={() => setDraggingId(null)}
                    onDragOver={(event) => {
                        event.preventDefault();
                        event.dataTransfer.dropEffect = "move";
                    }}
                    onDrop={(event) => dropOnObject(event, object)}
                >
                    <span className="flex h-4 w-4 items-center justify-center text-slate-400">
                        {hasChildren ? (
                            <ChevronDown className="h-3.5 w-3.5" />
                        ) : null}
                    </span>
                    <span
                        className={[
                            "flex h-5 w-5 items-center justify-center rounded-lg",
                            selected
                                ? "bg-sky-200 text-sky-800"
                                : "bg-slate-100 text-slate-500 group-hover:bg-slate-200",
                        ].join(" ")}
                    >
                        {icon}
                    </span>
                    {editingId === id ? (
                        <input
                            autoFocus
                            className="min-w-0 flex-1 rounded-md border border-sky-300 bg-white px-2 py-1 text-xs text-slate-950 outline-none"
                            value={editingName}
                            onChange={(event) =>
                                setEditingName(event.currentTarget.value)
                            }
                            onBlur={() => commitRename(object)}
                            onClick={(event) => event.stopPropagation()}
                            onKeyDown={(event) => {
                                if (event.key === "Escape") {
                                    setEditingId(null);
                                }
                                if (event.key === "Enter") {
                                    event.currentTarget.blur();
                                }
                            }}
                        />
                    ) : (
                        <span className="min-w-0 flex-1 truncate font-medium">
                            {object.name}
                        </span>
                    )}
                    <span className="hidden rounded-full bg-slate-100 px-2 py-0.5 text-[10px] uppercase tracking-[0.18em] text-slate-400 group-hover:block">
                        {object.type}
                    </span>
                </div>
                {object.children?.map((child) =>
                    renderObject(child, depth + 1),
                )}
            </Fragment>
        );
    }

    return (
        <aside
            className="relative z-40 flex h-full w-72 shrink-0 flex-col border-r border-slate-200 bg-[linear-gradient(180deg,#ffffff_0%,#f8fafc_48%,#eef6ff_100%)] pt-10 text-slate-950 shadow-[22px_0_60px_rgba(15,23,42,0.18)]"
            onContextMenu={(event) => openMenu(event)}
            onDragOver={(event) => event.preventDefault()}
            onDrop={(event) => dropOnRoot(event)}
        >
            <div className="flex items-center justify-between border-b border-slate-200/80 px-4 pb-3 pt-3">
                <div className="min-w-0">
                    <h1 className="truncate text-sm font-black tracking-tight">
                        {scene.name}
                    </h1>
                    <p className="text-[10px] font-semibold uppercase tracking-[0.22em] text-slate-400">
                        Live Scene
                    </p>
                </div>
                <button
                    className="flex h-8 w-8 items-center justify-center rounded-xl border border-slate-200 bg-white text-slate-600 shadow-sm transition hover:border-sky-200 hover:bg-sky-50 hover:text-sky-700"
                    onClick={(event) => openMenu(event)}
                    title="Add new object"
                >
                    <PlusIcon className="h-4 w-4" />
                </button>
            </div>
            <div className="flex-1 overflow-y-auto px-3 py-3">
                {scene.objects.length > 0 ? (
                    <div className="flex flex-col gap-1">
                        {scene.objects.map((object) => renderObject(object))}
                    </div>
                ) : (
                    <button
                        className="flex w-full flex-col items-start gap-2 rounded-2xl border border-dashed border-slate-300 bg-white/70 p-4 text-left text-slate-500 transition hover:border-sky-300 hover:bg-sky-50 hover:text-sky-800"
                        onClick={(event) => openMenu(event)}
                    >
                        <span className="text-sm font-bold text-slate-700">
                            No runtime objects
                        </span>
                        <span className="text-xs">
                            Right click or press plus to add an object.
                        </span>
                    </button>
                )}
            </div>
        </aside>
    );
}
