import React from "react";
import ReactDOM from "react-dom/client";
import "./styles.css";
import AppRouter from "./router";

ReactDOM.createRoot(document.getElementById("root")!).render(
    <React.StrictMode>
        <AppRouter />
    </React.StrictMode>,
);
