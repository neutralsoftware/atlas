import { ChevronLeft, ChevronRight, Folder } from "lucide-react";
import { useState } from "react";
import type { CreateProjectStyle } from "src/shared/types/ipc";
import Button from "../../components/Button";
import TextField from "../../components/TextField";

type ProjectOption = {
    style: CreateProjectStyle;
    name: string;
    description: string;
    image: string;
    color: string;
};

const options: ProjectOption[] = [
    {
        style: "pbr",
        name: "PBR (Physically Based Rendering)",
        description:
            "Create a project with a good balance of visual quality and performance, suitable for most applications.",
        image: "./pbr.png",
        color: "bg-green-500",
    },
    {
        style: "pathtracing",
        name: "Path Tracing",
        description:
            "Create a project with hyperrealistic lighting that consumes significant computational resources.",
        image: "./pathtracing.png",
        color: "bg-red-500",
    },
    {
        style: "pbr-gi",
        name: "PBR and Global Illumination",
        description:
            "Create a project with enhanced lighting effects that combines PBR with real-time global illumination techniques.",
        image: "./gi.png",
        color: "bg-blue-500",
    },
];

function buildProjectPath(location: string, name: string) {
    const trimmedLocation = location.trim();
    const trimmedName = name.trim();

    if (!trimmedLocation) {
        return "";
    }

    if (!trimmedName) {
        return trimmedLocation;
    }

    const needsSeparator =
        !trimmedLocation.endsWith("/") && !trimmedLocation.endsWith("\\");

    return `${trimmedLocation}${needsSeparator ? "/" : ""}${trimmedName}`;
}

function formatCreateError(error: unknown) {
    if (!(error instanceof Error)) {
        return "Failed to create project.";
    }

    const remotePrefix =
        "Error invoking remote method 'general:create-project': Error: ";

    return error.message.startsWith(remotePrefix)
        ? error.message.slice(remotePrefix.length)
        : error.message;
}

export function CreateProject() {
    const [option, setOption] = useState(0);
    const [step, setStep] = useState(0);
    const [name, setName] = useState("");
    const [location, setLocation] = useState("");
    const [isCreating, setIsCreating] = useState(false);
    const [error, setError] = useState<string | null>(null);

    const selectedOption = options[option]!;
    const canCreate =
        name.trim().length > 0 && location.trim().length > 0 && !isCreating;
    const projectPath = buildProjectPath(location, name);

    function typeCard(projectOption: ProjectOption) {
        return (
            <div
                className={`mt-2 flex h-90 w-90 flex-col items-center overflow-hidden rounded-3xl ${projectOption.color}`}
            >
                <div className="h-full w-full overflow-hidden">
                    <img
                        src={projectOption.image}
                        className="h-full w-full object-cover"
                        alt={projectOption.name}
                    />
                </div>
                <div
                    className={`mt-2 flex h-20 flex-col items-center ${projectOption.color}`}
                >
                    <p className="font-bold text-white">{projectOption.name}</p>
                    <p className="text-center text-xs text-white">
                        {projectOption.description}
                    </p>
                </div>
            </div>
        );
    }

    async function selectLocation() {
        if (isCreating) {
            return;
        }

        const paths = await window.app.fileDialog({
            title: "Select Project Location",
            buttonLabel: "Select Location",
            properties: ["openDirectory", "createDirectory"],
        });

        if (paths && paths.length > 0) {
            setLocation(paths[0] as string);
            setError(null);
        }
    }

    async function createProject() {
        if (!canCreate) {
            return;
        }

        setIsCreating(true);
        setError(null);

        try {
            await window.tasks.createProject({
                name,
                location,
                style: selectedOption.style,
            });
            window.app.destroyWindow("createProject");
        } catch (createError) {
            setError(formatCreateError(createError));
        } finally {
            setIsCreating(false);
        }
    }

    function firstStep() {
        return (
            <div className="flex h-screen w-screen flex-col items-center bg-[#f7f7f8] px-6 pt-8">
                <p className="font-bold">Create a project</p>
                <p className="mt-2 text-center text-xs text-slate-500">
                    Choose the rendering style for this project.
                </p>

                <div className="mt-2 flex flex-row items-center justify-center">
                    <button onClick={() => option > 0 && setOption(option - 1)}>
                        {option > 0 ? (
                            <ChevronLeft className="cursor-pointer" />
                        ) : (
                            <ChevronLeft className="opacity-50" />
                        )}
                    </button>
                    {typeCard(selectedOption)}
                    <button
                        onClick={() =>
                            option < options.length - 1 && setOption(option + 1)
                        }
                    >
                        {option < options.length - 1 ? (
                            <ChevronRight className="cursor-pointer" />
                        ) : (
                            <ChevronRight className="opacity-50" />
                        )}
                    </button>
                </div>

                <Button
                    type="special"
                    onClick={() => {
                        setStep(1);
                    }}
                    className="mt-4 cursor-pointer"
                >
                    Continue
                </Button>
            </div>
        );
    }

    function secondStep() {
        return (
            <div className="h-screen w-screen overflow-y-auto bg-[#f7f7f8] text-slate-900">
                <div className="flex min-h-full flex-col px-10 py-10">
                    <div className="flex items-center justify-between gap-4">
                    <button
                        className="text-sm font-medium text-slate-500 transition-colors hover:text-slate-800"
                        onClick={() => {
                            if (!isCreating) {
                                setStep(0);
                                setError(null);
                            }
                        }}
                    >
                        Back
                    </button>
                    <span
                        className={`rounded-full px-4 py-2 text-[11px] font-semibold text-white ${selectedOption.color}`}
                    >
                        {selectedOption.name}
                    </span>
                    </div>

                    <div className="mt-10">
                        <h1 className="text-2xl font-bold">Project details</h1>
                        <p className="mt-3 max-w-[28rem] text-sm leading-7 text-slate-500">
                            Enter a name and choose where the project folder
                            should be created.
                        </p>
                    </div>

                    <div className="mt-10 flex flex-col gap-6">
                        <div className="flex flex-col gap-3">
                            <label className="text-sm font-medium text-slate-700">
                                Project name
                            </label>
                            <TextField
                                value={name}
                                onChange={(event) => {
                                    setName(event.target.value);
                                    setError(null);
                                }}
                                placeholder="MyAtlasProject"
                                disabled={isCreating}
                                size="md"
                            />
                        </div>

                        <div className="flex flex-col gap-3">
                            <label className="text-sm font-medium text-slate-700">
                                Project location
                            </label>
                            <div className="flex gap-3">
                                <TextField
                                    value={location}
                                    onChange={(event) => {
                                        setLocation(event.target.value);
                                        setError(null);
                                    }}
                                    placeholder="/Users/maxvdec/Projects"
                                    disabled={isCreating}
                                    size="md"
                                    className="min-w-0 flex-1"
                                />
                                <Button
                                    type="secondary"
                                    onClick={() => {
                                        void selectLocation();
                                    }}
                                    className="min-w-0 shrink-0 px-5"
                                >
                                    <span className="flex items-center gap-2">
                                        <Folder size={14} />
                                        Browse
                                    </span>
                                </Button>
                            </div>
                        </div>
                    </div>

                    <div className="mt-8 rounded-[28px] border border-slate-200 bg-white/90 px-5 py-5 shadow-[0_12px_30px_-24px_rgba(15,23,42,0.28)]">
                        <p className="text-xs font-semibold uppercase tracking-[0.18em] text-slate-400">
                            Output path
                        </p>
                        <p className="mt-3 break-all text-sm font-medium leading-7 text-slate-700">
                            {projectPath || "Select a location and project name"}
                        </p>
                    </div>

                    {error ? (
                        <div className="mt-6 rounded-[24px] border border-red-200 bg-red-50 px-5 py-4">
                            <p className="break-words text-sm leading-7 text-red-600">
                                {error}
                            </p>
                        </div>
                    ) : null}

                    <div className="mt-auto flex gap-3 pt-8">
                        <Button
                            type="secondary"
                            onClick={() => {
                                if (!isCreating) {
                                    setStep(0);
                                    setError(null);
                                }
                            }}
                            className="min-w-0 flex-1"
                        >
                            Back
                        </Button>
                        <Button
                            type={canCreate ? "special" : "inactive"}
                            onClick={() => {
                                void createProject();
                            }}
                            className="min-w-0 flex-1"
                        >
                            {isCreating ? "Creating..." : "Create"}
                        </Button>
                    </div>
                </div>
            </div>
        );
    }

    return <main>{step === 0 ? firstStep() : secondStep()}</main>;
}
