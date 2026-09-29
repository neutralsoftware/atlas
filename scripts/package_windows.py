import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path


def run(command, cwd=None):
    print("+", " ".join(str(part) for part in command), flush=True)
    subprocess.run([str(part) for part in command], cwd=cwd, check=True)


def require(name, override=None):
    candidate = override or shutil.which(name)
    if not candidate:
        raise RuntimeError(f"Required tool was not found: {name}")
    return Path(candidate)


def locate_windeployqt():
    configured = os.environ.get("ATLAS_WINDEPLOYQT")
    if configured:
        return require("windeployqt", configured)
    found = shutil.which("windeployqt")
    if found:
        return Path(found)
    qtpaths = shutil.which("qtpaths6") or shutil.which("qtpaths")
    if qtpaths:
        result = subprocess.run(
            [qtpaths, "--query", "QT_INSTALL_BINS"],
            check=True,
            text=True,
            capture_output=True,
        )
        candidate = Path(result.stdout.strip()) / "windeployqt.exe"
        if candidate.is_file():
            return candidate
    raise RuntimeError("windeployqt was not found")


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
    if platform.system() != "Windows":
        raise RuntimeError("Windows packaging must run on Windows")
    if platform.machine().lower() not in {"amd64", "x86_64"}:
        raise RuntimeError("Windows packaging currently targets x86_64")

    root = Path(__file__).resolve().parent.parent
    mode = "release" if args.release else "debug"
    configuration = mode.capitalize()
    build_directory = root / "build" / "package" / f"windows-x86_64-{mode}"
    dist_directory = root / "dist" / "Windows" / mode
    package_directory = dist_directory / "Atlas Engine"
    if package_directory.exists():
        shutil.rmtree(package_directory)
    package_directory.mkdir(parents=True)

    run([
        require("cmake"), "-S", root, "-B", build_directory, "-G", "Ninja",
        f"-DCMAKE_BUILD_TYPE={configuration}", "-DBACKEND=VULKAN",
        "-DATLAS_PHOTON_BACKEND=VULKAN",
    ])
    run([
        require("cmake"), "--build", build_directory, "--target", "AtlasEditor", "atlasrun",
        "--parallel", "--verbose",
    ])

    bin_directory = build_directory / "bin"
    for name in ["AtlasEditor.exe", "atlas.exe", "atlasrun.exe", "runtime.dll",
                 "qtadvanceddocking-qt6.dll"]:
        copy_required(bin_directory / name, package_directory / name)

    deploy = [locate_windeployqt(), package_directory / "AtlasEditor.exe"]
    deploy.append("--release" if args.release else "--debug")
    deploy.extend(["--compiler-runtime", "--no-translations"])
    run(deploy)

    conda_root = os.environ.get("CONDA")

    if conda_root:
        conda_bin = Path(conda_root) / "Library" / "bin"

        for pattern in ("*gmp*.dll", "*mpir*.dll"):
            for dll in conda_bin.glob(pattern):
                shutil.copy2(dll, package_directory / dll.name)

    vcpkg_root = os.environ.get("VCPKG_INSTALLATION_ROOT")
    if vcpkg_root:
        vcpkg_bin = Path(vcpkg_root) / "installed" / "x64-windows" / "bin"
        for dll in vcpkg_bin.glob("*.dll"):
            shutil.copy2(dll, package_directory / dll.name)

    icon = package_directory / "atlas-star.ico"
    run([
        require("magick"), root / "editor" / "assets" / "atlasStarBright.svg",
        "-background", "none", "-define", "icon:auto-resize=256,128,64,48,32,16",
        icon,
    ])
    run([require("rcedit"), package_directory / "AtlasEditor.exe", "--set-icon", icon])

    if not (package_directory / "platforms" / "qwindows.dll").is_file():
        raise RuntimeError("Qt Windows platform plugin is missing")
    if not (package_directory / "atlas.exe").is_file():
        raise RuntimeError("Atlas CLI is missing")
    if not (package_directory / "runtime.dll").is_file():
        raise RuntimeError("Atlas runtime is missing")

    archive_base = dist_directory / f"Atlas-Engine-Windows-x86_64-{mode}"
    archive = shutil.make_archive(str(archive_base), "zip", dist_directory, package_directory.name)
    print(f"Packaged Windows artifact: {archive}")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Packaging failed: {error}", file=sys.stderr)
        raise SystemExit(1)
