#!/usr/bin/env python3
"""Cross-platform development commands for Atlas Engine."""

import os
import re
import shutil
import subprocess
import sys
import time
import webbrowser
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
NATIVE_BUILD = BUILD / "native-tests"
WINDOWS = sys.platform == "win32"
MACOS = sys.platform == "darwin"

TARGETS = [
    "atlas",
    "bezel",
    "finewave",
    "aurora",
    "hydra",
    "opal",
    "photon",
    "graphite",
]


def execute(*args, cwd=ROOT, env=None):
    print("+", *map(str, args), flush=True)
    subprocess.run(
        [str(arg) for arg in args],
        cwd=cwd,
        env=env,
        check=True,
    )


def executable(name, directory=BUILD / "bin"):
    suffix = ".exe" if WINDOWS else ""
    path = directory / (name + suffix)
    if not path.is_file():
        raise FileNotFoundError(f"Executable not found: {path}")
    return path


def configure(backend="AUTO", release_dir=None):
    directory = release_dir or BUILD
    execute(
        "cmake",
        "-S",
        ROOT,
        "-B",
        directory,
        "-G",
        "Ninja",
        "-DCMAKE_C_COMPILER_LAUNCHER=",
        "-DCMAKE_CXX_COMPILER_LAUNCHER=",
        f"-DBACKEND={backend}",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        *(["-DCMAKE_BUILD_TYPE=Release"] if release_dir else []),
    )


def configure_target(
    backend="AUTO", bezel_native="OFF", ftet_wild=None, directory=None, options=None
):
    directory = directory or BUILD
    if ftet_wild is None:
        ftet_wild = "OFF" if WINDOWS else "ON"
    requested = {
        "BACKEND": backend.upper(),
        "BEZEL_NATIVE": bezel_native.upper(),
        "ENABLE_FTET_WILD": ftet_wild.upper(),
    }
    requested.update(options or {})
    cache_path = directory / "CMakeCache.txt"
    cached = {}
    if cache_path.is_file():
        for line in cache_path.read_text(encoding="utf-8").splitlines():
            if line.startswith(("#", "//")) or "=" not in line:
                continue
            key, value = line.split("=", 1)
            cached[key.split(":", 1)[0]] = value
    if (directory / "build.ninja").is_file() and all(
        cached.get(key, "").upper() == value for key, value in requested.items()
    ):
        return
    execute(
        "cmake",
        "-S",
        ROOT,
        "-B",
        directory,
        "-G",
        "Ninja",
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_CXX_COMPILER=clang++",
        "-DCMAKE_C_COMPILER_LAUNCHER=",
        "-DCMAKE_CXX_COMPILER_LAUNCHER=",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        *(f"-D{key}={value}" for key, value in requested.items()),
    )


def test_directory(test=""):
    if not test:
        test = (ROOT / "tests/default.txt").read_text(encoding="utf-8").strip()
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", test):
        raise ValueError(f"Invalid test name: {test!r}")
    return ROOT / "tests" / test


def build_native_test(test="", backend="AUTO", bezel_native="OFF", ftet_wild=None):
    test_dir = test_directory(test)
    if not (test_dir / "main.cpp").is_file():
        available = ", ".join(
            sorted(path.parent.name for path in (ROOT / "tests").glob("*/main.cpp"))
        )
        raise ValueError(
            f"No native main.cpp in {test_dir}. Available tests: {available}. "
            "Use 'just run-project <name>' for scene projects."
        )
    configure_target(
        backend,
        bezel_native,
        ftet_wild,
        directory=NATIVE_BUILD,
        options={
            "ATLAS_BUILD_EDITOR": "OFF",
            "ATLAS_BUILD_RUNTIME": "OFF",
            "ATLAS_BUILD_TESTS": "ON",
        },
    )
    execute(
        "cmake",
        "--build",
        NATIVE_BUILD,
        "--target",
        f"atlas_test_{test_dir.name}",
        "--parallel",
    )
    return test_dir, executable(f"atlas_test_{test_dir.name}", NATIVE_BUILD / "bin")


def native_environment(test_dir, debug=False):
    env = os.environ.copy()
    if MACOS:
        env["MTL_HUD_ENABLED"] = "1" if debug else "0"
    if WINDOWS:
        directories = [NATIVE_BUILD / "bin", NATIVE_BUILD / "lib"]
        for manifest in sorted(
            NATIVE_BUILD.glob(f"atlas-test-{test_dir.name}-dlls-*.txt")
        ):
            directories.extend(
                Path(line).parent
                for line in manifest.read_text(encoding="utf-8").splitlines()
                if line.strip()
            )
        env["PATH"] = os.pathsep.join(
            [str(path) for path in dict.fromkeys(directories)] + [env.get("PATH", "")]
        )
    return env


def run_test(test="", backend="AUTO", bezel_native="OFF", ftet_wild=None, debug=False):
    test_dir, binary = build_native_test(test, backend, bezel_native, ftet_wild)
    execute(binary, cwd=test_dir, env=native_environment(test_dir, debug))


def run_project(test=""):
    test_dir = test_directory(test)
    if not (test_dir / "project.atlas").is_file():
        raise FileNotFoundError(test_dir / "project.atlas")

    execute("atlas", "script", "compile", cwd=test_dir)

    env = os.environ.copy()
    if MACOS:
        env["MTL_HUD_ENABLED"] = "0"

    execute(
        executable("atlasrun"),
        test_dir / "project.atlas",
        env=env,
    )


def add_test(name):
    test_dir = test_directory(name)
    if test_dir.exists():
        raise FileExistsError(f"Test already exists: {test_dir}")

    test_dir.mkdir(parents=True)
    shutil.copy2(ROOT / "tests/simple/main.cpp", test_dir / "main.cpp")
    print(f"Created {test_dir / 'main.cpp'}. Run with: just run {name}")


def clangd():
    source = BUILD / "compile_commands.json"
    destination = ROOT / "compile_commands.json"

    if not source.exists():
        raise FileNotFoundError(source)

    destination.unlink(missing_ok=True)
    try:
        destination.symlink_to(source)
    except OSError:
        shutil.copy2(source, destination)


def clean():
    for directory in [BUILD, ROOT / "docs/html"]:
        if directory.exists():
            shutil.rmtree(directory)

    (ROOT / "include/atlas/core/default_shaders.h").unlink(missing_ok=True)


def frametest(test=""):
    test_dir, binary = build_native_test(test)
    process = subprocess.Popen(
        [str(binary)],
        cwd=test_dir,
        env=native_environment(test_dir),
    )
    try:
        return_code = process.wait(timeout=2)
        if return_code:
            raise RuntimeError(f"Frame test exited with code {return_code}")
    except subprocess.TimeoutExpired:
        process.terminate()
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def copy_cli():
    binary = ROOT / "target/release" / ("atlas.exe" if WINDOWS else "atlas")

    if not binary.exists():
        raise FileNotFoundError(binary)

    if WINDOWS:
        destination = Path.home() / ".local/bin"
    else:
        destination = Path.home() / ".local/bin"

    destination.mkdir(parents=True, exist_ok=True)
    shutil.copy2(binary, destination / binary.name)
    print(f"Installed: {destination / binary.name}")
    print("Make sure this directory is on your PATH.")


def package(mode):
    if not MACOS:
        raise RuntimeError("macOS packaging requires macOS.")

    env = os.environ.copy()
    env["ATLAS_ALLOW_UNNOTARIZED_RELEASE"] = "1"

    execute(
        sys.executable,
        ROOT / "scripts/package_app.py",
        f"--{mode}",
        "--macOS",
        env=env,
    )
    execute(sys.executable, ROOT / "scripts/sign_macos_local.py")


def release_build(backend):
    if backend == "METAL" and not MACOS:
        raise RuntimeError("Metal builds require macOS.")

    directory = BUILD / f"release-{backend.lower()}"

    if directory.exists():
        shutil.rmtree(directory)

    directory.mkdir(parents=True)
    (ROOT / "dist/release").mkdir(parents=True, exist_ok=True)

    configure(backend, release_dir=directory)

    command = ["cmake", "--build", str(directory), "--parallel"]
    if backend != "METAL":
        command += ["--target", *TARGETS]

    execute(*command)


def release(backend):
    backend = backend.upper()

    if backend in ("AUTO", "ALL"):
        backends = ["METAL", "OPENGL", "VULKAN"] if MACOS else ["OPENGL", "VULKAN"]
    elif backend in ("METAL", "OPENGL", "VULKAN"):
        backends = [backend]
    else:
        raise ValueError(f"Unknown backend: {backend}")

    for item in backends:
        release_build(item)


def docs(backend):
    execute("doxygen", "-w", "html", "header.html", "delete.html", "delete.css")
    for name in ["delete.html", "delete.css"]:
        (ROOT / name).unlink(missing_ok=True)

    configure(backend)

    source = ROOT / "header.html"
    destination = BUILD / "_deps/doxygen-awesome-css-src/header.html"

    if destination.parent.exists():
        shutil.copy2(source, destination)

    source.unlink(missing_ok=True)
    execute("doxygen", "Doxyfile")


def serve_docs():
    import http.server
    import socketserver
    import functools

    handler = functools.partial(
        http.server.SimpleHTTPRequestHandler,
        directory=str(ROOT / "docs/html"),
    )
    with socketserver.TCPServer(("", 8000), handler) as server:
        print("Documentation: http://localhost:8000")
        server.serve_forever()


def editor():
    directory = BUILD / "bin"
    if MACOS:
        app = directory / "Atlas Engine.app/Contents/MacOS/Atlas Engine"
        execute(app)
    else:
        candidates = [
            directory / ("AtlasEditor.exe" if WINDOWS else "AtlasEditor"),
            directory / ("Atlas Engine.exe" if WINDOWS else "Atlas Engine"),
        ]
        for candidate in candidates:
            if candidate.exists():
                execute(candidate)
                return
        raise FileNotFoundError(f"Could not locate Atlas Editor in {directory}")


def tidy(full=False):
    roots = [
        "include",
        "src",
        "atlas",
        "aurora",
        "bezel",
        "editor",
        "finewave",
        "graphite",
        "hydra",
        "opal",
        "photon",
    ]

    checks = "-*,clang-diagnostic-*,portability-*," "misc-include-cleaner"
    if full:
        checks = (
            "-*,clang-diagnostic-*,bugprone-*,"
            "performance-*,portability-*,misc-include-cleaner"
        )

    files = []
    for root in roots:
        directory = ROOT / root
        if not directory.exists():
            continue

        for path in directory.rglob("*"):
            if path.suffix not in (".cpp", ".cc", ".cxx"):
                continue
            if any(part in ("extern", "third-party", "build") for part in path.parts):
                continue
            files.append(path)

    log = ROOT / "clang-tidy.log"
    failures = 0

    with log.open("w", encoding="utf-8") as output:
        for index, file in enumerate(files, 1):
            print(f"[{index}/{len(files)}] {file.relative_to(ROOT)}")
            result = subprocess.run(
                [
                    "clang-tidy",
                    str(file),
                    "-p",
                    str(BUILD),
                    f"-checks={checks}",
                ],
                cwd=ROOT,
                stdout=output,
                stderr=subprocess.STDOUT,
            )
            if result.returncode:
                failures += 1

    print(f"Finished. {failures} failed commands. Log: {log}")
    if failures:
        raise RuntimeError("clang-tidy reported failures.")


def lint():
    execute(
        "run-clang-tidy",
        "-p",
        "build",
        "-quiet",
        "-warnings-as-errors=*",
    )


def refresh_opal():
    execute("git", "submodule", "sync", "--recursive")
    execute("git", "-C", "opal", "fetch", "origin")
    execute("git", "-C", "opal", "checkout", "main")
    execute("git", "-C", "opal", "reset", "--hard", "origin/main")
    execute("git", "add", "opal")


def main():
    if len(sys.argv) < 2:
        raise SystemExit("Missing Atlas task name")

    task = sys.argv[1]
    args = sys.argv[2:]

    tasks = {
        "configure-target": lambda: configure_target(*args),
        "run": lambda: run_test(*args),
        "run-project": lambda: run_project(*args),
        "add-test": lambda: add_test(args[0]),
        "debug": lambda: run_test(*args, debug=True),
        "clangd": clangd,
        "lint": lint,
        "clean": clean,
        "frametest": lambda: frametest(*args),
        "copy-cli": copy_cli,
        "package": lambda: package(args[0]),
        "release-build": lambda: release_build(args[0].upper()),
        "release": lambda: release(args[0]),
        "docs": lambda: docs(args[0]),
        "serve-docs": serve_docs,
        "editor": editor,
        "tidy": tidy,
        "tidy-full": lambda: tidy(full=True),
        "refresh-opal": refresh_opal,
    }

    if task not in tasks:
        raise SystemExit(f"Unknown task: {task}")

    tasks[task]()


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Atlas task failed: {error}", file=sys.stderr)
        sys.exit(1)
