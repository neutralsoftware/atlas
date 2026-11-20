import argparse
import json
from pathlib import Path
import subprocess
import tempfile


def check_layout(slangc, spirv_cross):
    root = Path(__file__).resolve().parent.parent
    expected = {
        1: (256, {"v0": 0, "n0": 48, "uv0": 96, "t0": 144,
                  "b0": 192, "materialID": 240, "padding": 244}),
        2: (176, {"materialID": 0, "ao": 32, "emissiveIntensity": 36,
                  "albedo": 48, "emissiveColor": 64, "optical": 80,
                  "attenuation": 96, "volume": 112, "volumeAbsorption": 128,
                  "volumeScattering": 144, "volumeEmission": 160}),
        6: (48, {"direction": 0, "diffuse": 16, "specular": 32, "intensity": 44}),
        7: (80, {"position": 0, "diffuse": 16, "intensity": 44, "radius": 60}),
        8: (80, {"position": 0, "direction": 16, "cutOff": 28, "diffuse": 48}),
        9: (112, {"position": 0, "right": 16, "up": 32, "size": 48,
                  "diffuse": 64, "specular": 80, "intensity": 100}),
    }
    with tempfile.TemporaryDirectory(prefix="atlas-ddgi-layout-") as directory:
        binary = Path(directory) / "ddgi.spv"
        subprocess.run([slangc, str(root / "shaders/photon/vulkan/ddgi.slang"),
                        "-I", str(root / "shaders"), "-entry", "computeMain",
                        "-target", "spirv", "-profile", "spirv_1_5",
                        "-preserve-params", "-o", str(binary)], check=True,
                       capture_output=True)
        reflection = json.loads(subprocess.check_output(
            [spirv_cross, str(binary), "--reflect"]))
    checked = set()
    for buffer in reflection["ssbos"]:
        binding = buffer["binding"]
        if buffer["set"] != 1 or binding not in expected:
            continue
        array = reflection["types"][buffer["type"]]["members"][0]
        stride, offsets = expected[binding]
        if array["array_stride"] != stride:
            raise AssertionError(f"binding {binding}: GPU stride {array['array_stride']} != CPU stride {stride}")
        members = {m["name"]: m["offset"] for m in
                   reflection["types"][array["type"]]["members"]}
        for name, offset in offsets.items():
            if members.get(name) != offset:
                raise AssertionError(f"binding {binding}.{name}: GPU offset {members.get(name)} != CPU offset {offset}")
        checked.add(binding)
    if checked != set(expected):
        raise AssertionError(f"Missing GPU buffers: {set(expected) - checked}")
    print("PASS: six DDGI buffer layouts match CPU strides and member offsets")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--slangc", default="slangc")
    parser.add_argument("--spirv-cross", default="spirv-cross")
    arguments = parser.parse_args()
    check_layout(arguments.slangc, arguments.spirv_cross)
