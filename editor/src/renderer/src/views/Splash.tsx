import { useEffect, useState } from "react";
import { AppInfo } from "../model/app";

export default function Splash() {
    const [appInfo, setAppInfo] = useState<AppInfo>({
        debug: false,
        buildId: "",
        platform: "",
    });

    useEffect(() => {
        window.app.getAppInfo().then(setAppInfo);
    }, []);

    return (
        <main className="flex h-full w-full items-center justify-center">
            <div className="bg-white h-40 w-175 rounded-[40px] flex items-center flex-row">
                <img
                    src="../../assets/iconRelease.png"
                    alt="App Icon"
                    className="w-25 h-25 ml-5 shrink-0"
                ></img>
                <div className="flex items-start flex-col ml-5 mt-2">
                    <h1 className="text-4xl font-bold">Atlas Engine</h1>
                    <h3 className="text-xl font-bold text-secondary mt-1.5">
                        Alpha 9
                    </h3>
                    <p className="text-xs font-manrope font-bold">
                        by neutral software
                    </p>
                    <div className="mt-2 text-[9px] text-secondary flex justify-center items-center">
                        Loading the engine...
                    </div>
                </div>
            </div>
        </main>
    );
}
