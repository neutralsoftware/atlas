import os
import sys

if len(sys.argv) < 3:
    print("Usage: python pack_runtime_scripts.py <input_dir> <output_file>")
    sys.exit(1)

input_dir = sys.argv[1]
output_file = sys.argv[2]

MAX_CHUNK = 8192
SCRIPT_EXTENSIONS = {".js", ".mjs"}


def normalize_relative_path(path):
    return path.replace("\\", "/")


def script_stem(relative_path):
    normalized = normalize_relative_path(relative_path)
    return os.path.splitext(normalized)[0]


def variable_name_from_path(relative_path):
    stem = script_stem(relative_path)
    name = stem.replace("/", "_").replace("-", "_").replace(".", "_")
    return name.upper()


def module_names_from_path(relative_path):
    stem = script_stem(relative_path)
    names = []

    if stem.endswith("/index"):
        module_name = stem[:-len("/index")]
        if module_name:
            names.append(module_name)

    names.append(stem)

    if "/" not in stem and "_" in stem:
        alias = stem.replace("_", "/")
        if alias not in names:
            names.insert(0, alias)

    unique_names = []
    for name in names:
        if name and name not in unique_names:
            unique_names.append(name)
    return unique_names


def c_string_literal(text):
    parts = ['"']
    for byte in text.encode("utf-8"):
        if 32 <= byte <= 126 and byte not in (34, 92):
            parts.append(chr(byte))
        elif byte == 34:
            parts.append('\\"')
        elif byte == 92:
            parts.append("\\\\")
        elif byte == 9:
            parts.append("\\t")
        elif byte == 10:
            parts.append("\\n")
        elif byte == 13:
            parts.append("\\r")
        else:
            parts.append(f"\\{byte:03o}")
    parts.append('"')
    return "".join(parts)


def write_chunks(out, var_name, contents):
    out.write(f"static const char* const {var_name}_PARTS[] = {{\n")
    part_count = 0

    if contents == "":
        out.write('    "",\n')
        part_count = 1
    else:
        for index in range(0, len(contents), MAX_CHUNK):
            chunk = contents[index:index + MAX_CHUNK]
            out.write(f"    {c_string_literal(chunk)},\n")
            part_count += 1

    out.write("};\n")
    out.write(
        f"static const AtlasPackedScriptSource {var_name} = "
        f"{{{var_name}_PARTS, {part_count}}};\n\n"
    )


script_files = []
for root, _, files in os.walk(input_dir):
    for filename in files:
        path = os.path.join(root, filename)
        if not os.path.isfile(path):
            continue
        if os.path.splitext(filename)[1].lower() not in SCRIPT_EXTENSIONS:
            continue
        relative = os.path.relpath(path, input_dir)
        script_files.append((normalize_relative_path(relative), path))

script_files.sort(key=lambda entry: entry[0])

with open(output_file, "w") as out:
    out.write("#ifndef ATLAS_RUNTIME_SCRIPTS_H\n")
    out.write("#define ATLAS_RUNTIME_SCRIPTS_H\n\n")
    out.write("#include <cstddef>\n\n")
    out.write("struct AtlasPackedScriptSource {\n")
    out.write("    const char *const *parts;\n")
    out.write("    std::size_t count;\n")
    out.write("};\n\n")
    out.write("struct AtlasRuntimeScriptModule {\n")
    out.write("    const char *name;\n")
    out.write("    AtlasPackedScriptSource source;\n")
    out.write("};\n\n")

    module_entries = []
    for relative, path in script_files:
        var_name = variable_name_from_path(relative)
        with open(path, "r", encoding="utf-8") as script_file:
            contents = script_file.read()
        write_chunks(out, var_name, contents)
        for module_name in module_names_from_path(relative):
            module_entries.append((module_name, var_name))

    out.write("static const AtlasRuntimeScriptModule ATLAS_RUNTIME_SCRIPT_MODULES[] = {\n")
    for module_name, var_name in module_entries:
        out.write(
            f"    {{{c_string_literal(module_name)}, {var_name}}},\n"
        )
    out.write("};\n\n")
    out.write(
        "static constexpr std::size_t ATLAS_RUNTIME_SCRIPT_MODULE_COUNT = "
        "sizeof(ATLAS_RUNTIME_SCRIPT_MODULES) / sizeof(ATLAS_RUNTIME_SCRIPT_MODULES[0]);\n\n"
    )
    out.write("#endif\n")

print(f"Runtime scripts packed successfully to {output_file}")
