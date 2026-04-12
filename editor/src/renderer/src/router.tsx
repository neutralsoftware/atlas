import { createHashRouter, RouterProvider } from "react-router-dom";
import TestPage from "./views/Test";

const router = createHashRouter([{ path: "/test", Component: TestPage }]);

export default function AppRouter() {
    return <RouterProvider router={router} />;
}
