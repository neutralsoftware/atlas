GENERATOR := "Ninja"

build:
    cd build && ninja -j8

config backend="AUTO" bezel_native="OFF":
    mkdir -p build
    cd build && cmake -G "{{ GENERATOR }}" \
        -DCMAKE_C_COMPILER=/usr/bin/clang \
        -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
        -DCMAKE_C_COMPILER_LAUNCHER= \
        -DCMAKE_CXX_COMPILER_LAUNCHER= \
        -DBACKEND={{ backend }} \
        -DBEZEL_NATIVE={{ bezel_native }} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        ..

target target backend="AUTO" bezel_native="OFF":
    mkdir -p build
    cd build && cmake -G "{{ GENERATOR }}" \
        -DBACKEND={{ backend }} \
        -DBEZEL_NATIVE={{ bezel_native }} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_C_COMPILER_LAUNCHER= \
        -DCMAKE_CXX_COMPILER_LAUNCHER= \
        ..
    cd build && ninja -j8 {{ target }}


run test="" backend="AUTO" bezel_native="OFF":
    just build
    test_dir="{{ test }}"; if [ -z "$test_dir" ]; then test_dir="$(tr -d '\n' < tests/default.txt)"; fi; cd "tests/$test_dir" && atlas script compile && cd "../.."; MTL_HUD_ENABLED=0 ./build/bin/atlasrun "tests/$test_dir/project.atlas"

addTest name:
    test_dir="tests/{{ name }}"; [ ! -e "$test_dir" ] || { echo "Test already exists: $test_dir" >&2; exit 1; }; mkdir -p "$test_dir/assets/scripts" && cp tests/simple/project.atlas "$test_dir/project.atlas" && cp tests/simple/main.ascene "$test_dir/main.ascene" && cp tests/simple/assets/scripts/simpleLog.ts "$test_dir/assets/scripts/simpleLog.ts" && (cd "$test_dir" && cargo run --quiet --manifest-path ../../cli/Cargo.toml -- script init) && rm -f "$test_dir/lib/atlas.d.ts" && ln -s ../../../runtime/atlas.d.ts "$test_dir/lib/atlas.d.ts"

debug backend="AUTO" bezel_native="OFF":
    just build {{ backend }} {{ bezel_native }}
    MTL_HUD_ENABLED=1 ./build/bin/atlas_test

clangd backend="AUTO" bezel_native="OFF":
    mkdir -p build
    cd build && cmake -G "{{ GENERATOR }}" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DBACKEND={{ backend }} \
        -DBEZEL_NATIVE={{ bezel_native }} \
        -DCMAKE_C_COMPILER_LAUNCHER= \
        -DCMAKE_CXX_COMPILER_LAUNCHER= \
        ..
    ln -sf build/compile_commands.json compile_commands.json

lint:
    run-clang-tidy -p build -quiet -warnings-as-errors='*'

clean:
    rm -rf build include/atlas/core/default_shaders.h docs/html

frametest:
    just build
    timeout 2 ./build/bin/atlas_test

cli:
    cargo build

release-metal:
    rm -rf build/release-metal
    mkdir -p build/release-metal dist/release
    cd build/release-metal && cmake -G "{{ GENERATOR }}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBACKEND=METAL \
        -DCMAKE_C_COMPILER_LAUNCHER= \
        -DCMAKE_CXX_COMPILER_LAUNCHER= \
        ../..
    cd build/release-metal && ninja -j8

release-opengl:
    rm -rf build/release-opengl
    mkdir -p build/release-opengl dist/release
    cd build/release-opengl && cmake -G "{{ GENERATOR }}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBACKEND=OPENGL \
        -DCMAKE_C_COMPILER_LAUNCHER= \
        -DCMAKE_CXX_COMPILER_LAUNCHER= \
        ../..
    cd build/release-opengl && ninja -j8 atlas bezel finewave aurora hydra opal photon graphite

release-vulkan:
    rm -rf build/release-vulkan
    mkdir -p build/release-vulkan dist/release
    cd build/release-vulkan && cmake -G "{{ GENERATOR }}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBACKEND=VULKAN \
        -DCMAKE_C_COMPILER_LAUNCHER= \
        -DCMAKE_CXX_COMPILER_LAUNCHER= \
        ../..
    cd build/release-vulkan && ninja -j8 atlas bezel finewave aurora hydra opal photon graphite

docs backend="AUTO":
    mkdir -p build
    doxygen -w html header.html delete.html delete.css
    rm delete.html delete.css
    cd build && cmake -G "{{ GENERATOR }}" \
        -DBACKEND={{ backend }} \
        -DCMAKE_C_COMPILER_LAUNCHER= \
        -DCMAKE_CXX_COMPILER_LAUNCHER= \
        ..
    cp header.html build/_deps/doxygen-awesome-css-src/header.html
    rm header.html
    doxygen Doxyfile

release backend="AUTO":
    if [ "{{ backend }}" = "AUTO" ] || [ "{{ backend }}" = "all" ] || [ "{{ backend }}" = "ALL" ]; then just release-metal && just release-opengl && just release-vulkan; elif [ "{{ backend }}" = "metal" ] || [ "{{ backend }}" = "METAL" ]; then just release-metal; elif [ "{{ backend }}" = "opengl" ] || [ "{{ backend }}" = "OPENGL" ]; then just release-opengl; elif [ "{{ backend }}" = "vulkan" ] || [ "{{ backend }}" = "VULKAN" ]; then just release-vulkan; else echo "Unknown backend '{{ backend }}'. Use AUTO|metal|opengl|vulkan" && exit 1; fi

run-docs:
    just docs
    python3 -m http.server 8000 --directory docs/html
