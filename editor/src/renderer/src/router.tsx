import { createHashRouter, RouterProvider } from "react-router-dom";
import TestPage from "./views/Test";
import Splash from "./views/Splash";
import Onboarding from "./views/Onboarding";
import Projects from "./views/Projects";

const router = createHashRouter([
    { path: "/test", Component: TestPage },
    { path: "/onboarding", Component: Onboarding },
    { path: "/splash", Component: Splash },
    { path: "/projects", Component: Projects },
]);

export default function AppRouter() {
    return <RouterProvider router={router} />;
}
