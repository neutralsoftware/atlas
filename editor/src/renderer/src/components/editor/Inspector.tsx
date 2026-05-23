export default function Inspector() {
    return (
        <main className="h-full w-full min-w-0 border-l border-slate-200 bg-white pt-10 text-slate-950">
            <div className="flex items-center justify-between border-b border-slate-200/80 px-4 pb-3 pt-3">
                <div className="min-w-0">
                    <h2 className="truncate text-sm font-black tracking-tight">
                        Inspector
                    </h2>
                    <p className="text-[10px] font-semibold uppercase tracking-[0.22em] text-slate-400">
                        Selection
                    </p>
                </div>
            </div>
            <div className="p-4 text-xs text-slate-500">
                Select an object to edit its properties.
            </div>
        </main>
    );
}
