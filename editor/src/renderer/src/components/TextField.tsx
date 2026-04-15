import clsx from "clsx";
import {
    forwardRef,
    type ComponentPropsWithoutRef,
    type ReactNode,
} from "react";
import { twMerge } from "tailwind-merge";

type Props = Omit<ComponentPropsWithoutRef<"input">, "size"> & {
    leading?: ReactNode;
    trailing?: ReactNode;
    inputClassName?: string;
    size?: "sm" | "md";
};

const fieldSizeClasses = {
    sm: {
        shell: "h-10 rounded-2xl px-3.5",
        input: "text-sm",
        side: "text-sm",
    },
    md: {
        shell: "h-11 rounded-2xl px-4",
        input: "text-[15px]",
        side: "text-sm",
    },
};

const TextField = forwardRef<HTMLInputElement, Props>(function TextField(
    {
        leading,
        trailing,
        inputClassName,
        className,
        size = "sm",
        disabled,
        ...props
    },
    ref,
) {
    const sizing = fieldSizeClasses[size];

    return (
        <span
            className={twMerge(
                clsx(
                    "group relative flex w-full items-center overflow-hidden border",
                    "bg-white/72 shadow-[0_12px_30px_-18px_rgba(15,23,42,0.26),inset_0_1px_0_rgba(255,255,255,0.94)] backdrop-blur-md",
                    "border-slate-200/80 transition-all duration-200 ease-out",
                    disabled
                        ? "cursor-not-allowed opacity-60"
                        : "hover:border-slate-300/90 hover:bg-white/82",
                    "focus-within:border-sky-300/90 focus-within:bg-white/88 focus-within:shadow-[0_16px_36px_-18px_rgba(14,165,233,0.24),inset_0_1px_0_rgba(255,255,255,0.98)]",
                    sizing.shell,
                    className,
                ),
            )}
        >
            <span className="pointer-events-none absolute inset-y-0 left-0 w-16 bg-gradient-to-r from-white/50 to-transparent" />

            {leading ? (
                <span
                    className={clsx(
                        "relative z-10 mr-2.5 flex shrink-0 items-center text-slate-400 transition-colors duration-200 group-focus-within:text-slate-600",
                        sizing.side,
                    )}
                >
                    {leading}
                </span>
            ) : null}

            <input
                {...props}
                ref={ref}
                disabled={disabled}
                className={twMerge(
                    clsx(
                        "relative z-10 w-full min-w-0 border-none bg-transparent font-medium text-slate-800 outline-none",
                        "placeholder:font-normal placeholder:text-slate-400",
                        "disabled:cursor-not-allowed disabled:text-slate-500 disabled:placeholder:text-slate-300",
                        sizing.input,
                        inputClassName,
                    ),
                )}
            />

            {trailing ? (
                <span
                    className={clsx(
                        "relative z-10 ml-2.5 flex shrink-0 items-center text-slate-400",
                        sizing.side,
                    )}
                >
                    {trailing}
                </span>
            ) : null}
        </span>
    );
});

export default TextField;
