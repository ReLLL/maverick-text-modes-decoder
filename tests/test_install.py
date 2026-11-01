# Copyright (c) 2026 ReLLL and contributors
# SPDX-License-Identifier: GPL-3.0-or-later
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import sys

sys.path.insert(0, str(Path(__file__).parents[1] / "scripts"))

spec = importlib.util.spec_from_file_location("installer", Path(__file__).parents[1] / "scripts/install.py")
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)


class InstallerTests(unittest.TestCase):
    def setUp(self):
        # These tests cover transactional profile changes. Real Mach-O preflight
        # and ABI policy are covered independently by the compatibility suites.
        prepare = mock.patch.object(installer.host_compat, "prepare", side_effect=lambda source, staged, app, host: installer.shutil.copy2(source, staged))
        prepare.start()
        self.addCleanup(prepare.stop)
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.binary = self.root / "new.dylib"
        self.binary.write_bytes(b"new plugin")
        self.binary.with_suffix(".host.json").write_text('{"host":"brown"}')
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

    def test_preflight_failure_leaves_profile_and_backup_untouched(self):
        with mock.patch.object(installer.host_compat, "prepare", side_effect=ValueError("unverified ABI")):
            with self.assertRaisesRegex(ValueError, "unverified ABI"):
                installer.install(self.binary, self.root)
        self.assertEqual(json.loads(self.config.read_text()), self.original)
        self.assertFalse((self.root / "plugins").exists())
        self.assertFalse((self.root / "cw-notebook-backups").exists())

    def test_cli_uses_exact_process_query_and_installs(self):
        with mock.patch.object(sys, "argv", ["install.py", "--binary", str(self.binary)]), mock.patch.object(installer.platform, "system", return_value="Darwin"), mock.patch.object(installer.subprocess, "run", return_value=mock.Mock(returncode=1)) as processes, mock.patch.object(installer.host_compat, "select_payload", return_value=self.binary), mock.patch.object(installer, "install", return_value=self.root) as install:
            self.assertEqual(installer.main(), 0)
        self.assertEqual(processes.call_args.args[0], ["pgrep", "-x", "sdrpp"])
        install.assert_called_once()

    def test_cli_refuses_running_or_uninspectable_host(self):
        for result in (0, 3):
            with self.subTest(result=result), mock.patch.object(sys, "argv", ["install.py", "--binary", str(self.binary)]), mock.patch.object(installer.platform, "system", return_value="Darwin"), mock.patch.object(installer.subprocess, "run", return_value=mock.Mock(returncode=result, stdout="123\n")), mock.patch.object(installer, "pid_executable", return_value=Path("/Applications/SDR++Brown.app/Contents/MacOS/sdrpp")), mock.patch.object(installer, "install") as install:
                with self.assertRaises(SystemExit):
                    installer.main()
                install.assert_not_called()

    def test_running_host_uses_resolved_kernel_path_and_fails_closed(self):
        executable = (self.root / "App.app/Contents/MacOS/sdrpp").resolve()
        with mock.patch.object(installer.subprocess, "run", return_value=mock.Mock(returncode=0, stdout="123\n")), mock.patch.object(installer, "pid_executable", return_value=executable):
            self.assertTrue(installer.running_host(self.root / "App.app"))
            self.assertFalse(installer.running_host(self.root / "Other.app"))
        with mock.patch.object(installer.subprocess, "run", return_value=mock.Mock(returncode=0, stdout="123\n")), mock.patch.object(installer, "pid_executable", side_effect=ValueError("inspection denied")):
            with self.assertRaisesRegex(ValueError, "inspection denied"):
                installer.running_host(self.root / "App.app")


if __name__ == "__main__":
    unittest.main()
