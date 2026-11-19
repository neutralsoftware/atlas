import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import atlas_tasks


class ConfigureTargetTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.build = Path(self.directory.name)
        self.build_patch = patch.object(atlas_tasks, "BUILD", self.build)
        self.build_patch.start()
        self.addCleanup(self.build_patch.stop)

    def cache(self, backend="AUTO", bezel="OFF", ftet="OFF"):
        (self.build / "build.ninja").write_text("")
        (self.build / "CMakeCache.txt").write_text(
            f"BACKEND:STRING={backend}\nBEZEL_NATIVE:BOOL={bezel}\n"
            f"ENABLE_FTET_WILD:BOOL={ftet}\n"
        )

    @patch.object(atlas_tasks, "execute")
    def test_unchanged_configuration_does_not_run_cmake(self, execute):
        self.cache()
        atlas_tasks.configure_target("AUTO", "OFF", "OFF")
        atlas_tasks.configure_target("auto", "off", "off")
        execute.assert_not_called()

    @patch.object(atlas_tasks, "execute")
    def test_changed_options_reconfigure(self, execute):
        self.cache()
        atlas_tasks.configure_target("METAL", "ON", "ON")
        execute.assert_called_once()
        self.assertIn("-DBACKEND=METAL", execute.call_args.args)
        self.assertIn("-DBEZEL_NATIVE=ON", execute.call_args.args)
        self.assertIn("-DENABLE_FTET_WILD=ON", execute.call_args.args)

    @patch.object(atlas_tasks, "execute")
    def test_missing_build_graph_reconfigures(self, execute):
        self.cache()
        (self.build / "build.ninja").unlink()
        atlas_tasks.configure_target("AUTO", "OFF", "OFF")
        execute.assert_called_once()

    @patch.object(atlas_tasks, "execute")
    def test_first_configuration_runs_cmake(self, execute):
        atlas_tasks.configure_target("AUTO", "OFF", "OFF")
        execute.assert_called_once()

    @patch.object(atlas_tasks, "execute")
    def test_native_configuration_uses_its_own_cache_and_options(self, execute):
        self.cache()
        native = self.build / "native-tests"
        native.mkdir()
        (native / "build.ninja").write_text("")
        (native / "CMakeCache.txt").write_text(
            (self.build / "CMakeCache.txt").read_text()
            + "ATLAS_BUILD_EDITOR:BOOL=ON\n"
        )
        atlas_tasks.configure_target(
            "AUTO", "OFF", "OFF", directory=native,
            options={"ATLAS_BUILD_EDITOR": "OFF"},
        )
        self.assertIn(native, execute.call_args.args)
        self.assertIn("-DATLAS_BUILD_EDITOR=OFF", execute.call_args.args)
        execute.reset_mock()
        cache = native / "CMakeCache.txt"
        cache.write_text(cache.read_text().replace("EDITOR:BOOL=ON", "EDITOR:BOOL=OFF"))
        atlas_tasks.configure_target(
            "AUTO", "OFF", "OFF", directory=native,
            options={"ATLAS_BUILD_EDITOR": "OFF"},
        )
        execute.assert_not_called()


class NativeTestTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.build = self.root / "build/native-tests"
        for name in ("ROOT", "NATIVE_BUILD"):
            value = self.root if name == "ROOT" else self.build
            patcher = patch.object(atlas_tasks, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)
        self.tests = self.root / "tests"
        (self.tests / "simple").mkdir(parents=True)
        (self.tests / "simple/main.cpp").write_text("int main() {}\n")
        (self.tests / "default.txt").write_text("simple\n")

    @patch.object(atlas_tasks, "configure_target")
    @patch.object(atlas_tasks, "execute")
    @patch.object(atlas_tasks, "executable")
    def test_default_builds_native_target_and_runs_in_test_folder(
        self, executable, execute, configure
    ):
        executable.return_value = self.build / "bin/atlas_test_simple"
        atlas_tasks.run_test()
        configure.assert_called_once_with(
            "AUTO", "OFF", None, directory=self.build,
            options={"ATLAS_BUILD_EDITOR": "OFF", "ATLAS_BUILD_RUNTIME": "OFF",
                     "ATLAS_BUILD_TESTS": "ON"},
        )
        self.assertEqual(execute.call_args_list[0].args, (
            "cmake", "--build", self.build,
            "--target", "atlas_test_simple", "--parallel",
        ))
        self.assertEqual(execute.call_args_list[1].args, (executable.return_value,))
        self.assertEqual(execute.call_args.kwargs["cwd"], self.tests / "simple")

    @patch.object(atlas_tasks, "execute")
    def test_named_test_forwards_backend_options(self, execute):
        (self.tests / "other").mkdir()
        (self.tests / "other/main.cpp").write_text("int main() {}\n")
        with patch.object(atlas_tasks, "configure_target") as configure:
            with patch.object(atlas_tasks, "executable"):
                atlas_tasks.build_native_test("other", "VULKAN", "ON", "OFF")
        self.assertEqual(configure.call_args.args, ("VULKAN", "ON", "OFF"))
        self.assertIn("atlas_test_other", execute.call_args.args)

    @patch.object(atlas_tasks, "execute")
    def test_scene_only_test_fails_before_configuration(self, execute):
        (self.tests / "legacy").mkdir()
        (self.tests / "legacy/project.atlas").write_text("")
        with self.assertRaisesRegex(ValueError, "run-project"):
            atlas_tasks.run_test("legacy")
        execute.assert_not_called()

    def test_invalid_names_cannot_escape_tests(self):
        for name in ("../outside", "a/b", "a\\b", "-target", ".", "a b"):
            with self.subTest(name=name):
                with self.assertRaises(ValueError):
                    atlas_tasks.test_directory(name)

    @patch.object(atlas_tasks, "execute")
    def test_scaffold_is_native_and_does_not_invoke_cli(self, execute):
        atlas_tasks.add_test("my-feature")
        self.assertEqual(
            (self.tests / "my-feature/main.cpp").read_text(),
            (self.tests / "simple/main.cpp").read_text(),
        )
        self.assertEqual(list((self.tests / "my-feature").iterdir()),
                         [self.tests / "my-feature/main.cpp"])
        execute.assert_not_called()
        with self.assertRaises(FileExistsError):
            atlas_tasks.add_test("my-feature")

    @patch.object(atlas_tasks, "WINDOWS", True)
    def test_windows_manifest_adds_runtime_dll_directories(self):
        self.build.mkdir(parents=True)
        dll_dir = self.root / "dependency/bin"
        (self.build / "atlas-test-simple-dlls-Debug.txt").write_text(
            f"{dll_dir / 'dependency.dll'}\n\n"
        )
        env = atlas_tasks.native_environment(self.tests / "simple")
        self.assertIn(str(dll_dir), env["PATH"])

    @patch.object(atlas_tasks, "execute")
    @patch.object(atlas_tasks, "executable")
    def test_legacy_runner_resolves_default_project(self, executable, execute):
        (self.tests / "simple/project.atlas").write_text("")
        atlas_tasks.run_project()
        self.assertEqual(execute.call_args_list[0].args,
                         ("atlas", "script", "compile"))
        self.assertEqual(execute.call_args.args[1],
                         self.tests / "simple/project.atlas")


if __name__ == "__main__":
    unittest.main()
