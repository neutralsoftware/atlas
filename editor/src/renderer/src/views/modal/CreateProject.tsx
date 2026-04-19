import { ChevronLeft, ChevronRight } from "lucide-react";
import { useState } from "react";
import Button from "../../components/Button";

export function CreateProject() {
    const [option, setOption] = useState<number>(0);
    const [step, setStep] = useState<number>(0);

    const options = [
        {
            name: "Path Tracing",
            description:
                "Create a project with hyperrealistic lighting that consumes significant computational resources.",
            image: "../../../assets/pathtracing.png",
            color: "bg-red-500",
        },
        {
            name: "PBR (Physically Based Rendering)",
            description:
                "Create a project with a good balance of visual quality and performance, suitable for most applications.",
            image: "../../../assets/pbr.png",
            color: "bg-green-500",
        },
        {
            name: "PBR and Global Illumination",
            description:
                "Create a project with enhanced lighting effects that combines PBR with real-time global illumination techniques.",
            image: "../../../assets/gi.png",
            color: "bg-blue-500",
        },
    ];

    function typeCard(option: {
        name: string;
        description: string;
        image: string;
        color: string;
    }) {
        return (
            <div
                className={`h-90 w-90 mt-2 overflow-hidden rounded-3xl flex flex-col items-center ${option.color}`}
            >
                <div className="h-full w-full overflow-hidden">
                    <img
                        src={option.image}
                        className="w-full h-full object-cover"
                    ></img>
                </div>
                <div
                    className={`flex flex-col items-center h-20 mt-2 ${option.color}`}
                >
                    <p className="font-bold text-white">{option.name}</p>
                    <p className="text-white text-xs text-center">
                        {option.description}
                    </p>
                </div>
            </div>
        );
    }

    function firstStep() {
        return (
            <div className="flex flex-col h-screen w-screen items-center pt-8 ">
                <p className="font-bold ">Create a project</p>
                <div className="flex flex-row justify-center items-center">
                    <button onClick={() => option > 0 && setOption(option - 1)}>
                        {option > 0 ? (
                            <ChevronLeft className="cursor-pointer"></ChevronLeft>
                        ) : (
                            <ChevronLeft className="opacity-50"></ChevronLeft>
                        )}
                    </button>
                    {typeCard(options[option]!)}
                    <button
                        onClick={() =>
                            option < options.length - 1 && setOption(option + 1)
                        }
                    >
                        {option < options.length - 1 ? (
                            <ChevronRight className="cursor-pointer"></ChevronRight>
                        ) : (
                            <ChevronRight className="opacity-50"></ChevronRight>
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
                    Create
                </Button>
            </div>
        );
    }

    return <main>{step === 0 && firstStep()}</main>;
}
