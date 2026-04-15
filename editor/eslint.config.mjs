import js from "@eslint/js";
import globals from "globals";
import tseslint from "typescript-eslint";
import reactHooks from "eslint-plugin-react-hooks";
import reactRefresh from "eslint-plugin-react-refresh";
import prettier from "eslint-config-prettier";

export default tseslint.config(
    {
        ignores: ["dist", "dist-electron", "release", "release-dev", "main.js"],
    },

    js.configs.recommended,
    ...tseslint.configs.recommended,

    {
        files: ["src/renderer/src/**/*.{ts,tsx}"],
        languageOptions: {
            globals: {
                ...globals.browser,
            },
        },
        plugins: {
            "react-hooks": reactHooks,
            "react-refresh": reactRefresh,
        },
        rules: {
            ...reactHooks.configs.recommended.rules,
            "react-refresh/only-export-components": "warn",
        },
    },

    {
        files: ["src/**/*.{ts,tsx,js,jsx}"],
        languageOptions: {
            globals: {
                ...globals.node,
            },
        },
    },

    prettier,
);
