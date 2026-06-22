
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/extern/imgui)

file(GLOB_RECURSE EDITOR_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/editor/*"
)

if (APPLE)
    message("Configuring editor for macOS")
else ()
    message(FATAL_ERROR "Cannot build editor for other platforms")
endif ()

add_executable(AtlasEditor ${EDITOR_FILES}
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp

        ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_metal.mm
)

target_include_directories(AtlasEditor
        PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${CMAKE_SOURCE_DIR}/extern
        ${CMAKE_SOURCE_DIR}/include
        ${ATLAS_DEP_INCLUDE_DIRS}
        ${SDL3_SOURCE_DIR}/include
)

target_compile_definitions(AtlasEditor
        PRIVATE
        ATLAS_DEFAULT_PROJECT_FILE="${CMAKE_SOURCE_DIR}/tests/simple/project.atlas"
)

if (NOT BEZEL_NATIVE)
    target_compile_definitions(AtlasEditor PRIVATE
            JPH_PROFILE_ENABLED=1
            JPH_DEBUG_RENDERER=1
            JPH_OBJECT_STREAM=1
    )
endif ()

# Pick SDL static target.
set(ATLAS_EDITOR_SDL "")
if (TARGET SDL3::SDL3-static)
    set(ATLAS_EDITOR_SDL SDL3::SDL3-static)
elseif (TARGET SDL3::SDL3)
    set(ATLAS_EDITOR_SDL SDL3::SDL3)
elseif (TARGET SDL3-static)
    set(ATLAS_EDITOR_SDL SDL3-static)
else ()
    message(FATAL_ERROR "Could not determine SDL3 target")
endif ()

target_link_libraries(AtlasEditor
        PRIVATE
        runtime_lib
        atlas
        opal
        bezel
        finewave
        photon
        aurora
        hydra
        graphite
        quickjs
        fmt::fmt
        ${ATLAS_FREETYPE_TARGET}
        ${ATLAS_ASSIMP_TARGET}
        ${ATLAS_OPENAL_TARGET}
        ${ATLAS_GLM_TARGET}
        ${ATLAS_EDITOR_SDL}
)

if (NOT BEZEL_NATIVE)
    target_link_libraries(AtlasEditor PRIVATE Jolt::Jolt)
endif ()

if (APPLE)
    target_link_libraries(AtlasEditor
            PUBLIC
            "-framework Metal"
            "-framework MetalKit"
            "-framework MetalFX"
            "-framework QuartzCore"
            "-framework Foundation"
            "-framework Cocoa"
            "-framework IOKit"
    )
endif ()
