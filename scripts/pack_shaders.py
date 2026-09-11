import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile


def run(command):
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(f"Shader command failed: {' '.join(map(str, command))}\n"
                           f"{result.stdout}{result.stderr}")
    return result.stdout


def read_source(path, stack=()):
    path = Path(path).resolve()
    if path in stack:
        raise ValueError(f"Cyclic shader include: {path}")
    return re.sub(
        r'^[ \t]*#include "([^"\n]+)"\n',
        lambda match: read_source(path.parent / match[1], (*stack, path)),
        path.read_text(), flags=re.MULTILINE)


def write_atomic(path, contents):
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(mode="w", dir=path.parent,
                                     delete=False) as temporary:
        temporary.write(contents)
        temporary_path = Path(temporary.name)
    temporary_path.replace(path)


def pack_source(symbol, source):
    if ')ATLAS_SHADER"' in source:
        raise ValueError(f"Raw-string delimiter collision in {symbol}")
    chunks = [source[i:i + 8192] for i in range(0, len(source), 8192)] or [""]
    parts = "\n".join(f'R"ATLAS_SHADER({chunk})ATLAS_SHADER",' for chunk in chunks)
    return (f"inline const char *const {symbol}_PARTS[] = {{\n{parts}\n}};\n"
            f"inline const AtlasPackedShaderSource {symbol} = "
            f"{{{symbol}_PARTS, {len(chunks)}}};\n")


def generate(input_dir, output_file, backend, slangc, spirv_cross,
             photon_backend, photon_manifest=None):
    input_dir = Path(input_dir).resolve()
    output_file = Path(output_file).resolve()
    artifact_dir = output_file.parent / "shader_artifacts"
    artifact_dir.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((input_dir / "manifest.json").read_text())
    entries = list(manifest["shaders"])
    native = []
    if photon_backend != "off":
        if photon_backend != backend:
            raise ValueError("Photon's native backend must match Atlas's backend")
        native_manifest = (Path(photon_manifest) if photon_manifest else
                           input_dir / "photon" / photon_backend / "manifest.json")
        if not native_manifest.is_file():
            raise ValueError(f"Photon {photon_backend} shaders are not implemented: "
                             f"missing {native_manifest}")
        extension = json.loads(native_manifest.read_text())
        entries.extend(extension.get("shaders", []))
        native = extension.get("native", [])
        symbols = {entry["symbol"] for entry in entries + native}
        if not {"PATH", "DDGI", "DDGI_WRITE"}.issubset(symbols):
            raise ValueError("Photon requires PATH, DDGI, and DDGI_WRITE shaders")

    packed = {}
    artifacts = []
    for entry in sorted(entries, key=lambda item: item["symbol"]):
        symbol = entry["symbol"]
        if not re.fullmatch(r"[A-Z][A-Z0-9_]*", symbol) or symbol in packed:
            raise ValueError(f"Invalid or duplicate shader symbol: {symbol}")
        source = input_dir / entry["source"]
        stage = entry["stage"]
        if stage not in ("vertex", "fragment", "compute"):
            raise ValueError(f"Unsupported shader stage: {stage}")
        spirv = artifact_dir / f"{symbol}.spv"
        run([slangc, str(source), "-I", str(input_dir), "-D",
             f"ATLAS_{stage.upper()}=1", "-entry", entry["entry"],
             "-target", "spirv", "-profile", "spirv_1_3", "-preserve-params",
             "-o", str(spirv)])
        reflection = run([spirv_cross, str(spirv), "--reflect"])
        write_atomic(artifact_dir / f"{symbol}.json", reflection)
        if backend == "metal":
            metal = artifact_dir / f"{symbol}.metal"
            run([spirv_cross, str(spirv), "--msl", "--msl-version", "230000",
                 "--output", str(metal)])
            packed[symbol] = metal.read_text()
        else:
            packed[symbol] = spirv.read_bytes().hex()
        artifacts.append({**entry, "spirv": str(spirv)})

    for entry in native:
        symbol = entry["symbol"]
        if not re.fullmatch(r"[A-Z][A-Z0-9_]*", symbol) or symbol in packed:
            raise ValueError(f"Invalid or duplicate native shader symbol: {symbol}")
        source = input_dir / entry["source"]
        if backend == "metal" and source.suffix == ".metal":
            packed[symbol] = read_source(source)
            write_atomic(artifact_dir / f"{symbol}.metal", packed[symbol])
        elif backend == "vulkan" and source.suffix == ".spv":
            data = source.read_bytes()
            if len(data) % 4 or data[:4] != b"\x03\x02\x23\x07":
                raise ValueError(f"Invalid native SPIR-V: {source}")
            packed[symbol] = data.hex()
        else:
            raise ValueError(f"Invalid native {backend} shader: {source}")

    header = """#pragma once
#include <cstddef>

namespace opal {
const char *packedShaderSource(const char *const *parts, std::size_t count);
}

struct AtlasPackedShaderSource {
    const char *const *parts;
    std::size_t count;
    operator const char *() const {
        return opal::packedShaderSource(parts, count);
    }
};

"""
    header += f"#define ATLAS_HAS_PHOTON {int(photon_backend != 'off')}\n"
    header += f"#define ATLAS_PHOTON_NATIVE_METAL {int(photon_backend == 'metal')}\n\n"
    for symbol, source in sorted(packed.items()):
        header += pack_source(symbol, source) + "\n"
    write_atomic(output_file, header)
    write_atomic(artifact_dir / "manifest.json", json.dumps(artifacts, indent=2) + "\n")
    print(f"Generated {len(entries)} Slang stages and {len(native)} native "
          f"Photon shaders for {backend}: {output_file}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_dir")
    parser.add_argument("output_file")
    parser.add_argument("backend", choices=("vulkan", "metal"))
    parser.add_argument("--slangc", default="slangc")
    parser.add_argument("--spirv-cross", default="spirv-cross")
    parser.add_argument("--photon-backend", choices=("off", "metal", "vulkan"),
                        default="off")
    parser.add_argument("--photon-manifest")
    arguments = parser.parse_args()
    generate(**vars(arguments))


if __name__ == "__main__":
    main()
