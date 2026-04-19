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
        const active = page === index;
        return (
            <button
                key={index}
                className={`flex items-center gap-2.5 px-3 py-2 rounded-lg cursor-pointer my-0.5 text-sm transition-all duration-150 ${
                    active
                        ? "bg-white shadow-sm text-gray-900 font-medium"
                        : "text-gray-500 hover:bg-white/60 hover:text-gray-700"
                }`}
                onClick={() => setPage(index)}
            >
                <Icon size={15} strokeWidth={active ? 2.2 : 1.8} />
                {label}
            </button>
        );
    }

    useEffect(() => {
        window.tasks.getProjects().then((projects) => {
            if (projects) setProjects(projects);
        });
    }, []);

    const filteredProjects = useMemo(() => {
        const q = search.trim().toLowerCase();
        let filtered = !q
            ? [...projects]
            : projects.filter((p) => p.name.toLowerCase().includes(q));

        filtered.sort((a, b) =>
            modifiedUp
                ? b.modified.getTime() - a.modified.getTime()
                : a.modified.getTime() - b.modified.getTime(),
        );

        if (starFilter) filtered = filtered.filter((p) => p.starred);
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

    function newProject() {
        window.app.showWindow("createProject");
    }

    function projectsPage() {
        return (
            <div className="ml-56 h-screen flex flex-1 flex-col select-none bg-[#f7f7f8]">
                {/* Header */}
                <div className="flex flex-row w-full px-8 py-5 items-center shrink-0 border-b border-gray-200/80 bg-[#f7f7f8]">
                    <h1 className="font-semibold text-[17px] tracking-tight text-gray-900">
                        Projects
                    </h1>

                    <div className="ml-6 w-64">
                        <TextField
                            value={search}
                            onChange={(e) => setSearch(e.target.value)}
                            placeholder="Search…"
                            leading={
                                <Search
                                    size={13}
                                    strokeWidth={2.2}
                                    className="text-gray-400"
                                />
                            }
                        />
                    </div>

                    <div className="ml-auto flex flex-row items-center gap-2">
                        <Button
                            type="secondary"
                            onClick={() => {}}
                            className="min-w-0 text-sm px-3 py-1.5 cursor-pointer"
                        >
                            Load existing
                        </Button>
                        <Button
                            type="special"
                            onClick={() => {
                                newProject();
                            }}
                            className="min-w-0 text-sm px-3 py-1.5 cursor-pointer"
                        >
                            New project
                        </Button>
                    </div>
                </div>

                {/* Table header */}
                <div className="shrink-0 flex flex-row px-8 py-2.5 items-center border-b border-gray-200/80 bg-[#f7f7f8]">
                    <button
                        onClick={() => setStarFilter((prev) => !prev)}
                        className="mr-4 transition-colors duration-150"
                    >
                        <Star
                            size={14}
                            className={
                                starFilter
                                    ? "fill-amber-400 text-amber-400"
                                    : "text-gray-300 hover:text-gray-400"
                            }
                        />
                    </button>
                    <span className="text-[11px] font-medium text-gray-400 uppercase tracking-widest">
                        Name
                    </span>
                    <button
                        className="ml-auto flex items-center gap-1 text-[11px] font-medium text-gray-400 uppercase tracking-widest hover:text-gray-600 transition-colors duration-150"
                        onClick={() => setModifiedUp((prev) => !prev)}
                    >
                        Modified
                        {modifiedUp ? (
                            <ChevronDown size={12} />
                        ) : (
                            <ChevronUp size={12} />
                        )}
                    </button>
                </div>

                {/* Project list */}
                <div className="flex-1 min-h-0 overflow-y-auto">
                    <div className="flex flex-col px-5 py-3 gap-1.5">
                        {filteredProjects.length === 0 && (
                            <p className="text-center text-gray-400 text-sm mt-16">
                                No projects found.
                            </p>
                        )}
                        {filteredProjects.map((project, index) => (
                            <div
                                key={index}
                                className="group flex flex-row bg-white rounded-xl border border-gray-200/80 px-4 py-3 gap-4 items-center cursor-pointer hover:border-gray-300 hover:shadow-sm transition-all duration-150"
                                onClick={() => console.log("Opening project.")}
                            >
                                {/* Star */}
                                <button
                                    onClick={(e) => {
                                        e.stopPropagation();
                                        setProjects((prev) =>
                                            prev.map((p) =>
                                                p.id === project.id
                                                    ? {
                                                          ...p,
                                                          starred: !p.starred,
                                                      }
                                                    : p,
                                            ),
                                        );
                                    }}
                                    className="transition-colors duration-150 shrink-0"
                                >
                                    <Star
                                        size={14}
                                        className={
                                            project.starred
                                                ? "fill-amber-400 text-amber-400"
                                                : "text-gray-200 group-hover:text-gray-300"
                                        }
                                    />
                                </button>

                                {/* Folder icon */}
                                <div className="w-8 h-8 rounded-lg bg-gray-50 border border-gray-100 flex items-center justify-center shrink-0">
                                    <Folder
                                        size={14}
                                        strokeWidth={1.8}
                                        className="text-gray-400"
                                    />
                                </div>

                                {/* Name + path */}
                                <div className="flex flex-col min-w-0">
                                    <span className="text-sm font-medium text-gray-800 leading-snug">
                                        {project.name}
                                    </span>
                                    <span className="text-xs text-gray-400 font-mono truncate mt-0.5">
                                        {project.path}
                                    </span>
                                </div>

                                {/* Time */}
                                <span className="ml-auto text-xs text-gray-400 shrink-0 tabular-nums">
                                    {timeAgo(project.modified)}
                                </span>
                            </div>
                        ))}
                    </div>
                </div>
            </div>
        );
    }

    return (
        <main className="flex flex-row bg-[#f7f7f8]">
            <div className="w-full h-7.5 [app-region:drag] fixed top-0 left-0 z-50" />

            {/* Sidebar */}
            <nav className="bg-[#efefef] border-r border-gray-200 h-screen w-56 flex flex-col px-3 pt-10 pb-4 fixed">
                {/* Logo */}
                <div className="flex flex-row items-center mb-5 px-2">
                    <div
                        className="relative w-5 h-5 mr-2.5"
                        style={{ filter: "drop-shadow(0 0 0px transparent)" }}
                        onMouseEnter={(e) => {
                            const el = e.currentTarget as HTMLDivElement;
                            el.style.transition =
                                "filter 0.3s ease, transform 0.3s ease";
                            el.style.filter = [
                                "drop-shadow(0 0 6px rgba(255, 180, 255, 0.9))",
                                "drop-shadow(0 0 14px rgba(200, 140, 255, 0.7))",
                                "drop-shadow(0 0 28px rgba(160, 220, 255, 0.5))",
                            ].join(" ");
                            el.style.transform = "scale(1.08)";
                        }}
                        onMouseLeave={(e) => {
                            const el = e.currentTarget as HTMLDivElement;
                            el.style.transition =
                                "filter 0.4s ease, transform 0.4s ease";
                            el.style.filter =
                                "drop-shadow(0 0 0px transparent)";
                            el.style.transform = "scale(1)";
                        }}
                    >
                        <img
                            src="../../assets/atlasBall.png"
                            alt="Atlas Engine Logo"
                            className="w-5 h-5"
                        />
                    </div>
                    <h1 className="font-semibold text-[15px] tracking-tight text-gray-800 select-none">
                        Atlas Engine
                    </h1>
                </div>

                {/* Nav items */}
                <div className="flex flex-col flex-1">
                    {renderOption("Projects", 0, Folder)}
                    {renderOption("Learn", 1, GraduationCap)}
                    {renderOption("Settings", 2, Settings)}
                </div>
            </nav>

            <div className="w-screen h-screen">
                {page === 0 && projectsPage()}
                {page === 1 && (
                    <div className="flex-1 ml-56 flex items-center justify-center h-full text-sm text-gray-400">
                        Learn
                    </div>
                )}
                {page === 2 && (
                    <div className="flex-1 ml-56 flex items-center justify-center h-full text-sm text-gray-400">
                        Settings
                    </div>
                )}
            </div>
        </main>
    );
}
