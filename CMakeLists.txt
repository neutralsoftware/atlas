cmake_minimum_required(VERSION 3.21)
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

project(Atlas LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(JOLT_VERSION "5.5.0")

message(STATUS "Using Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Compiler at path:" ${CMAKE_CXX_COMPILER})

add_compile_definitions(
    ATLAS_VERSION="Alpha 9"
    OPAL_VERSION="3 Tetrahedron"
)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

# ─── Compiler Diagnostics ────────────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fcolor-diagnostics)
endif()
add_library(atlas_warnings INTERFACE)

if(MSVC)
    target_compile_options(atlas_warnings INTERFACE
        /W4
        /WX
        /permissive-
    )
else()
    target_compile_options(atlas_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wno-gnu-anonymous-struct
    )
endif()

# ─── Build Type ──────────────────────────────────────────────────────────────
message(STATUS "Building for platform: ${CMAKE_SYSTEM_NAME}")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ─── Output Directories ──────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ─── Rendering Backend ───────────────────────────────────────────────────────
set(BACKEND "AUTO" CACHE STRING "Rendering backend: AUTO, VULKAN, METAL, OPENGL")
set_property(CACHE BACKEND PROPERTY STRINGS AUTO VULKAN METAL OPENGL)
string(TOUPPER "${BACKEND}" BACKEND)

if(BACKEND STREQUAL "AUTO")
    if(APPLE)
        set(BACKEND "METAL")
    else()
        set(BACKEND "VULKAN")
    endif()
endif()

set(BACKEND_OPENGL OFF)
set(BACKEND_VULKAN OFF)
set(BACKEND_METAL  OFF)

if(BACKEND STREQUAL "OPENGL")
    set(BACKEND_OPENGL ON)
elseif(BACKEND STREQUAL "VULKAN")
    set(BACKEND_VULKAN ON)
elseif(BACKEND STREQUAL "METAL")
    set(BACKEND_METAL ON)
else()
    message(FATAL_ERROR "Invalid BACKEND='${BACKEND}'. Valid values are: AUTO, VULKAN, METAL, OPENGL")
endif()

message(STATUS "Selected backend: ${BACKEND}")

# ─── Options ─────────────────────────────────────────────────────────────────
option(BEZEL_NATIVE "Enable Bezel Native" OFF)

# Force static third-party builds where possible.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies as static libraries" FORCE)

# ─── FetchContent ────────────────────────────────────────────────────────────
include(FetchContent)
cmake_policy(SET CMP0135 NEW)

# Doxygen CSS
FetchContent_Declare(
    doxygen-awesome-css
    URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/tags/v2.4.0.zip
)
FetchContent_MakeAvailable(doxygen-awesome-css)
FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)

# glm (header-only)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

# fmt
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 12.1.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(fmt)

# SDL3
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.10
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

# Freetype
set(FT_DISABLE_ZLIB OFF CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    freetype
    URL https://download.savannah.gnu.org/releases/freetype/freetype-2.13.3.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)

FetchContent_GetProperties(freetype)
if(NOT freetype_POPULATED)
    FetchContent_Populate(freetype)
    add_subdirectory(${freetype_SOURCE_DIR} ${freetype_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Assimp
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v6.0.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(assimp)

# OpenAL Soft
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
set(ALSOFT_CONFIG OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(LIBTYPE STATIC CACHE STRING "" FORCE)
FetchContent_Declare(
    openal_soft
    GIT_REPOSITORY https://github.com/kcat/openal-soft.git
    GIT_TAG 1.24.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(openal_soft)

# Jolt
if(NOT BEZEL_NATIVE)
    FetchContent_Declare(
        JoltPhysics
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v${JOLT_VERSION}
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR Build
    )
    set(JPH_DEBUG_RENDERER  ON  CACHE BOOL "" FORCE)
    set(JPH_PROFILE_ENABLED ON  CACHE BOOL "" FORCE)
    set(JPH_OBJECT_STREAM   ON  CACHE BOOL "" FORCE)
    set(JPH_ENABLE_IPO      OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(JoltPhysics)
    if(TARGET Jolt)
        set_target_properties(Jolt PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
    endif()
    message(STATUS "Downloaded Jolt Physics, version ${JOLT_VERSION}")
endif()

# ─── FetchContent include roots ──────────────────────────────────────────────
set(ATLAS_DEP_INCLUDE_DIRS
    ${glm_SOURCE_DIR}
    ${SDL3_SOURCE_DIR}/include
    ${freetype_SOURCE_DIR}/include
    ${assimp_SOURCE_DIR}/include
    ${openal_soft_SOURCE_DIR}/include
)

if(NOT BEZEL_NATIVE)
    list(APPEND ATLAS_DEP_INCLUDE_DIRS ${joltphysics_SOURCE_DIR})
endif()

# ─── Backend Definitions ─────────────────────────────────────────────────────
if(BACKEND_OPENGL)
    add_compile_definitions(OPENGL)
    message(STATUS "Building using OpenGL backend")
elseif(BACKEND_VULKAN)
    add_compile_definitions(VULKAN GLM_FORCE_DEPTH_ZERO_TO_ONE)
    message(STATUS "Building using Vulkan backend")
elseif(BACKEND_METAL)
    add_compile_definitions(METAL GLM_FORCE_DEPTH_ZERO_TO_ONE)
    message(STATUS "Building using Metal backend")
endif()

# ─── Shader Generation ───────────────────────────────────────────────────────
if(BACKEND_OPENGL)
    set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/shaders/opengl")
elseif(BACKEND_VULKAN)
    set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/shaders/vulkan")
elseif(BACKEND_METAL)
    set(SHADER_INPUT_DIR "${CMAKE_SOURCE_DIR}/shaders/metal")
endif()

set(SHADER_OUTPUT_FILE "${CMAKE_SOURCE_DIR}/include/atlas/core/default_shaders.h")
set(RUNTIME_SCRIPT_INPUT_DIR "${CMAKE_SOURCE_DIR}/runtime/scripts")
set(RUNTIME_SCRIPT_OUTPUT_FILE "${CMAKE_SOURCE_DIR}/include/atlas/runtime/atlasScripts.h")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

file(GLOB_RECURSE SHADER_SOURCES
    ${SHADER_INPUT_DIR}/*.vert
    ${SHADER_INPUT_DIR}/*.frag
    ${SHADER_INPUT_DIR}/*.glsl
    ${SHADER_INPUT_DIR}/*.geom
    ${SHADER_INPUT_DIR}/*.tesc
    ${SHADER_INPUT_DIR}/*.tese
    ${SHADER_INPUT_DIR}/*.metal
)

file(GLOB_RECURSE RUNTIME_SCRIPT_SOURCES CONFIGURE_DEPENDS
    ${RUNTIME_SCRIPT_INPUT_DIR}/*.js
    ${RUNTIME_SCRIPT_INPUT_DIR}/*.mjs
)

if(BACKEND_OPENGL)
    set(SHADER_BACKEND_ARG "opengl")
elseif(BACKEND_VULKAN)
    set(SHADER_BACKEND_ARG "vulkan")
elseif(BACKEND_METAL)
    set(SHADER_BACKEND_ARG "metal")
endif()

message(STATUS "Packing shaders for ${BACKEND} backend")
add_custom_command(
    OUTPUT  ${SHADER_OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/include/atlas/core
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/pack_shaders.py
            ${SHADER_INPUT_DIR} ${SHADER_OUTPUT_FILE} ${SHADER_BACKEND_ARG}
    DEPENDS ${SHADER_SOURCES}
    COMMENT "Packing shaders into default_shaders.h"
    VERBATIM
)

add_custom_target(generate_shaders DEPENDS ${SHADER_OUTPUT_FILE})

add_custom_command(
    OUTPUT  ${RUNTIME_SCRIPT_OUTPUT_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/include/atlas/runtime
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/pack_runtime_scripts.py
            ${RUNTIME_SCRIPT_INPUT_DIR} ${RUNTIME_SCRIPT_OUTPUT_FILE}
    DEPENDS ${RUNTIME_SCRIPT_SOURCES} ${CMAKE_SOURCE_DIR}/pack_runtime_scripts.py
    COMMENT "Packing runtime scripts into atlasScripts.h"
    VERBATIM
)

add_custom_target(generate_runtime_scripts DEPENDS ${RUNTIME_SCRIPT_OUTPUT_FILE})

# ─── Typst Docs ──────────────────────────────────────────────────────────────
find_program(TYPST_EXECUTABLE typst)
if(TYPST_EXECUTABLE)
    message(STATUS "Found Typst: ${TYPST_EXECUTABLE}")
    file(GLOB_RECURSE TYPST_SOURCES ${CMAKE_SOURCE_DIR}/*.typ)
    if(TYPST_SOURCES)
        set(TYPST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/docs)
        file(MAKE_DIRECTORY ${TYPST_OUTPUT_DIR})
        set(TYPST_OUTPUTS "")
        foreach(TYPST_FILE ${TYPST_SOURCES})
            get_filename_component(TYPST_NAME ${TYPST_FILE} NAME_WE)
            set(PDF_OUTPUT ${TYPST_OUTPUT_DIR}/${TYPST_NAME}.pdf)
            add_custom_command(
                OUTPUT  ${PDF_OUTPUT}
                COMMAND ${TYPST_EXECUTABLE} compile ${TYPST_FILE} ${PDF_OUTPUT}
                DEPENDS ${TYPST_FILE}
                COMMENT "Compiling Typst document: ${TYPST_NAME}.typ -> ${TYPST_NAME}.pdf"
                VERBATIM
            )
            list(APPEND TYPST_OUTPUTS ${PDF_OUTPUT})
        endforeach()
        add_custom_target(typst_docs ALL DEPENDS ${TYPST_OUTPUTS})
        message(STATUS "Typst documents will be compiled to: ${TYPST_OUTPUT_DIR}")
    else()
        message(STATUS "No Typst documents found")
    endif()
else()
    message(WARNING "Typst not found - skipping document compilation")
endif()

# ─── Platform Packages ───────────────────────────────────────────────────────
if(BACKEND_OPENGL)
    find_package(OpenGL REQUIRED)
elseif(BACKEND_VULKAN)
    find_package(Vulkan REQUIRED)
    message(STATUS "Found Vulkan SDK at ${Vulkan_LIBRARY}")
    message(STATUS "Found Vulkan include dirs at ${Vulkan_INCLUDE_DIRS}")

    # Optional: still find spirv-cross through package manager / SDK.
    # Convert this to FetchContent too later if you want fully source-based Vulkan deps.
    if(APPLE)
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/spirv-cross")
    endif()
    find_package(spirv_cross_core REQUIRED)
    find_package(spirv_cross_glsl REQUIRED)
    find_package(spirv_cross_reflect REQUIRED)
elseif(BACKEND_METAL)
    message(STATUS "Using Metal backend")
endif()

# ─── Global Includes ─────────────────────────────────────────────────────────
include_directories(
    include
    extern
    extern/quickjs
)

if(NOT BEZEL_NATIVE)
    include_directories(${joltphysics_SOURCE_DIR})
endif()

# ─── Sources ─────────────────────────────────────────────────────────────────
if(BEZEL_NATIVE)
    file(GLOB_RECURSE BEZEL_SOURCES CONFIGURE_DEPENDS bezel/native/*.cpp)
    message(STATUS "Compiling with Bezel Native")
else()
    file(GLOB_RECURSE BEZEL_SOURCES CONFIGURE_DEPENDS bezel/jolt/*.cpp)
    message(STATUS "Compiling with Bezel Jolt (Jolt Physics ${JOLT_VERSION})")
endif()

file(GLOB_RECURSE FINEWAVE_SOURCES    CONFIGURE_DEPENDS finewave/*.cpp)
file(GLOB_RECURSE AURORA_SOURCES      CONFIGURE_DEPENDS aurora/*.cpp)
file(GLOB_RECURSE HYDRA_SOURCES       CONFIGURE_DEPENDS hydra/*.cpp)
file(GLOB_RECURSE ATLAS_SOURCES       CONFIGURE_DEPENDS atlas/*.cpp extern/glad/*.c)
file(GLOB_RECURSE OPAL_SOURCES        CONFIGURE_DEPENDS opal/*.cpp)
file(GLOB_RECURSE PHOTON_SOURCES      CONFIGURE_DEPENDS photon/*.cpp)
file(GLOB_RECURSE GRAPHITE_SOURCES    CONFIGURE_DEPENDS graphite/*.cpp)
file(GLOB_RECURSE RUNTIME_LIB_SOURCES CONFIGURE_DEPENDS runtime/lib/*.cpp)
file(GLOB_RECURSE RUNTIME_SOURCES     CONFIGURE_DEPENDS runtime/executable/*.cpp)
file(GLOB_RECURSE QUICKJS_SOURCES     CONFIGURE_DEPENDS extern/quickjs/*.c)

# ─── quickjs ─────────────────────────────────────────────────────────────────
add_library(quickjs STATIC ${QUICKJS_SOURCES})
target_include_directories(quickjs PUBLIC ${CMAKE_SOURCE_DIR}/extern/quickjs)

if(MSVC)
    target_compile_options(quickjs PRIVATE /W0)
else()
    target_compile_options(quickjs PRIVATE -w -Wno-error)
endif()

# Pick the right OpenAL target name from openal-soft.
set(ATLAS_OPENAL_TARGET "")
if(TARGET OpenAL)
    set(ATLAS_OPENAL_TARGET OpenAL)
elseif(TARGET openal)
    set(ATLAS_OPENAL_TARGET openal)
elseif(TARGET OpenAL::OpenAL)
    set(ATLAS_OPENAL_TARGET OpenAL::OpenAL)
else()
    message(FATAL_ERROR "Could not determine OpenAL target exported by openal-soft")
endif()

# Pick SDL static target.
set(ATLAS_SDL_TARGET "")
if(TARGET SDL3::SDL3-static)
    set(ATLAS_SDL_TARGET SDL3::SDL3-static)
elseif(TARGET SDL3::SDL3)
    set(ATLAS_SDL_TARGET SDL3::SDL3)
elseif(TARGET SDL3-static)
    set(ATLAS_SDL_TARGET SDL3-static)
else()
    message(FATAL_ERROR "Could not determine SDL3 target")
endif()

# Pick Freetype target.
set(ATLAS_FREETYPE_TARGET "")
if(TARGET Freetype::Freetype)
    set(ATLAS_FREETYPE_TARGET Freetype::Freetype)
elseif(TARGET freetype)
    set(ATLAS_FREETYPE_TARGET freetype)
else()
    message(FATAL_ERROR "Could not determine Freetype target")
endif()

# Pick Assimp target.
set(ATLAS_ASSIMP_TARGET "")
if(TARGET assimp::assimp)
    set(ATLAS_ASSIMP_TARGET assimp::assimp)
elseif(TARGET assimp)
    set(ATLAS_ASSIMP_TARGET assimp)
else()
    message(FATAL_ERROR "Could not determine Assimp target")
endif()

# Pick glm target.
set(ATLAS_GLM_TARGET "")
if(TARGET glm::glm-header-only)
    set(ATLAS_GLM_TARGET glm::glm-header-only)
elseif(TARGET glm::glm)
    set(ATLAS_GLM_TARGET glm::glm)
elseif(TARGET glm-header-only)
    set(ATLAS_GLM_TARGET glm-header-only)
else()
    message(FATAL_ERROR "Could not determine GLM target")
endif()

# ─── bezel ───────────────────────────────────────────────────────────────────
add_library(bezel STATIC ${BEZEL_SOURCES})
target_link_libraries(bezel PRIVATE ${ATLAS_SDL_TARGET} ${ATLAS_GLM_TARGET})
target_include_directories(bezel PRIVATE ${ATLAS_DEP_INCLUDE_DIRS})

if(BEZEL_NATIVE)
    target_compile_definitions(bezel PRIVATE NATIVE)
else()
    target_compile_definitions(bezel PRIVATE
        JPH_PROFILE_ENABLED=1
        JPH_DEBUG_RENDERER=1
        JPH_OBJECT_STREAM=1
    )
endif()

# ─── finewave ────────────────────────────────────────────────────────────────
add_library(finewave STATIC ${FINEWAVE_SOURCES})
target_include_directories(finewave PRIVATE ${ATLAS_DEP_INCLUDE_DIRS})
target_link_libraries(finewave PRIVATE
    ${ATLAS_SDL_TARGET}
    ${ATLAS_GLM_TARGET}
    ${ATLAS_OPENAL_TARGET}
    fmt::fmt
)

# ─── opal ────────────────────────────────────────────────────────────────────
add_library(opal STATIC ${OPAL_SOURCES})
target_include_directories(opal PRIVATE ${ATLAS_DEP_INCLUDE_DIRS})
target_link_libraries(opal PRIVATE ${ATLAS_GLM_TARGET} ${ATLAS_SDL_TARGET})
add_dependencies(opal generate_shaders)

if(BACKEND_OPENGL)
    target_link_libraries(opal PRIVATE OpenGL::GL)
elseif(BACKEND_VULKAN)
    target_link_libraries(opal PRIVATE Vulkan::Vulkan spirv-cross-core spirv-cross-glsl spirv-cross-reflect)
    target_compile_definitions(opal PRIVATE VULKAN)
endif()

# ─── photon ──────────────────────────────────────────────────────────────────
add_library(photon STATIC ${PHOTON_SOURCES})
target_link_libraries(photon PRIVATE opal ${ATLAS_GLM_TARGET})
target_include_directories(photon PRIVATE ${ATLAS_DEP_INCLUDE_DIRS})
add_dependencies(photon generate_shaders)

# ─── aurora ──────────────────────────────────────────────────────────────────
add_library(aurora STATIC ${AURORA_SOURCES})
target_link_libraries(aurora PRIVATE ${ATLAS_GLM_TARGET} ${ATLAS_SDL_TARGET} bezel)
target_include_directories(aurora PRIVATE ${ATLAS_DEP_INCLUDE_DIRS})
add_dependencies(aurora generate_shaders)

if(BACKEND_OPENGL)
    target_link_libraries(aurora PRIVATE OpenGL::GL)
elseif(BACKEND_VULKAN)
    target_link_libraries(aurora PRIVATE Vulkan::Vulkan)
endif()

# ─── hydra ───────────────────────────────────────────────────────────────────
add_library(hydra STATIC ${HYDRA_SOURCES})
target_link_libraries(hydra PRIVATE ${ATLAS_GLM_TARGET} ${ATLAS_SDL_TARGET} bezel)
target_include_directories(hydra PRIVATE ${ATLAS_DEP_INCLUDE_DIRS})
add_dependencies(hydra generate_shaders)

if(BACKEND_OPENGL)
    target_link_libraries(hydra PRIVATE OpenGL::GL)
elseif(BACKEND_VULKAN)
    target_link_libraries(hydra PRIVATE Vulkan::Vulkan)
endif()

# ─── graphite ────────────────────────────────────────────────────────────────
add_library(graphite STATIC ${GRAPHITE_SOURCES})
target_link_libraries(graphite PRIVATE
    ${ATLAS_GLM_TARGET}
    ${ATLAS_SDL_TARGET}
    ${ATLAS_FREETYPE_TARGET}
    ${ATLAS_ASSIMP_TARGET}
)
target_include_directories(graphite PRIVATE ${ATLAS_DEP_INCLUDE_DIRS})
add_dependencies(graphite generate_shaders)

# ─── atlas ───────────────────────────────────────────────────────────────────
add_library(atlas STATIC ${ATLAS_SOURCES})
add_dependencies(atlas generate_shaders)

target_link_libraries(atlas
    PUBLIC
        ${ATLAS_SDL_TARGET}
        ${ATLAS_GLM_TARGET}
        bezel
        finewave
        aurora
        hydra
        opal
        ${ATLAS_FREETYPE_TARGET}
        ${ATLAS_ASSIMP_TARGET}
        photon
        graphite
)

if(NOT BEZEL_NATIVE)
    target_link_libraries(atlas PUBLIC Jolt::Jolt)
endif()

target_include_directories(atlas PRIVATE ${ATLAS_DEP_INCLUDE_DIRS})

if(BACKEND_OPENGL)
    target_link_libraries(atlas PRIVATE OpenGL::GL)
elseif(BACKEND_VULKAN)
    target_link_libraries(atlas PRIVATE Vulkan::Vulkan)
    if(APPLE)
        message(STATUS "Configuring MoltenVK for macOS")
        target_compile_definitions(atlas PRIVATE VK_USE_PLATFORM_MACOS_MVK)
    endif()
endif()

target_compile_definitions(atlas PRIVATE ATLAS_LIB GL_SILENT_DEPRECATION)
target_compile_options(atlas PRIVATE -Wno-overlength-strings)

if(WIN32)
    target_compile_definitions(atlas PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    target_link_libraries(atlas PRIVATE opengl32)
elseif(APPLE)
    if(BACKEND_OPENGL)
        target_link_libraries(atlas PRIVATE
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit"
        )
    elseif(BACKEND_VULKAN)
        target_link_libraries(atlas PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework QuartzCore"
        )
    elseif(BACKEND_METAL)
        target_link_libraries(atlas PRIVATE
            "-framework Metal"
            "-framework MetalKit"
            "-framework MetalFX"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework QuartzCore"
        )
    endif()
elseif(UNIX)
    target_link_libraries(atlas PRIVATE dl pthread)
endif()

# ─── runtime_lib ─────────────────────────────────────────────────────────────
add_library(runtime_lib SHARED ${RUNTIME_LIB_SOURCES})
target_link_libraries(runtime_lib PRIVATE atlas quickjs fmt::fmt)
target_compile_definitions(runtime_lib PRIVATE
    RUNTIME_LIB
    JPH_PROFILE_ENABLED=1
    JPH_DEBUG_RENDERER=1
    JPH_OBJECT_STREAM=1
)
add_dependencies(runtime_lib generate_shaders generate_runtime_scripts)
target_include_directories(runtime_lib PUBLIC ${CMAKE_SOURCE_DIR}/extern/quickjs)
target_include_directories(runtime_lib PRIVATE ${ATLAS_DEP_INCLUDE_DIRS})
set_target_properties(runtime_lib PROPERTIES
    OUTPUT_NAME runtime
    PREFIX ""
)

# ─── runtime executable ──────────────────────────────────────────────────────
add_executable(atlasrun ${RUNTIME_SOURCES})
target_link_libraries(atlasrun PRIVATE runtime_lib)
target_compile_definitions(atlasrun PRIVATE
    RUNTIME_BIN
    JPH_PROFILE_ENABLED=1
    JPH_DEBUG_RENDERER=1
    JPH_OBJECT_STREAM=1
)
target_include_directories(atlasrun PRIVATE ${CMAKE_SOURCE_DIR}/runtime/include)
target_include_directories(atlasrun PRIVATE ${ATLAS_DEP_INCLUDE_DIRS})
add_dependencies(atlasrun generate_shaders)
