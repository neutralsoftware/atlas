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


if __name__ == "__main__":
    unittest.main()
