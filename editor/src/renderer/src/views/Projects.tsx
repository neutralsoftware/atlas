import {
    Folder,
    GraduationCap,
    LucideIcon,
    Search,
    Settings,
} from "lucide-react";
import { useEffect, useState } from "react";
import Button from "../components/Button";
import TextField from "../components/TextField";

export default function Projects() {
    const [page, setPage] = useState<number>(0);
    const [search, setSearch] = useState<string>("");
    const [projects, setProjects] = useState<string[]>([]);

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

    useEffect(() => {}, []);

    function projectsPage() {
        return (
            <div className="flex-1 flex justify-center h-full flex-col">
                <div className="flex flex-row w-full p-7 items-center">
                    <h1 className="font-bold text-2xl">Projects</h1>
                    <div className="ml-8 w-70 max-w-sm">
                        <TextField
                            value={search}
                            onChange={(event) => setSearch(event.target.value)}
                            placeholder="Search projects"
                            leading={<Search size={16} strokeWidth={2.2} />}
                        />
                    </div>
                    <div className="ml-auto flex flex-row h-10 w-auto">
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
                <div className="h-full bg-red-500"></div>
            </div>
        );
    }

    return (
        <main className="flex flex-row">
            <div className="w-full h-7.5 [app-region:drag] absolute top-0 left-0 z-50"></div>
            <nav className="bg-gray-200 border-r-2 border-r-gray-300 h-screen w-60 flex flex-col p-5 pt-10">
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
