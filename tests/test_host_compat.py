# Copyright (c) 2026 ReLLL and contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Installer compatibility policy; native Mach-O cases live in test_macos_compat."""
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).parents[1] / "scripts"))
import host_compat as compat
check_images = compat.check_images


class CompatibilityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.binary = self.root / "cw_notebook.dylib"
        self.binary.write_bytes(b"plugin")
        self.app = self.root / "SDR++.app"
        self.frameworks = self.app / "Contents/Frameworks"
        self.frameworks.mkdir(parents=True)
        self.core = self.frameworks / "libsdrpp_core.dylib"
        self.core.write_bytes(b"known core ABI")
        self.executable = self.app / "Contents/MacOS/sdrpp"
        self.executable.parent.mkdir()
        self.executable.write_bytes(b"known host")
        self.volk = self.frameworks / "libvolk.3.3.dylib"
        self.volk.write_bytes(b"known VOLK")
        self.data = {"schema": 2, "host": "sdrpp", "version": "1.0.51",
                     "binary_sha256": compat.sha256(self.binary),
                     "host_version": "1.3.2", "host_executable_sha256": compat.sha256(self.executable),
                     "host_core_sha256": compat.sha256(self.core),
                     "libraries": {p.name: compat.sha256(p) for p in (self.core, self.volk)}}
        self.write_manifest()
        self.patches = [mock.patch.object(compat, "check_images"),
                        mock.patch.object(compat, "library_closure", return_value={p.name: p for p in (self.core, self.volk)})]
        for patch in self.patches:
            patch.start()
            self.addCleanup(patch.stop)

    def write_manifest(self):
        self.binary.with_suffix(".host.json").write_text(json.dumps(self.data))

    def test_known_host_and_relocated_app_are_accepted(self):
        compat.validate(self.binary, self.app, "sdrpp")
        moved = self.root / "Moved.app"
        self.app.rename(moved)
        compat.library_closure.return_value = {p.name: moved / "Contents/Frameworks" / p.name for p in (self.core, self.volk)}
        compat.validate(self.binary, moved, "sdrpp")

    def test_unknown_core_rejected_even_with_same_version_and_symbols(self):
        self.core.write_bytes(b"different C++ layouts, same exported symbols")
        with self.assertRaisesRegex(ValueError, "ABI.*matching"):
            compat.validate(self.binary, self.app, "sdrpp")

    def test_same_core_with_changed_volk_is_not_silently_certified(self):
        self.volk.write_bytes(b"unverified new library")
        with self.assertRaisesRegex(ValueError, "libvolk.*changed"):
            compat.validate(self.binary, self.app, "sdrpp")

    def test_tampered_plugin_and_legacy_manifest_rejected(self):
        self.binary.write_bytes(b"corrupt")
        with self.assertRaisesRegex(ValueError, "checksum"):
            compat.validate(self.binary, self.app, "sdrpp")
        self.data = {"host": "sdrpp"}
        self.write_manifest()
        with self.assertRaisesRegex(ValueError, "older.*manifest"):
            compat.validate(self.binary, self.app, "sdrpp")

    def test_wrong_host_and_dependency_traversal_rejected(self):
        with self.assertRaisesRegex(ValueError, "different host"):
            compat.validate(self.binary, self.app, "brown")
        self.data["libraries"]["../elsewhere.dylib"] = "0" * 64
        self.write_manifest()
        with self.assertRaisesRegex(ValueError, "library manifest"):
            compat.validate(self.binary, self.app, "sdrpp")

    def test_missing_library_reports_name(self):
        self.volk.unlink()
        with self.assertRaisesRegex(ValueError, "libvolk.3.3.dylib"):
            compat.validate(self.binary, self.app, "sdrpp")

    def test_selects_local_matching_variant(self):
        old = self.root / "old/cw_notebook.dylib"
        old.parent.mkdir()
        old.write_bytes(b"old")
        old.with_suffix(".host.json").write_text('{"schema":2,"host":"sdrpp"}')
        with mock.patch.object(compat, "validate", side_effect=lambda p, a, h: {} if p == self.binary else (_ for _ in ()).throw(ValueError("wrong ABI"))):
            self.assertEqual(compat.select_payload(old, self.app, "sdrpp", [self.binary]), self.binary)

    def test_build_selects_core_dependency_rather_than_guessing_newest_volk(self):
        (self.frameworks / "libvolk.9.9.dylib").write_bytes(b"unused")
        self.assertEqual(compat.bundled_volk(self.app), self.volk)

    def test_architecture_and_minimum_os_rejections(self):
        with mock.patch.object(compat.platform, "machine", return_value="arm64"), mock.patch.object(compat, "architectures", return_value=["x86_64"]):
            with self.assertRaisesRegex(ValueError, "Architecture mismatch"):
                check_images(self.binary, self.app, {})
        with mock.patch.object(compat.platform, "machine", return_value="arm64"), mock.patch.object(compat.platform, "mac_ver", return_value=("15.0", (), "")), mock.patch.object(compat, "architectures", return_value=["arm64"]), mock.patch.object(compat, "image", return_value={"minimum": "26.0"}):
            with self.assertRaisesRegex(ValueError, "requires macOS 26"):
                check_images(self.binary, self.app, {})

    def test_loader_failure_does_not_replace_existing_files(self):
        stage = self.root / "stage.dylib"
        stage.write_bytes(b"stage")
        with mock.patch.object(compat, "run"), mock.patch.object(compat, "image", return_value={"loads": [], "rpaths": []}), mock.patch.object(compat, "probe", side_effect=ValueError("dyld missing symbol")):
            with self.assertRaisesRegex(ValueError, "missing symbol"):
                compat.prepare(self.binary, stage, self.app, "sdrpp")
        self.assertEqual(self.binary.read_bytes(), b"plugin")
        self.assertEqual(self.core.read_bytes(), b"known core ABI")


if __name__ == "__main__":
    unittest.main()
