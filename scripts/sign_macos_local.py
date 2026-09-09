#!/usr/bin/env python3

import os
import re
import subprocess
import sys
from pathlib import Path


def run(command, capture=False):
    return subprocess.run(
        [str(part) for part in command],
        check=True,
        text=True,
        capture_output=capture,
    )


def find_identity():
    requested = os.environ.get("ATLAS_LOCAL_SIGNING_IDENTITY")
    result = run(
        ["/usr/bin/security", "find-identity", "-p", "codesigning", "-v"],
        capture=True,
    )
    identities = re.findall(
        r'^\s*\d+\)\s+([0-9A-F]{40})\s+"(Apple Development:[^"]+)"',
        result.stdout,
        re.MULTILINE,
    )
    if requested:
        for fingerprint, name in identities:
            if requested in (fingerprint, name) or requested in name:
                return fingerprint, name
        raise RuntimeError(
            f"ATLAS_LOCAL_SIGNING_IDENTITY did not match an Apple Development identity: {requested}"
        )
    if not identities:
        raise RuntimeError("No Apple Development signing identity is installed")
    return identities[0]


def main():
    root = Path(__file__).resolve().parent.parent
    bundle = (
        Path(sys.argv[1]).resolve()
        if len(sys.argv) > 1
        else root / "dist" / "macOS" / "release" / "Atlas Engine.app"
    )
    if not bundle.is_dir():
        raise RuntimeError(f"App bundle does not exist: {bundle}")
    fingerprint, name = find_identity()
    run(
        [
            "/usr/bin/codesign",
            "--force",
            "--deep",
            "--options",
            "runtime",
            "--sign",
            fingerprint,
            bundle,
        ]
    )
    run(
        [
            "/usr/bin/codesign",
            "--verify",
            "--deep",
            "--strict",
            "--verbose=2",
            bundle,
        ]
    )
    print(f"Locally signed {bundle} with {name}")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Local signing failed: {error}", file=sys.stderr)
        raise SystemExit(1)
