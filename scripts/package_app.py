#!/usr/bin/env python3

import argparse
import os
import platform
import plistlib
import shutil
import subprocess
import sys
from pathlib import Path


def run(command, cwd=None, capture=False):
    print("+", " ".join(str(part) for part in command), flush=True)
    return subprocess.run(
        [str(part) for part in command],
        cwd=cwd,
        check=True,
        text=True,
        capture_output=capture,
    )


def require(name, override=None):
    candidate = override or shutil.which(name)
    if not candidate:
        raise RuntimeError(f"Required tool was not found: {name}")
    return Path(candidate)


def create_icon(source, destination, work_directory):
    sips = require("sips", "/usr/bin/sips")
    iconutil = require("iconutil", "/usr/bin/iconutil")
    iconset = work_directory / "AtlasEngine.iconset"
    if iconset.exists():
        shutil.rmtree(iconset)
    iconset.mkdir(parents=True)
    sizes = {
        "icon_16x16.png": 16,
        "icon_16x16@2x.png": 32,
        "icon_32x32.png": 32,
        "icon_32x32@2x.png": 64,
        "icon_128x128.png": 128,
        "icon_128x128@2x.png": 256,
        "icon_256x256.png": 256,
        "icon_256x256@2x.png": 512,
        "icon_512x512.png": 512,
        "icon_512x512@2x.png": 1024,
    }
    for filename, size in sizes.items():
        run([sips, "-z", size, size, source, "--out", iconset / filename])
    run([iconutil, "-c", "icns", iconset, "-o", destination])


def locate_macdeployqt():
    configured = os.environ.get("ATLAS_MACDEPLOYQT")
    if configured:
        return require("macdeployqt", configured)
    found = shutil.which("macdeployqt")
    if found:
        return Path(found)
    qtpaths = shutil.which("qtpaths6") or shutil.which("qtpaths")
    if qtpaths:
        result = run([qtpaths, "--query", "QT_INSTALL_BINS"], capture=True)
        candidate = Path(result.stdout.strip()) / "macdeployqt"
        if candidate.is_file():
            return candidate
    raise RuntimeError("macdeployqt was not found. Install Qt 6 or set ATLAS_MACDEPLOYQT.")


def macho_dependencies(bundle):
    file_tool = require("file", "/usr/bin/file")
    otool = require("otool", "/usr/bin/otool")
    invalid = []
    for path in bundle.rglob("*"):
        if not path.is_file() or path.is_symlink():
            continue
        kind = subprocess.run(
            [file_tool, "-b", path],
            check=True,
            text=True,
            capture_output=True,
        ).stdout
        if "Mach-O" not in kind:
            continue
        install_ids = set(
            subprocess.run(
                [otool, "-D", path],
                check=False,
                text=True,
                capture_output=True,
            ).stdout.splitlines()[1:]
        )
        output = subprocess.run(
            [otool, "-L", path],
            check=True,
            text=True,
            capture_output=True,
        ).stdout.splitlines()[1:]
        for line in output:
            dependency = line.strip().split(" (", 1)[0]
            if dependency in install_ids:
                continue
            if dependency.startswith(("@", "/System/Library/", "/usr/lib/")):
                continue
            invalid.append((path, dependency))
    return invalid


def sign_bundle(bundle, identity):
    codesign = require("codesign", "/usr/bin/codesign")
    command = [codesign, "--force", "--deep"]
    if identity != "-":
        command.extend(["--options", "runtime", "--timestamp"])
    command.extend(["--sign", identity, bundle])
    run(command)
    run([codesign, "--verify", "--deep", "--strict", "--verbose=2", bundle])


def archive_bundle(bundle, archive):
    if archive.exists():
        archive.unlink()
    run([
        "/usr/bin/ditto",
        "-c",
        "-k",
        "--sequesterRsrc",
        "--keepParent",
        bundle,
        archive,
    ])


def create_dmg(bundle, dmg, staging_directory):
    if staging_directory.exists():
        shutil.rmtree(staging_directory)
    staging_directory.mkdir(parents=True)
    run(["/usr/bin/ditto", bundle, staging_directory / bundle.name])
    os.symlink("/Applications", staging_directory / "Applications")
    if dmg.exists():
        dmg.unlink()
    run([
        "/usr/bin/hdiutil",
        "create",
        "-volname",
        "Atlas Engine",
        "-srcfolder",
        staging_directory,
        "-format",
        "UDZO",
        "-ov",
        dmg,
    ])


def sign_dmg(dmg, identity):
    if identity == "-":
        return
    run([
        "/usr/bin/codesign",
        "--force",
        "--timestamp",
        "--sign",
        identity,
        dmg,
    ])
    run(["/usr/bin/codesign", "--verify", "--verbose=2", dmg])


def validate_release_identity(identity):
    result = subprocess.run(
        ["/usr/bin/security", "find-identity", "-p", "codesigning", "-v"],
        check=True,
        text=True,
        capture_output=True,
    )
    matches = [line for line in result.stdout.splitlines() if identity in line]
    if not any('"Developer ID Application:' in line for line in matches):
        raise RuntimeError(
            "ATLAS_SIGNING_IDENTITY must select an installed Developer ID "
            "Application certificate for a publishable release"
        )


def notarize(artifact, profile):
    run([
        "/usr/bin/xcrun",
        "notarytool",
        "submit",
        artifact,
        "--keychain-profile",
        profile,
        "--wait",
    ])
    run(["/usr/bin/xcrun", "stapler", "staple", artifact])
    run(["/usr/bin/xcrun", "stapler", "validate", artifact])


def validate_dmg(dmg, mountpoint):
    if mountpoint.exists():
        shutil.rmtree(mountpoint)
    mountpoint.mkdir(parents=True)
    run([
        "/usr/bin/hdiutil",
        "attach",
        "-readonly",
        "-nobrowse",
        "-mountpoint",
        mountpoint,
        dmg,
    ])
    try:
        mounted_app = mountpoint / "Atlas Engine.app"
        if not mounted_app.is_dir():
            raise RuntimeError("DMG does not contain Atlas Engine.app")
        if not (mountpoint / "Applications").is_symlink():
            raise RuntimeError("DMG does not contain the Applications link")
        run([
            "/usr/bin/codesign",
            "--verify",
            "--deep",
            "--strict",
            "--verbose=2",
            mounted_app,
        ])
    finally:
        run(["/usr/bin/hdiutil", "detach", mountpoint])


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Build and package Atlas Engine as a self-contained macOS app."
    )
    configuration = parser.add_mutually_exclusive_group(required=True)
    configuration.add_argument("--debug", action="store_true")
    configuration.add_argument("--release", action="store_true")
    parser.add_argument("--macOS", dest="macos", action="store_true", required=True)
    return parser.parse_args()


def main():
    args = parse_arguments()
    if platform.system() != "Darwin":
        raise RuntimeError("--macOS packaging must run on macOS")

    root = Path(__file__).resolve().parent.parent
    mode = "release" if args.release else "debug"
    configuration = mode.capitalize()
    architectures = os.environ.get("ATLAS_MACOS_ARCHITECTURES", platform.machine())
    architecture_tag = "universal" if ";" in architectures else architectures
    deployment_target = os.environ.get("ATLAS_MACOS_DEPLOYMENT_TARGET", "14.0")
    signing_identity = os.environ.get("ATLAS_SIGNING_IDENTITY", "-")
    notary_profile = os.environ.get("ATLAS_NOTARY_PROFILE")
    allow_unnotarized = os.environ.get("ATLAS_ALLOW_UNNOTARIZED_RELEASE") == "1"
    if args.release and (signing_identity == "-" or not notary_profile):
        if not allow_unnotarized:
            raise RuntimeError(
                "A publishable release requires ATLAS_SIGNING_IDENTITY and "
                "ATLAS_NOTARY_PROFILE. Set ATLAS_ALLOW_UNNOTARIZED_RELEASE=1 "
                "only to create a local test DMG."
            )
    if args.release and not allow_unnotarized:
        validate_release_identity(signing_identity)
    build_directory = root / "build" / "package" / f"macos-{mode}-{architecture_tag}"
    assets_directory = build_directory / "package-assets"
    dist_directory = root / "dist" / "macOS" / mode
    app_name = "Atlas Engine.app"
    built_app = build_directory / "bin" / app_name
    packaged_app = dist_directory / app_name
    icon_source = root / "editor" / "assets" / (
        "iconFile-iOS-Dark-1024x1024@1x.png"
        if args.release
        else "Icon-iOS-Default-1024x1024@1x.png"
    )
    icon = assets_directory / "AtlasEngine.icns"

    assets_directory.mkdir(parents=True, exist_ok=True)
    dist_directory.mkdir(parents=True, exist_ok=True)
    create_icon(icon_source, icon, assets_directory)

    run([
        require("cmake"),
        "-S",
        root,
        "-B",
        build_directory,
        "-G",
        "Ninja",
        f"-DCMAKE_BUILD_TYPE={configuration}",
        "-DBACKEND=METAL",
        f"-DCMAKE_OSX_ARCHITECTURES={architectures}",
        f"-DCMAKE_OSX_DEPLOYMENT_TARGET={deployment_target}",
        f"-DATLAS_APP_ICON={icon}",
    ])
    run([
        require("cmake"),
        "--build",
        build_directory,
        "--target",
        "AtlasEditor",
        "--parallel",
        str(os.cpu_count() or 4),
    ])
    if not built_app.is_dir():
        raise RuntimeError(f"Atlas Engine app bundle was not produced at {built_app}")

    if packaged_app.exists():
        shutil.rmtree(packaged_app)
    run(["/usr/bin/ditto", built_app, packaged_app])

    deploy = [
        locate_macdeployqt(),
        packaged_app,
        "-always-overwrite",
        f"-libpath={build_directory / 'lib'}",
    ]
    if signing_identity == "-":
        deploy.append("-codesign=-")
    elif notary_profile:
        deploy.append(f"-sign-for-notarization={signing_identity}")
    else:
        deploy.extend([
            f"-codesign={signing_identity}",
            "-hardened-runtime",
            "-timestamp",
        ])
    run(deploy)
    sign_bundle(packaged_app, signing_identity)

    plist_path = packaged_app / "Contents" / "Info.plist"
    with plist_path.open("rb") as stream:
        plist = plistlib.load(stream)
    if plist.get("CFBundleIdentifier") != "neutralsoftware.atlas":
        raise RuntimeError("Packaged app has the wrong bundle identifier")
    if plist.get("LSMinimumSystemVersion") != deployment_target:
        raise RuntimeError("Packaged app has the wrong minimum macOS version")
    if not (packaged_app / "Contents" / "Helpers" / "atlas").is_file():
        raise RuntimeError("Packaged app is missing the Atlas CLI")
    if not (packaged_app / "Contents" / "Frameworks" / "runtime.dylib").is_file():
        raise RuntimeError("Packaged app is missing the Atlas runtime")

    invalid_dependencies = macho_dependencies(packaged_app)
    if invalid_dependencies:
        details = "\n".join(
            f"{path.relative_to(packaged_app)}: {dependency}"
            for path, dependency in invalid_dependencies
        )
        raise RuntimeError(f"The app contains non-portable library paths:\n{details}")

    archive = dist_directory / (
        f"Atlas-Engine-alpha9-macOS-{architecture_tag}-{mode}.zip"
    )
    archive_bundle(packaged_app, archive)
    dmg_suffix = ""
    if args.release and allow_unnotarized and not notary_profile:
        dmg_suffix = "-UNNOTARIZED"
    dmg = dist_directory / (
        f"Atlas-Engine-alpha9-macOS-{architecture_tag}-{mode}{dmg_suffix}.dmg"
    )
    create_dmg(packaged_app, dmg, build_directory / "dmg-root")
    sign_dmg(dmg, signing_identity)
    if notary_profile:
        if signing_identity == "-":
            raise RuntimeError("ATLAS_NOTARY_PROFILE requires ATLAS_SIGNING_IDENTITY")
        notarize(dmg, notary_profile)
        run([
            "/usr/sbin/spctl",
            "--assess",
            "--type",
            "open",
            "--context",
            "context:primary-signature",
            "--verbose=2",
            dmg,
        ])
    validate_dmg(dmg, build_directory / "dmg-mount")
    signature = "ad-hoc development signature"
    if notary_profile:
        signature = "Developer ID signature and notarization"
    elif signing_identity != "-":
        signature = "Developer ID signature"
    print(f"Packaged app: {packaged_app}")
    print(f"Archive: {archive}")
    print(f"DMG: {dmg}")
    print(f"Architecture: {architectures}")
    print(f"Minimum macOS: {deployment_target}")
    print(f"Trust: {signature}")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Packaging failed: {error}", file=sys.stderr)
        raise SystemExit(1)
