import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";
import path from "node:path";

export default defineConfig({
    root: "src/renderer",
    base: "./",
    plugins: [react(), tailwindcss()],
    resolve: {
        alias: {
            "@shared": path.resolve(__dirname, "src/shared"),
        },
    },
    build: {
        outDir: "../../dist/renderer",
        emptyOutDir: true,
    },
    server: {
        port: 5173,
        strictPort: true,
    },
});
