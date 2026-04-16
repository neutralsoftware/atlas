import clsx from "clsx";
import { twMerge } from "tailwind-merge";

type Props = {
    type: "primary" | "secondary" | "destructive" | "inactive" | "special";
    onClick: () => void;
    className?: string;
};

export default function Button({
    type,
    onClick,
    className,
    children,
}: React.PropsWithChildren<Props>) {
    const baseClasses =
        "rounded-[100px] px-4 py-2 font-medium transition-colors duration-200 disabled:cursor-not-allowed disabled:opacity-50 min-w-[252px] text-center min-h-[41px]";

    if (type === "special") {
        return (
            <>
                <style>{`
                @keyframes drift {
                    0%   { background-position: 0% 50%; }
                    50%  { background-position: 100% 50%; }
                    100% { background-position: 0% 50%; }
                }
                .btn-special {
                    position: relative;
                    background: #f5f4f8;
                    color: rgba(80, 60, 120, 0.5);
                    border: 1px solid rgba(180, 160, 220, 0.3);
                    transition: color 0.4s ease, border-color 0.4s ease, box-shadow 0.4s ease;
                    overflow: hidden;
                }
                .btn-special::before {
                    content: '';
                    position: absolute;
                    inset: 0;
                    border-radius: inherit;
                    background: linear-gradient(
                        120deg,
                        #ffe8b8, #ffcdf4, #ddc8ff, #bde4ff, #c8f5e8, #ffcdf4, #ffe8b8
                    );
                    background-size: 300% 300%;
                    opacity: 0;
                    transition: opacity 0.5s ease;
                    animation: drift 5s ease infinite;
                }
                .btn-special:hover::before {
                    opacity: 1;
                }
                .btn-special:hover {
                    color: rgba(60, 30, 90, 0.85);
                    border-color: transparent;
                    box-shadow:
                        0 0 10px rgba(255, 180, 230, 0.5),
                        0 0 22px rgba(200, 160, 255, 0.35),
                        0 0 40px rgba(160, 210, 255, 0.2),
                        inset 0 0 0 1px rgba(255,255,255,0.6);
                }
                .btn-special-inner {
                    position: relative;
                    z-index: 1;
                }
            `}</style>
                <button
                    className={twMerge(
                        clsx(baseClasses, "btn-special", className),
                    )}
                    onClick={onClick}
                >
                    <span className="btn-special-inner">{children}</span>
                </button>
            </>
        );
    }

    const typeClasses = {
        primary: "bg-accent text-white hover:bg-accent-hover",
        secondary: "bg-gray-300 text-gray-800 hover:bg-gray-400",
        destructive: "bg-red-600 text-white hover:bg-red-700",
        inactive: "bg-gray-300 text-gray-500 cursor-not-allowed",
    }[type];

    return (
        <button
            className={twMerge(clsx(baseClasses, typeClasses, className))}
            onClick={onClick}
        >
            {children}
        </button>
    );
}
