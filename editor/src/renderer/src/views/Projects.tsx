import {
    ChevronDown,
    ChevronUp,
    Folder,
    GraduationCap,
    LucideIcon,
    Search,
    Settings,
    Star,
} from "lucide-react";
import { useEffect, useMemo, useState } from "react";
import Button from "../components/Button";
import TextField from "../components/TextField";
import { Project } from "src/shared/types/atlas";

export default function Projects() {
    const [page, setPage] = useState<number>(0);
    const [search, setSearch] = useState<string>("");
    const [projects, setProjects] = useState<Project[]>([]);
    const [modifiedUp, setModifiedUp] = useState<boolean>(true);
    const [starFilter, setStarFilter] = useState<boolean>(false);

    function renderOption(label: string, index: number, icon: LucideIcon) {
        const Icon = icon;
        return (
            <button
                key={index}
                className={`flex items-center gap-2 p-2 rounded-xl cursor-pointer ${
                    page === index ? "bg-gray-300" : "hover:bg-gray-100"
                }`}
                onClick={() => setPage(index)}
            >
                <Icon size={18} />
                {label}
            </button>
        );
    }

    useEffect(() => {
        window.tasks.getProjects().then((projects) => {
            if (projects) {
                setProjects(projects);
            }
        });
    }, []);

    const filteredProjects = useMemo(() => {
        const q = search.trim().toLowerCase();

        let filtered = !q
            ? [...projects]
            : projects.filter((project) =>
                  project.name.toLowerCase().includes(q),
              );

        filtered.sort((a, b) =>
            modifiedUp
                ? b.modified.getTime() - a.modified.getTime()
                : a.modified.getTime() - b.modified.getTime(),
        );

        if (starFilter) {
            filtered = filtered.filter((p) => p.starred);
        }

        return filtered;
    }, [search, projects, modifiedUp, starFilter]);

    function timeAgo(date: Date): string {
        const now = new Date();
        const diff = (date.getTime() - now.getTime()) / 1000;

        const rtf = new Intl.RelativeTimeFormat("en", { numeric: "auto" });

        const divisions = [
            { amount: 60, name: "seconds" },
            { amount: 60, name: "minutes" },
            { amount: 24, name: "hours" },
            { amount: 7, name: "days" },
            { amount: 4.34524, name: "weeks" },
            { amount: 12, name: "months" },
            { amount: Infinity, name: "years" },
        ];

        let duration = diff;

        for (const division of divisions) {
            if (Math.abs(duration) < division.amount) {
                return rtf.format(
                    Math.round(duration),
                    division.name as Intl.RelativeTimeFormatUnit,
                );
            }
            duration /= division.amount;
        }

        return "";
    }

    function projectsPage() {
        return (
            <div className="ml-60 h-screen flex flex-1 flex-col select-none">
                <div className="flex flex-row w-full p-7 items-center shrink-0">
                    <h1 className="font-bold text-2xl">Projects</h1>

                    <div className="ml-8 w-70 max-w-sm">
                        <TextField
                            value={search}
                            onChange={(event) => setSearch(event.target.value)}
                            placeholder="Search projects"
                            leading={<Search size={16} strokeWidth={2.2} />}
                        />
                    </div>

                    <div className="ml-auto flex flex-row h-10 w-auto gap-2">
                        <Button
                            type="secondary"
                            onClick={() => {}}
                            className="min-w-0 scale-85 cursor-pointer"
                        >
                            Load an existing project
                        </Button>
                        <Button
                            type="primary"
                            onClick={() => {}}
                            className="min-w-0 scale-85 cursor-pointer"
                        >
                            Create
                        </Button>
                    </div>
                </div>

                <div className="flex-1 min-h-0 pb-5 flex flex-col gap-3">
                    <div className="shrink-0 flex flex-row bg-gray-100 border-gray-300 border-y-2 p-3 gap-6 items-center -mt-2.5">
                        <Star
                            className={`${
                                starFilter
                                    ? "fill-yellow-400 text-yellow-400 cursor-pointer"
                                    : "cursor-pointer"
                            }`}
                            onClick={() => setStarFilter((prev) => !prev)}
                        />

                        <h3 className="font-bold">Name</h3>

                        <div className="ml-auto flex flex-row gap-6">
                            <div
                                className="gap-2 flex flex-row items-center cursor-pointer"
                                onClick={() => setModifiedUp((prev) => !prev)}
                            >
                                <h3 className="font-bold select-none">
                                    Modified
                                </h3>
                                {modifiedUp ? <ChevronDown /> : <ChevronUp />}
                            </div>
                        </div>
                    </div>

                    <div className="flex-1 min-h-0 overflow-y-auto pr-1">
                        <div className="flex flex-col gap-3">
                            {filteredProjects.length === 0 && (
                                <p className="text-center text-gray-500 mt-10">
                                    No projects found.
                                </p>
                            )}
                            {filteredProjects.map((project, index) => (
                                <div
                                    key={index}
                                    className="flex flex-row bg-white rounded-2xl border-gray-300 border-2 shadow-sm p-3 gap-6 items-center cursor-pointer hover:shadow-md transition-shadow mx-4"
                                    onClick={() => {
                                        console.log("Opening project.");
                                    }}
                                >
                                    <Star
                                        className={`${
                                            project.starred
                                                ? "fill-yellow-400 text-yellow-400"
                                                : ""
                                        }`}
                                        onClick={(e) => {
                                            e.stopPropagation();
                                            setProjects((prev) =>
                                                prev.map((p) =>
                                                    p.id === project.id
                                                        ? {
                                                              ...p,
                                                              starred:
                                                                  !p.starred,
                                                          }
                                                        : p,
                                                ),
                                            );
                                        }}
                                    />
                                    <h3 className="flex flex-col">
                                        <p className="font-bold">
                                            {project.name}
                                        </p>
                                        <span className="text-xs text-gray-500">
                                            {project.path}
                                        </span>
                                    </h3>
                                    <div className="ml-auto">
                                        <span className="text-sm text-gray-500">
                                            {timeAgo(project.modified)}
                                        </span>
                                    </div>
                                </div>
                            ))}
                        </div>
                    </div>
                </div>
            </div>
        );
    }

    return (
        <main className="flex flex-row">
            <div className="w-full h-7.5 [app-region:drag] fixed top-0 left-0 z-50"></div>
            <nav className="bg-gray-200 border-r-2 border-r-gray-300 h-screen w-60 flex flex-col p-5 pt-10 fixed">
                <h1 className="font-bold text-2xl select-none mb-3">
                    Atlas Engine
                </h1>
                {renderOption("Projects", 0, Folder)}
                {renderOption("Learn", 1, GraduationCap)}
                {renderOption("Settings", 2, Settings)}
            </nav>
            <div className="w-screen h-screen">
                {page === 0 && projectsPage()}
                {page === 1 && (
                    <div className="flex-1 flex items-center justify-center h-full">
                        Learn Page
                    </div>
                )}
                {page === 2 && (
                    <div className="flex-1 flex items-center justify-center h-full">
                        Settings Page
                    </div>
                )}
            </div>
        </main>
    );
}
