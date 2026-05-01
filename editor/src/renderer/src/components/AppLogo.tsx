import { useEffect, useState } from "react";
import { AppInfo } from "../model/app";

type Props = {
    className?: string;
};

export default function AppLogo({ className }: Props) {
    const [appInfo, setAppInfo] = useState<AppInfo>({
        debug: false,
        buildId: "",
        platform: "",
    });

    useEffect(() => {
        window.app.getAppInfo().then(setAppInfo);
    }, []);

    return (
        <div>
            {(!appInfo.debug && (
                <img
                    src="./iconRelease.png"
                    className={className}
                    alt="App Logo"
                />
            )) || (
                <img
                    src="./iconDebug.png"
                    alt="App Logo"
                    className={className}
                />
            )}
        </div>
    );
}
