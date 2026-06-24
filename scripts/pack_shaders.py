import os
import sys
import subprocess
import tempfile

if len(sys.argv) < 3:
    print("Usage: python pack_shaders.py <input_dir> <output_file> [opengl|vulkan|metal]")
    sys.exit(1)

input_dir = sys.argv[1]
output_file = sys.argv[2]
mode_arg = sys.argv[3].lower() if len(sys.argv) >= 4 else None

MAX_CHUNK = 8192
GLSL_EXTENSIONS = {'.vert', '.frag', '.comp', '.geom', '.tesc', '.tese', '.glsl'}
SHADER_EXTENSIONS = GLSL_EXTENSIONS | {'.metal'}


def parse_backend_mode(mode):
    if mode is None:
        return None
    if mode in ('opengl', 'vulkan', 'metal'):
        return mode
    if mode in ('true', '1', 'yes'):
        return 'vulkan'
    if mode in ('false', '0', 'no'):
        return 'opengl'
    raise ValueError(f"Unknown backend mode '{mode}'. Expected opengl, vulkan, or metal.")


def compile_to_spirv(glsl_code, shader_path):
    """Compile GLSL to SPIR-V using glslangValidator or glslc"""
    ext = os.path.splitext(shader_path)[1].lower()
    stage_map = {
        '.vert': 'vert',
        '.frag': 'frag',
        '.comp': 'comp',
        '.geom': 'geom',
        '.tesc': 'tesc',
        '.tese': 'tese'
    }

    stage = stage_map.get(ext, 'vert')

    with tempfile.NamedTemporaryFile(mode='w', suffix=f'.{stage}', delete=False) as tmp_in:
        tmp_in.write(glsl_code)
        tmp_in_path = tmp_in.name

    tmp_out_path = tmp_in_path + '.spv'

    try:
        try:
            subprocess.run(
                ['glslangValidator', '-V', '-o', tmp_out_path, tmp_in_path],
                check=True,
                capture_output=True
            )
        except (subprocess.CalledProcessError, FileNotFoundError):
            subprocess.run(
                ['glslc', f'-fshader-stage={stage}', '-o', tmp_out_path, tmp_in_path],
                check=True,
                capture_output=True
            )

        with open(tmp_out_path, 'rb') as f:
            return f.read()

    except subprocess.CalledProcessError as e:
        print(f"Error compiling {shader_path}: {e}")
        print(f"stderr: {e.stderr.decode() if e.stderr else 'N/A'}")
        return None

    finally:
        if os.path.exists(tmp_in_path):
            os.remove(tmp_in_path)
        if os.path.exists(tmp_out_path):
            os.remove(tmp_out_path)


def detect_backend(path):
    explicit_backend = parse_backend_mode(mode_arg)
    if explicit_backend is not None:
        return explicit_backend

    normalized = os.path.normpath(path).lower()
    base = os.path.basename(normalized)
    if base in ('opengl', 'vulkan', 'metal'):
        return base
    parts = normalized.split(os.sep)
    for backend in ('metal', 'vulkan', 'opengl'):
        if backend in parts:
            return backend
    return 'opengl'


try:
    backend_mode = detect_backend(input_dir)
except ValueError as e:
    print(f"Error: {e}")
    sys.exit(1)


def canonical_shader_name(filename):
    if filename.endswith('.metal'):
        filename = filename[:-6]
    return filename


def variable_name_from_filename(filename):
    canonical = canonical_shader_name(filename)
    return canonical.replace('.', '_').replace('-', '_').upper()


def should_compile_file(path, backend):
    if backend != 'vulkan':
        return False
    ext = os.path.splitext(path)[1].lower()
    if ext == '.metal':
        return False
    return ext in GLSL_EXTENSIONS


def shader_priority(relative_path, backend):
    rel = relative_path.replace('\\', '/')
    if backend == 'metal':
        if rel.startswith('vulkan/'):
            return 2
        if rel.startswith('opengl/'):
            return 1
    return 0


def write_chunks(out, var_name, contents):
    """Write shader source as chunk arrays to avoid oversized string literals"""
    out.write(f'static const char* const {var_name}_PARTS[] = {{\n')

    part_count = 0
    if contents == "":
        out.write('R"()",\n')
        part_count = 1
    else:
        for i in range(0, len(contents), MAX_CHUNK):
            chunk = contents[i:i + MAX_CHUNK]
            out.write(f'R"({chunk})",\n')
            part_count += 1

    out.write('};\n')
    out.write(
        f'static const AtlasPackedShaderSource {var_name} = '
        f'{{{var_name}_PARTS, {part_count}}};\n\n'
    )


with open(output_file, "w") as out:
    backend = backend_mode

    out.write("// This file contains packed shader source code.\n")
    if backend == 'vulkan':
        out.write("// Shaders compiled to SPIR-V for Vulkan\n")
    elif backend == 'metal':
        out.write("// Metal shaders packed as source\n")
    out.write("#ifndef ATLAS_GENERATED_SHADERS_H\n")
    out.write("#define ATLAS_GENERATED_SHADERS_H\n\n")
    out.write("#include <cstddef>\n\n")
    out.write("namespace opal {\n")
    out.write("const char *packedShaderSource(const char *const *parts, std::size_t count);\n")
    out.write("}\n\n")
    out.write("struct AtlasPackedShaderSource {\n")
    out.write("    const char *const *parts;\n")
    out.write("    std::size_t count;\n\n")
    out.write("    operator const char *() const {\n")
    out.write("        return opal::packedShaderSource(parts, count);\n")
    out.write("    }\n")
    out.write("};\n\n")

    shader_files = []
    for root, _, files in os.walk(input_dir):
        for filename in files:
            path = os.path.join(root, filename)
            if not os.path.isfile(path):
                continue
            ext = os.path.splitext(filename)[1].lower()
            if ext not in SHADER_EXTENSIONS:
                continue
            rel = os.path.relpath(path, input_dir)
            shader_files.append((rel, path, filename))

    shader_files.sort(key=lambda x: x[0].replace('\\', '/'))

    chosen_files = {}
    for rel, path, filename in shader_files:
        var_name = variable_name_from_filename(filename)
        priority = shader_priority(rel, backend)
        existing = chosen_files.get(var_name)
        if existing is None:
            chosen_files[var_name] = (priority, rel, path, filename)
        else:
            prev_priority, prev_rel, _, _ = existing
            if priority > prev_priority:
                chosen_files[var_name] = (priority, rel, path, filename)
                print(f"Info: Replaced shader symbol {var_name}: {prev_rel} -> {rel}")
            else:
                print(f"Info: Skipping duplicate shader symbol {var_name}: {rel}")

    ordered_entries = sorted(chosen_files.items(), key=lambda item: item[0])
    for var_name, (_, rel, path, filename) in ordered_entries:
        with open(path, "r") as f:
            contents = f.read()

        if should_compile_file(path, backend):
            if contents.strip() == "":
                out.write(f'// {rel} is empty\n')
                write_chunks(out, var_name, "")
            else:
                spirv_bytes = compile_to_spirv(contents, path)
                if spirv_bytes is not None:
                    hex_string = ''.join(f'{b:02x}' for b in spirv_bytes)
                    out.write(f'// Compiled from {rel} (SPIR-V as hex string)\n')
                    write_chunks(out, var_name, hex_string)
                else:
                    print(f"Warning: Failed to compile {rel}, emitting empty shader")
                    write_chunks(out, var_name, "")
        else:
            write_chunks(out, var_name, contents)

    out.write("#endif // ATLAS_GENERATED_SHADERS_H\n")


print(f"Shaders packed successfully to {output_file}")
if backend_mode == 'vulkan':
    print("SPIR-V compilation enabled")
