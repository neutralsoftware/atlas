
# Finding all Editor Files
file(GLOB_RECURSE EDITOR_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/editor/*"
        "${CMAKE_SOURCE_DIR}/include/editor/*.h"
)

file(GLOB_RECURSE ATLAS_QSS_FILES
        CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/editor/styling/*.qss"
)

set(ATLAS_GENERATED_THEME_HEADER
        "${CMAKE_SOURCE_DIR}/include/editor/core/themes.h"
)

# Find packages

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Compile all Qt themes into one header file

add_custom_command(
        OUTPUT "${ATLAS_GENERATED_THEME_HEADER}"
        COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/scripts/pack_qt_themes.py"
        "${ATLAS_GENERATED_THEME_HEADER}"
        ${ATLAS_QSS_FILES}
        DEPENDS
        "${CMAKE_SOURCE_DIR}/scripts/pack_qt_themes.py"
        ${ATLAS_QSS_FILES}
        COMMENT "Generating embedded QSS theme header"
        VERBATIM
)

add_custom_target(generate_atlas_themes
        DEPENDS "${ATLAS_GENERATED_THEME_HEADER}"
)

# Setting up the project

qt_standard_project_setup()

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(
        "${CMAKE_SOURCE_DIR}/extern/QtDockingSystem"
        "${CMAKE_BINARY_DIR}/extern/QtDockingSystem"
)

qt_add_executable(AtlasEditor ${EDITOR_FILES})
add_dependencies(AtlasEditor generate_atlas_themes)

qt_add_resources(AtlasEditor "atlas_editor_assets"
        PREFIX "/editor"
        BASE "${CMAKE_SOURCE_DIR}/editor"
        FILES
        "${CMAKE_SOURCE_DIR}/editor/assets/Icon-iOS-Default-1024x1024@1x.png"
        "${CMAKE_SOURCE_DIR}/editor/assets/iconFile-iOS-Dark-1024x1024@1x.png"
        "${CMAKE_SOURCE_DIR}/editor/assets/Manrope-VariableFont_wght.ttf"
)

target_include_directories(AtlasEditor
        PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/QtDockingSystem/src
        ${ATLAS_DEP_INCLUDE_DIRS}
)

string(TIMESTAMP ATLAS_EDITOR_BUILD_DATE "%Y%m%d")

target_compile_definitions(AtlasEditor PRIVATE
        "$<$<CONFIG:Debug>:ATLAS_DEBUG_BUILD>"
        "$<$<CONFIG:Debug>:ATLAS_BUILD_STRING=\"${ATLAS_EDITOR_BUILD_DATE}\">"
)

# Linking libraries

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
