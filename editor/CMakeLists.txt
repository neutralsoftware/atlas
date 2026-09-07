
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
find_program(ATLAS_CARGO_EXECUTABLE cargo REQUIRED)

file(GLOB_RECURSE ATLAS_CLI_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/cli/src/*.rs"
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(ATLAS_EDITOR_CARGO_ARGS --release)
    set(ATLAS_EDITOR_CLI "${CMAKE_SOURCE_DIR}/target/release/atlas")
else ()
    set(ATLAS_EDITOR_CARGO_ARGS)
    set(ATLAS_EDITOR_CLI "${CMAKE_SOURCE_DIR}/target/debug/atlas")
endif ()

add_custom_command(
        OUTPUT "${ATLAS_EDITOR_CLI}"
        COMMAND
        "${CMAKE_COMMAND}" -E env CARGO_TERM_COLOR=always
        "${ATLAS_CARGO_EXECUTABLE}" build
        ${ATLAS_EDITOR_CARGO_ARGS}
        --manifest-path "${CMAKE_SOURCE_DIR}/cli/Cargo.toml"
        DEPENDS
        "${CMAKE_SOURCE_DIR}/Cargo.toml"
        "${CMAKE_SOURCE_DIR}/Cargo.lock"
        "${CMAKE_SOURCE_DIR}/cli/Cargo.toml"
        ${ATLAS_CLI_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Building Atlas CLI for editor project export"
        VERBATIM
)

add_custom_target(atlas_editor_cli DEPENDS "${ATLAS_EDITOR_CLI}")

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

if (APPLE)
    set(ATLAS_APP_ICON
                CACHE FILEPATH "Atlas Engine macOS bundle icon"
    )
    get_filename_component(ATLAS_APP_ICON_NAME "${ATLAS_APP_ICON}" NAME)
    set_source_files_properties("${ATLAS_APP_ICON}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
    )
    list(APPEND EDITOR_FILES "${ATLAS_APP_ICON}")
endif ()

qt_add_executable(AtlasEditor ${EDITOR_FILES})
add_dependencies(AtlasEditor generate_atlas_themes atlas_editor_cli)

if (APPLE)
    set_target_properties(AtlasEditor PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "Atlas Engine"
            MACOSX_BUNDLE_GUI_IDENTIFIER "neutralsoftware.atlas"
            MACOSX_BUNDLE_ICON_FILE "${ATLAS_APP_ICON_NAME}"
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/editor/macos/Info.plist.in"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "0.9.0"
            MACOSX_BUNDLE_BUNDLE_VERSION "10"
            OUTPUT_NAME "Atlas Engine"
            BUILD_RPATH "@executable_path/../Frameworks"
            INSTALL_RPATH "@executable_path/../Frameworks"
    )
    add_custom_command(TARGET AtlasEditor POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_DIR:AtlasEditor>/Contents/Helpers"
            "$<TARGET_BUNDLE_DIR:AtlasEditor>/Contents/Frameworks"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${ATLAS_EDITOR_CLI}"
            "$<TARGET_BUNDLE_DIR:AtlasEditor>/Contents/Helpers/atlas"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:runtime_lib>"
            "$<TARGET_BUNDLE_DIR:AtlasEditor>/Contents/Frameworks/runtime.dylib"
            VERBATIM
    )
else ()
    add_custom_command(TARGET AtlasEditor POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${ATLAS_EDITOR_CLI}"
            "$<TARGET_FILE_DIR:AtlasEditor>/atlas"
            VERBATIM
    )
endif ()

qt_add_resources(AtlasEditor "atlas_editor_assets"
        PREFIX "/editor"
        BASE "${CMAKE_SOURCE_DIR}/editor"
        FILES
        "${CMAKE_SOURCE_DIR}/editor/assets/atlas-app-dev.png"
        "${CMAKE_SOURCE_DIR}/editor/assets/atlas-star.png"
        "${CMAKE_SOURCE_DIR}/editor/assets/check.svg"
        "${CMAKE_SOURCE_DIR}/editor/assets/atlas-app.png"
        "${CMAKE_SOURCE_DIR}/editor/assets/Manrope-VariableFont_wght.ttf"
        "${CMAKE_SOURCE_DIR}/editor/assets/Phosphor.ttf"
        "${CMAKE_SOURCE_DIR}/editor/assets/PHOSPHOR_LICENSE.txt"
)

target_include_directories(AtlasEditor
        PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/QtDockingSystem/src
        ${ATLAS_DEP_INCLUDE_DIRS}
)

string(TIMESTAMP ATLAS_EDITOR_BUILD_DATE "%Y%m%d")

target_compile_definitions(AtlasEditor PRIVATE
        ATLAS_TOOLCHAIN_VERSION="beta1"
        "$<$<CONFIG:Debug>:ATLAS_DEBUG_BUILD>"
        "$<$<NOT:$<CONFIG:Debug>>:ATLAS_RELEASE_BUILD>"
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
            "-framework CoreGraphics"
            "-framework IOKit"
    )
endif ()
