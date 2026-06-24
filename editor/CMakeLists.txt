
file(GLOB_RECURSE EDITOR_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/editor/*"
)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(
        "${CMAKE_SOURCE_DIR}/extern/QtDockingSystem"
        "${CMAKE_BINARY_DIR}/extern/QtDockingSystem"
)

qt_add_executable(AtlasEditor ${EDITOR_FILES})

target_include_directories(AtlasEditor
        PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/QtDockingSystem/src
        ${ATLAS_DEP_INCLUDE_DIRS}
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
        Qt6::Widgets
        ads::qtadvanceddocking-qt6
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
