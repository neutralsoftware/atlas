//
// main.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2026
// --------------------------------------------------
// Description: Main part of the main app runtime
// Copyright (c) 2026 Max Van den Eynde
//

#include "atlas/runtime/context.h"
#include "atlas/window.h"
#include <cstdio>
#include <exception>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <project_file.atlas>\n", argv[0]);
        return 1;
    }
    try {
        auto context = runtime::makeContext(argv[1]);
        context->loadProject();
        context->runWindowed();
    } catch (const std::exception &error) {
        std::fprintf(stderr, "Atlas runtime error: %s\n", error.what());
        return 1;
    }
    return 0;
}
