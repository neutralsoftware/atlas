import argparse
import os
import platform
import shutil
import subprocess
import sys
import tarfile
from pathlib import Path


def run(command, cwd=None, capture=False, env=None):
    print("+", " ".join(str(part) for part in command), flush=True)
    return subprocess.run(
        [str(part) for part in command],
        cwd=cwd,
        check=True,
        text=True,
        capture_output=capture,
        env=env
    )


def require(name, override=None):
    candidate = override or shutil.which(name)
    if not candidate:
        raise RuntimeError(f"Required tool was not found: {name}")
    return Path(candidate)


def copy_required(source, destination):
    if not source.is_file():
        raise RuntimeError(f"Required build output was not produced: {source}")
    shutil.copy2(source, destination)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("--release", action="store_true")
    args = parser.parse_args()
    if args.debug == args.release:
        raise RuntimeError("Choose exactly one of --debug or --release")
    if platform.system() != "Linux":
        raise RuntimeError("Linux packaging must run on Linux")

    machine = platform.machine().lower()
    if machine in {"aarch64", "arm64"}:
        architecture = "aarch64"
    elif machine in {"x86_64", "amd64"}:
        architecture = "x86_64"
    else:
        raise RuntimeError(f"Unsupported Linux architecture: {machine}")
    root = Path(__file__).resolve().parent.parent
    mode = "release" if args.release else "debug"
    configuration = mode.capitalize()
    build_directory = root / "build" / "package" / f"linux-{architecture}-{mode}"
    dist_directory = root / "dist" / "Linux" / mode
    app_directory = build_directory / "AppDir"
    if app_directory.exists():
        shutil.rmtree(app_directory)
    bin_directory = app_directory / "usr" / "bin"
    icon_directory = app_directory / "usr" / "share" / "icons" / "hicolor" / "scalable" / "apps"
    applications_directory = app_directory / "usr" / "share" / "applications"
    bin_directory.mkdir(parents=True)
    icon_directory.mkdir(parents=True)
    applications_directory.mkdir(parents=True)
    dist_directory.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env["CC"] = "clang"
    env["CXX"] = "clang++"

    run([
        require("cmake"), "-S", root, "-B", build_directory, "-G", "Ninja",
        f"-DCMAKE_BUILD_TYPE={configuration}", "-DBACKEND=VULKAN",
        "-DATLAS_PHOTON_BACKEND=VULKAN",
    ], env=env)
    run([
        require("cmake"), "--build", build_directory, "--target", "AtlasEditor", "atlasrun",
        "--parallel", str(os.cpu_count()), "--verbose",
    ])

    built_bin = build_directory / "bin"
    copy_required(built_bin / "AtlasEditor", bin_directory / "atlas-engine")
    copy_required(built_bin / "atlas", bin_directory / "atlas")
    copy_required(built_bin / "atlasrun", bin_directory / "atlasrun")
    copy_required(built_bin / "runtime.so", bin_directory / "runtime.so")
    shutil.copy2(root / "editor" / "assets" / "atlasStarBright.svg", icon_directory / "atlas-engine.svg")
    shutil.copy2(root / "packaging" / "linux" / "atlas-engine.desktop", applications_directory / "atlas-engine.desktop")

    linuxdeploy = require("linuxdeploy", os.environ.get("ATLAS_LINUXDEPLOY"))
    run([
        linuxdeploy, "--appdir", app_directory,
        "--executable", bin_directory / "atlas-engine",
        "--desktop-file", applications_directory / "atlas-engine.desktop",
        "--icon-file", icon_directory / "atlas-engine.svg",
        "--plugin", "qt",
    ])

    dependency_check = run(["ldd", bin_directory / "atlas-engine"], capture=True)
    if "not found" in dependency_check.stdout:
        raise RuntimeError(f"Unresolved dependencies:\n{dependency_check.stdout}")
    for required in ["atlas-engine", "atlas", "atlasrun", "runtime.so"]:
        if not (bin_directory / required).is_file():
            raise RuntimeError(f"Packaged Linux artifact is missing {required}")

    archive = dist_directory / f"Atlas-Engine-Linux-{architecture}-{mode}.tar.gz"
    if archive.exists():
        archive.unlink()
    with tarfile.open(archive, "w:gz") as stream:
        stream.add(app_directory, arcname="Atlas Engine")
    print(f"Packaged Linux artifact: {archive}")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Packaging failed: {error}", file=sys.stderr)
        raise SystemExit(1)
