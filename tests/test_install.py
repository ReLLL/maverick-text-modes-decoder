# Copyright (c) 2026 ReLLL and contributors
# SPDX-License-Identifier: GPL-3.0-or-later
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

spec = importlib.util.spec_from_file_location("installer", Path(__file__).parents[1] / "scripts/install.py")
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)


class InstallerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.binary = self.root / "new.dylib"
        self.binary.write_bytes(b"new plugin")
        self.config = self.root / "config.json"
        self.original = {"modules": ["other"], "moduleInstances": {}, "menuElements": [{"name": "Radio", "open": True}], "frequency": 7000000}
        self.config.write_text(json.dumps(self.original))

    def test_install_update_backup_and_idempotence(self):
        backup = installer.install(self.binary, self.root)
        self.assertEqual(json.loads((backup / "config.json").read_text()), self.original)
        target = self.root / "plugins/cw_notebook.dylib"
        self.assertEqual(target.read_bytes(), b"new plugin")
        self.binary.write_bytes(b"updated plugin")
        backup = installer.install(self.binary, self.root)
        self.assertEqual((backup / target.name).read_bytes(), b"new plugin")
        data = json.loads(self.config.read_text())
        self.assertEqual(data["modules"].count(str(target)), 1)
        self.assertEqual(sum(x["name"] == "Maverick Text Modes Decoder" for x in data["menuElements"]), 1)
        self.assertEqual(data["frequency"], 7000000)

    def test_bad_config_changes_nothing(self):
        self.config.write_text('{"modules": "bad"}')
        with self.assertRaises(ValueError):
            installer.install(self.binary, self.root)
        self.assertEqual(self.config.read_text(), '{"modules": "bad"}')
        self.assertFalse((self.root / "plugins").exists())

    def test_previous_names_preserve_instance_and_disabled_state(self):
        original = dict(self.original)
        original["moduleInstances"] = {"CW Notebook": {"module": "cw_notebook", "enabled": False, "receiverGrid": "KM69", "rttyEnabled": False}}
        original["menuElements"] = [{"name": name, "open": False} for name in ("CW Notebook", "Maverick Decoder", "Maverick CW / RTTY / JS8 / Olivia / PSK31 / PSK63 Decoder", "Maverick Text Modes Decoder", "Radio")]
        self.config.write_text(json.dumps(original))
        installer.install(self.binary, self.root)
        data = json.loads(self.config.read_text())
        self.assertEqual(data["moduleInstances"], original["moduleInstances"])
        self.assertEqual([item["name"] for item in data["menuElements"]], ["Maverick Text Modes Decoder", "Radio"])
        self.assertFalse(data["menuElements"][0]["open"])

    def test_separate_host_profiles_and_disabled_preference(self):
        self.binary.with_suffix(".host.json").write_text('{"host":"sdrpp","version":"1.0.35"}')
        original = self.config.read_bytes()
        with self.assertRaises(ValueError):
            installer.install(self.binary, self.root, "brown")
        self.assertEqual(self.config.read_bytes(), original)
        self.assertFalse((self.root / "cw-notebook-backups").exists())
        installer.install(self.binary, self.root, "sdrpp")
        data = json.loads(self.config.read_text())
        data["moduleInstances"]["CW Notebook"]["enabled"] = False
        data["moduleInstances"]["CW Notebook"]["receiverGrid"] = "KM69"
        self.config.write_text(json.dumps(data))
        installer.install(self.binary, self.root, "sdrpp")
        self.assertFalse(json.loads(self.config.read_text())["moduleInstances"]["CW Notebook"]["enabled"])
        self.assertEqual(json.loads(self.config.read_text())["moduleInstances"]["CW Notebook"]["receiverGrid"], "KM69")

    def test_standard_host_requires_manifest_and_correct_profile(self):
        with self.assertRaises(ValueError):
            installer.install(self.binary, self.root, "sdrpp")
        self.binary.with_suffix(".host.json").write_text('{"host":"sdrpp"}')
        with self.assertRaises(ValueError):
            installer.validate_host(self.binary, "sdrpp", self.root / "sdrpp-brown")
        self.binary.with_suffix(".host.json").write_text('[]')
        with self.assertRaises(ValueError):
            installer.install(self.binary, self.root, "sdrpp")

    def test_config_failure_restores_previous_plugin(self):
        installer.install(self.binary, self.root)
        previous = self.config.read_bytes()
        self.binary.write_bytes(b"update")
        replace = installer.os.replace
        def fail_config(source, dest):
            if Path(dest) == self.config:
                raise OSError("simulated config failure")
            return replace(source, dest)
        with mock.patch.object(installer.os, "replace", side_effect=fail_config):
            with self.assertRaises(OSError):
                installer.install(self.binary, self.root)
        self.assertEqual(self.config.read_bytes(), previous)
        self.assertEqual((self.root / "plugins/cw_notebook.dylib").read_bytes(), b"new plugin")


if __name__ == "__main__":
    unittest.main()
