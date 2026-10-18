
set windows-shell := ["pwsh", "-NoLogo", "-NoProfile", "-Command"]

GENERATOR := "Ninja"
PYTHON := if os() == "windows" { "python" } else { "python3" }
FTET_WILD := if os() == "windows" { "OFF" } else { "ON" }

build:
    cmake --build build --parallel

single:
    cmake --build build --parallel 1

config backend="AUTO" bezel_native="OFF" ftet_wild=FTET_WILD:
    cmake -S . -B build -G {{GENERATOR}} -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER= -DBACKEND={{backend}} -DBEZEL_NATIVE={{bezel_native}} -DENABLE_FTET_WILD={{ftet_wild}} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

target target_name backend="AUTO" bezel_native="OFF" ftet_wild=FTET_WILD:
    just config {{backend}} {{bezel_native}} {{ftet_wild}}
    cmake --build build --target {{target_name}} --parallel

run test="" backend="AUTO" bezel_native="OFF":
    {{PYTHON}} scripts/atlas_tasks.py run "{{test}}"

addTest name:
    {{PYTHON}} scripts/atlas_tasks.py add-test "{{name}}"

debug backend="AUTO" bezel_native="OFF" ftet_wild=FTET_WILD:
    just target atlas_test {{backend}} {{bezel_native}} {{ftet_wild}}
    {{PYTHON}} scripts/atlas_tasks.py debug

clangd backend="AUTO" bezel_native="OFF" ftet_wild=FTET_WILD:
    just config {{backend}} {{bezel_native}} {{ftet_wild}}
    {{PYTHON}} scripts/atlas_tasks.py clangd

lint:
    {{PYTHON}} scripts/atlas_tasks.py lint

clean:
    {{PYTHON}} scripts/atlas_tasks.py clean

frametest:
    just build
    {{PYTHON}} scripts/atlas_tasks.py frametest

cli:
    cargo build

copy-cli:
    cargo build --release
    {{PYTHON}} scripts/atlas_tasks.py copy-cli

package-debug-macos args="":
    {{PYTHON}} scripts/atlas_tasks.py package debug

package-release-macos:
    {{PYTHON}} scripts/atlas_tasks.py package release

release-macos:
    just package-release-macos

release-metal:
    {{PYTHON}} scripts/atlas_tasks.py release-build METAL

release-opengl:
    {{PYTHON}} scripts/atlas_tasks.py release-build OPENGL

release-vulkan:
    {{PYTHON}} scripts/atlas_tasks.py release-build VULKAN

release backend="AUTO":
    {{PYTHON}} scripts/atlas_tasks.py release "{{backend}}"

docs backend="AUTO":
    {{PYTHON}} scripts/atlas_tasks.py docs "{{backend}}"

run-docs:
    just docs
    {{PYTHON}} scripts/atlas_tasks.py serve-docs

editor backend="AUTO" bezel_native="OFF" ftet_wild=FTET_WILD:
    just target AtlasEditor {{backend}} {{bezel_native}} {{ftet_wild}}
    {{PYTHON}} scripts/atlas_tasks.py editor

clang-tidy:
    {{PYTHON}} scripts/atlas_tasks.py tidy

clang-tidy-full:
    {{PYTHON}} scripts/atlas_tasks.py tidy-full

refresh-opal:
    {{PYTHON}} scripts/atlas_tasks.py refresh-opal
