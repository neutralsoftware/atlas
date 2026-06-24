#!/usr/bin/env python3

from pathlib import Path
import sys
import re


def constant_name(path: Path) -> str:
    name = path.stem.upper()
    name = re.sub(r"[^A-Z0-9]+", "_", name)
    return f"{name}_THEME"


def cpp_string_literal(text: str) -> str:
    text = text.replace("\\", "\\\\")
    text = text.replace("\"", "\\\"")
    text = text.replace("\r\n", "\n")
    text = text.replace("\r", "\n")

    return "\n".join(f'"{line}\\n"' for line in text.splitlines())


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: embed_qss.py <output_header> <qss_files...>")
        return 1

    output = Path(sys.argv[1])
    qss_files = [Path(p) for p in sys.argv[2:]]

    output.parent.mkdir(parents=True, exist_ok=True)

    lines: list[str] = [
        "#pragma once",
        "",
        "// This file is generated. Do not edit manually.",
        "// Generated from .qss theme files.",
        "",
    ]

    for qss_file in qss_files:
        text = qss_file.read_text(encoding="utf-8")
        constant = constant_name(qss_file)

        lines.append(f"// Source: {qss_file.as_posix()}")
        lines.append(f"inline constexpr const char* {constant} =")
        lines.append(cpp_string_literal(text))
        lines.append(";")
        lines.append("")

    output.write_text("\n".join(lines), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
