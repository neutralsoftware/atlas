import { createHashRouter, RouterProvider } from "react-router-dom";
import TestPage from "./views/Test";
import Splash from "./views/Splash";
import Onboarding from "./views/Onboarding";
import Projects from "./views/Projects";
import EditorSplash from "./views/EditorSplash";
import { CreateProject } from "./views/modal/CreateProject";
import EditorOverlay from "./views/EditorOverlay";

const router = createHashRouter([
    { path: "/test", Component: TestPage },
    { path: "/onboarding", Component: Onboarding },
    { path: "/splash", Component: Splash },
    { path: "/projects", Component: Projects },
    { path: "/createProject", Component: CreateProject },
    { path: "/editorSplash", Component: EditorSplash },
    { path: "/editorOverlay", Component: EditorOverlay },
]);

export default function AppRouter() {
    return <RouterProvider router={router} />;
}
