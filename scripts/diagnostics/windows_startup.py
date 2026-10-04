import json
import os
from pathlib import Path
import subprocess
import sys
import time
import winreg

from PIL import ImageGrab
from pywinauto import Application

executable = Path(sys.argv[1])
output = Path(sys.argv[2])
output.mkdir(parents=True, exist_ok=True)
project = output / "project"
project.mkdir(exist_ok=True)
(project / "project.atlas").write_text('''name = "Windows startup regression"
[game]
main_scene = "main.ascene"
assets = []
[window]
dimensions = [640, 360]
mouse_capture = false
multisampling = false
[renderer]
default = "deferred"
global_illumination = false
''')
(project / "main.ascene").write_text(json.dumps({
    "name": "Windows startup regression",
    "objects": [{"name": "Cube", "type": "solid", "solid_type": "cube",
                 "position": [0, 0, 0], "scale": [1, 1, 1]}],
    "camera": {"position": [0, 0, -5], "target": [0, 0, 0], "fov": 60},
    "lights": [{"type": "ambient", "intensity": 0.5}],
    "targets": [{"name": "Main Target", "type": "multisampled", "render": True, "display": True}],
}))
with winreg.CreateKey(winreg.HKEY_CURRENT_USER, r"Software\Neutral Software\Atlas Engine\toolchain") as key:
    winreg.SetValueEx(key, "installationPromptDismissed", 0, winreg.REG_SZ, "true")
log = Path(os.environ["LOCALAPPDATA"]) / "Neutral Software" / "Atlas Engine" / "editor.log"
app = Application(backend="uia").start(str(executable), work_dir=str(executable.parent))
try:
    browser = app.window(title_re="Atlas Engine.*Projects")
    browser.wait("visible", timeout=45)
    browser.child_window(title="Open existing", control_type="Button").invoke()
    dialog = app.window(title="Open an Atlas project")
    dialog.wait("visible", timeout=20)
    dialog.child_window(auto_id="1148", control_type="Edit").set_edit_text(str(project / "project.atlas"))
    dialog.child_window(auto_id="1", control_type="Button").invoke()
    deadline = time.monotonic() + 180
    while time.monotonic() < deadline:
        text = log.read_text(errors="replace") if log.exists() else ""
        if "native crash" in text or "Runtime unavailable" in text or not app.is_process_running():
            raise RuntimeError("Editor crashed or rejected runtime startup\n" + text)
        if "Project ready" in text:
            time.sleep(10)
            text = log.read_text(errors="replace")
            if not app.is_process_running() or "native crash" in text or "runtime frame failed" in text:
                raise RuntimeError("Editor failed after its first frame\n" + text)
            ImageGrab.grab().save(output / "project-ready.png")
            app.top_window().close()
            app.wait_for_process_exit(timeout=30)
            print("Packaged Windows editor opened a PBR project and shut down successfully", flush=True)
            break
        time.sleep(2)
    else:
        raise RuntimeError("Project startup timed out\n" + text)
finally:
    ImageGrab.grab().save(output / "desktop.png")
    if log.exists():
        (output / "editor.log").write_bytes(log.read_bytes())
        print(log.read_text(errors="replace"), flush=True)
    if app.is_process_running():
        try:
            app.top_window().print_control_identifiers()
        finally:
            subprocess.run(["taskkill", "/F", "/T", "/PID", str(app.process)], check=False)
