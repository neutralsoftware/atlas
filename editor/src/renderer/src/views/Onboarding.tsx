import { useState } from "react";
import AppLogo from "../components/AppLogo";
import Button from "../components/Button";
import { onboardingData, setOnboardingData } from "../model/app";

export default function Onboarding() {
    const [step, setStep] = useState(0);
    const [runtimePath, setRuntimePath] = useState<string | null>(null);
    const [executablePath, setExecutablePath] = useState<string | null>(null);

    const nextStep = () => {
        setStep((prev) => Math.min(prev + 1, 4));
    };

    function selectRuntime() {
        window.app
            .fileDialog({
                title: "Select Atlas Runtime",
                buttonLabel: "Select Runtime",
                properties: ["openFile"],
                filters: [
                    { name: "Runtime", extensions: ["dylib"] },
                    { name: "All Files", extensions: ["*"] },
                ],
            })
            .then((paths) => {
                if (paths && paths.length > 0) {
                    setRuntimePath(paths[0] as string);
                }
            });
    }

    function selectExecutable() {
        window.app
            .fileDialog({
                title: "Select Atlas Executable",
                buttonLabel: "Select Executable",
                properties: ["openFile"],
                filters: [{ name: "All Files", extensions: ["*"] }],
            })
            .then((paths) => {
                if (paths && paths.length > 0) {
                    setExecutablePath(paths[0] as string);
                }
            });
    }

    function canContinue() {
        if (step === 4) {
            return runtimePath !== null && executablePath !== null;
        }
    }

    async function finishOnboarding() {
        setOnboardingData(runtimePath!, executablePath!);
        await window.app.storeOnboardingData(onboardingData);
        window.app.showWindow("splashOnboarding");
        window.app.destroyWindow("onboarding");
    }

    const welcomeMessage = `
    Welcome to Atlas Engine! We're excited to have you on board. This is an open-source project, and we encourage you to explore the codebase, contribute, and provide feedback. If you have any questions or need assistance, feel free to reach out to us on our GitHub repository or join our community forums. 
    Let's build something amazing together! Building games should be modern, fun, and accessible to everyone, and we're committed to making that a reality with Atlas Engine. 
    This is just the beginning of our journey, and we can't wait to see what you'll create with Atlas Engine. 
   
    Happy coding!
    `;

    return (
        <main className="h-screen w-screen overflow-hidden">
            <div
                className="flex h-full transition-transform duration-500 ease-in-out"
                style={{ transform: `translateX(-${step * 100}vw)` }}
            >
                <section className="flex h-screen w-screen shrink-0 flex-col items-center">
                    <AppLogo className="mt-20 h-28 w-28" />
                    <h2 className="mt-7 text-3xl font-bold">Welcome to</h2>
                    <h1 className="mt-2 text-4xl font-bold">Atlas Engine</h1>

                    <div className="mt-auto font-manrope font-bold">
                        <h1>by neutral software</h1>
                    </div>

                    <div className="mt-auto">
                        <Button
                            type="primary"
                            onClick={nextStep}
                            className="mb-10 shadow-2xl"
                        >
                            Get started
                        </Button>
                    </div>
                </section>

                <section className="flex h-screen w-screen shrink-0 flex-col items-center justify-center px-8">
                    <h1 className="text-3xl font-bold">
                        Message from the team
                    </h1>
                    <p className="mt-4 text-center text-xs">{welcomeMessage}</p>
                    <figure className="w-full flex flex-col items-center mt-10">
                        <img
                            src="../../assets/sign.jpg"
                            alt="Signature"
                            className="w-40"
                        ></img>
                        <figcaption className="text-[10px] ">
                            Max Van den Eynde, Founder and Director of Neutral
                            Software.
                        </figcaption>
                    </figure>

                    <div className="mt-10">
                        <Button type="primary" onClick={nextStep}>
                            Continue
                        </Button>
                    </div>
                </section>

                <section className="flex h-screen w-screen shrink-0 flex-col items-center justify-center px-8">
                    <h1 className="text-3xl font-bold">Choose a runtime</h1>

                    <div className="mt-10 w-full flex flex-col items-center gap-4">
                        <Button type="primary" onClick={nextStep}>
                            Download a runtime
                        </Button>

                        <Button
                            type="secondary"
                            onClick={() => {
                                nextStep();
                                nextStep();
                            }}
                        >
                            Specify an existing runtime path
                        </Button>
                    </div>
                </section>

                <section className="flex h-screen w-screen shrink-0 flex-col items-center justify-center px-8">
                    <h1 className="text-3xl font-bold">Choose a runtime</h1>

                    <div className="mt-10 w-full flex flex-col items-center gap-4">
                        <Button type="primary" onClick={nextStep}>
                            Downloading runtime
                        </Button>

                        <Button
                            type="secondary"
                            onClick={() => {
                                setStep(4);
                            }}
                        >
                            Specify an existing runtime path
                        </Button>
                    </div>
                </section>

                <section className="flex h-screen w-screen shrink-0 flex-col items-center justify-center px-8">
                    <h1 className="text-3xl font-bold">
                        Specify a runtime path
                    </h1>

                    <p className="mt-4 text-center text-xs">
                        If you already have a compatible runtime installed, you
                        can specify its path here to use it with Atlas Engine.
                    </p>

                    {runtimePath && (
                        <div className="mt-4 text-center text-xs">
                            Selected runtime path: {runtimePath}
                        </div>
                    )}

                    <Button
                        type="primary"
                        onClick={selectRuntime}
                        className="mt-10"
                    >
                        {runtimePath
                            ? "Change runtime path"
                            : "Select runtime path"}
                    </Button>

                    {executablePath && (
                        <div className="mt-4 text-center text-xs">
                            Selected executable path: {executablePath}
                        </div>
                    )}

                    <Button
                        type="primary"
                        onClick={selectExecutable}
                        className="mt-4"
                    >
                        {executablePath
                            ? "Change executable path"
                            : "Select executable path"}
                    </Button>

                    <div className="mt-10">
                        <Button
                            type={canContinue() ? "primary" : "inactive"}
                            onClick={finishOnboarding}
                        >
                            Continue
                        </Button>
                    </div>
                </section>
            </div>
        </main>
    );
}
