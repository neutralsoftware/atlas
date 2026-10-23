import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import pack_qt_themes


ROOT = Path(__file__).resolve().parent.parent


class IncrementalBuildTests(unittest.TestCase):
    def test_unchanged_theme_preserves_timestamp(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "themes.h"
            source = Path(directory) / "dark.qss"
            source.write_text("QWidget { color: white; }\n")
            with patch("sys.argv", ["pack_qt_themes", str(output), str(source)]):
                pack_qt_themes.main()
                timestamp = output.stat().st_mtime_ns
                pack_qt_themes.main()
                self.assertEqual(timestamp, output.stat().st_mtime_ns)
                source.write_text("QWidget { color: black; }\n")
                pack_qt_themes.main()
                self.assertIn("black", output.read_text())

    @unittest.skipUnless(shutil.which("cmake"), "CMake is required")
    def test_revision_is_scoped_to_its_consumers(self):
        root = (ROOT / "CMakeLists.txt").read_text()
        editor = (ROOT / "editor/CMakeLists.txt").read_text()
        global_definitions = "\n".join(re.findall(
            r"add_compile_definitions\(.*?\)", root, re.S
        ))
        runtime_scope = re.search(
            r"set_property\(SOURCE runtime/lib/c_api.cpp.*?\)", root, re.S
        ).group()
        editor_scope = re.search(
            r"set_property\(SOURCE main.cpp.*?\)", editor, re.S
        ).group()
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory)
            (source / "editor").mkdir()
            (source / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.21)\n'
                'project(RevisionScope LANGUAGES NONE)\n'
                'set(ATLAS_BUILD_REVISION probe_revision)\n'
                + global_definitions + '\n' + runtime_scope + '\n'
                'get_directory_property(definitions COMPILE_DEFINITIONS)\n'
                'if(definitions MATCHES "ATLAS_BUILD_REVISION")\n'
                'message(FATAL_ERROR "Revision leaks into dependencies")\n'
                'endif()\n'
                'get_source_file_property(value runtime/lib/c_api.cpp COMPILE_DEFINITIONS)\n'
                'if(NOT value MATCHES "probe_revision")\n'
                'message(FATAL_ERROR "Runtime revision missing")\n'
                'endif()\n'
                'add_subdirectory(editor)\n'
            )
            (source / "editor/CMakeLists.txt").write_text(
                editor_scope + '\n'
                'get_source_file_property(value main.cpp COMPILE_DEFINITIONS)\n'
                'if(NOT value MATCHES "probe_revision")\n'
                'message(FATAL_ERROR "Editor revision missing")\n'
                'endif()\n'
                'get_source_file_property(other inspector.cpp COMPILE_DEFINITIONS)\n'
                'if(other MATCHES "probe_revision")\n'
                'message(FATAL_ERROR "Revision leaks into editor sources")\n'
                'endif()\n'
            )
            subprocess.run([
                "cmake", "-S", str(source), "-B", str(source / "out"),
                "-G", "Ninja",
            ], check=True, capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()
