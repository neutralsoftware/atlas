type Props = {
    type: "primary" | "secondary" | "destructive";
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
    const typeClasses = {
        primary: "bg-accent text-white hover:bg-accent-hover",
        secondary: "bg-gray-300 text-gray-800 hover:bg-gray-400",
        destructive: "bg-red-600 text-white hover:bg-red-700",
    }[type];

    return (
        <button
            className={`${baseClasses} ${typeClasses} ${className || ""}`}
            onClick={onClick}
        >
            {children}
        </button>
    );
}
