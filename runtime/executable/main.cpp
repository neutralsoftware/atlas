//
// main.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Main part of the main app runtime
// Copyright (c) 2026 Max Van den Eynde
//

#include "atlas/runtime/c_api.h"
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <string>

int wmain(int argc, wchar_t **argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: atlasrun <project_file.atlas>\n");
        return 1;
    }
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argv[1],
                                        -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        return 1;
    std::string projectFile(static_cast<size_t>(size), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, argv[1], -1,
                            projectFile.data(), size, nullptr, nullptr) <= 0)
        return 1;
    return atlas_runtime_run_project(projectFile.c_str()) ? 0 : 1;
}
#else
int main(int argc, char **argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <project_file.atlas>\n", argv[0]);
        return 1;
    }
    return atlas_runtime_run_project(argv[1]) ? 0 : 1;
}
#endif
