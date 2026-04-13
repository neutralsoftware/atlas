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

    const debugMessage = `
    As this software is in its development version issues may be found with the experience. If you meant to use the traditional version please access:
    https://atlasengine.org to get the official builds. In development builds, the engine may require you to have a runtime already installed. Therefore, make sure
    that you have an appropiate runtime in your system that works with this version. 
    `;

    return (
        <main className="flex h-screen w-screen items-center justify-center">
            {(!appInfo.debug && (
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
            )) || (
                <div className="bg-white h-52.5 w-175 rounded-[40px] flex py-7 px-2 flex-col">
                    <div className="flex flex-row">
                        <img
                            src="../../assets/iconDebug.png"
                            alt="App Icon"
                            className="w-25 h-25 ml-5 shrink-0"
                        ></img>
                        <div className="flex items-start flex-col ml-5 mt-1">
                            <h1 className="text-4xl font-bold">
                                Atlas Engine (Development)
                            </h1>
                            <h3 className="text-xl font-bold text-secondary mt-1.5">
                                {`Alpha 9 (build ${appInfo.buildId})`}
                            </h3>
                            <p className="text-xs font-manrope font-bold">
                                by neutral software
                            </p>
                            <div className="mt-2 text-[9px] text-secondary flex justify-center items-center">
                                Loading the engine...
                            </div>
                        </div>
                    </div>
                    <div className="mt-2 text-[9px] text-secondary flex justify-center items-center ml-5">
                        {debugMessage}
                    </div>
                </div>
            )}
        </main>
    );
}
