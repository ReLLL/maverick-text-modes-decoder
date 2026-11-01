# Copyright (c) 2026 ReLLL and contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Real Mach-O regression tests, with no SDR++ installation or GUI required."""
import json
import os
from pathlib import Path
import platform
import plistlib
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).parents[1] / "scripts"))
import host_compat as compat
import install as installer


@unittest.skipUnless(platform.system() == "Darwin" and shutil.which("clang"), "requires macOS Command Line Tools")
class MachOCompatibilityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="maverick-macho-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.app = self.root / "Host with spaces.app"
        self.frameworks = self.app / "Contents/Frameworks"
        self.frameworks.mkdir(parents=True)
        executable = self.app / "Contents/MacOS/sdrpp"
        executable.parent.mkdir()
        with (self.app / "Contents/Info.plist").open("wb") as dest:
            plistlib.dump({"CFBundleVersion": "fixture"}, dest)
        self.compile(executable, "int main(void) { return 0; }")
        self.volk = self.frameworks / "libvolk.3.1.dylib"
        self.compile(self.volk, "int volk_fixture(void) { return 3; }", "-dynamiclib", "-install_name", "/usr/local/lib/libvolk.3.1.dylib")
        self.core = self.frameworks / "libsdrpp_core.dylib"
        self.compile(self.core, "int host_fixture(void) { return 7; }", "-dynamiclib", "-install_name", "@rpath/libsdrpp_core.dylib")
        self.binary = self.root / "cw_notebook.dylib"
        self.compile(self.binary, "#include <stdbool.h>\nbool maverick_host_compatible(void) { return true; } bool maverick_probe_host_libraries(void) { return true; } extern int volk_fixture(void); extern int host_fixture(void); int value(void) { return volk_fixture() + host_fixture(); }", "-bundle", self.core, self.volk)

    def compile(self, output, source, *args):
        code = self.root / (output.name + ".c")
        code.write_text(source)
        compat.run("clang", "-Wl,-headerpad_max_install_names", "-o", output, code, *args)

    def seal(self):
        compat.seal(self.binary, self.app, "sdrpp", "1.0.51")

    def test_absolute_build_dependency_is_repaired_and_loads_after_app_move(self):
        self.assertIn("/usr/local/lib/libvolk.3.1.dylib", compat.image(self.binary)["loads"])
        self.seal()
        self.assertIn("@rpath/libvolk.3.1.dylib", compat.image(self.binary)["loads"])
        relocated = self.root / "Relocated.app"
        self.app.rename(relocated)
        staged = self.root / "staged.dylib"
        compat.prepare(self.binary, staged, relocated, "sdrpp")
        code = "import ctypes, sys; lib=ctypes.CDLL(sys.argv[1]); assert lib.value() == 10"
        result = subprocess.run([sys.executable, "-c", code, str(staged)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(compat.sha256(self.binary), compat.read_manifest(self.binary)["binary_sha256"])

    def test_different_core_same_exports_rejected_before_profile_mutation(self):
        self.seal()
        self.compile(self.core, "int host_fixture(void) { return 8; }", "-dynamiclib", "-install_name", "@rpath/libsdrpp_core.dylib")
        profile = self.root / "profile"
        profile.mkdir()
        original = b'{"modules":[],"moduleInstances":{}}'
        (profile / "config.json").write_bytes(original)
        with self.assertRaisesRegex(ValueError, "Host ABI"):
            installer.install(self.binary, profile, "sdrpp", self.app)
        self.assertEqual((profile / "config.json").read_bytes(), original)
        self.assertFalse((profile / "plugins").exists())
        self.assertFalse((profile / "cw-notebook-backups").exists())

    def test_build_rejects_updated_host_with_stale_headers(self):
        source = self.root / "sdk-source"
        (source / "core/src").mkdir(parents=True)
        (source / "core/src/module.h").write_text("// independently pinned fixture headers\n")
        catalog = self.root / "sdks.json"
        sdk = {"id": "fixture", "host": "sdrpp", **compat.sdk_identity(source, self.app)}
        catalog.write_text(json.dumps({"sdks": [sdk]}))
        self.assertEqual(compat.verify_sdk(source, self.app, "sdrpp", catalog)["id"], "fixture")
        header = self.root / "host_images.h"
        compat.write_guard_header(self.app, header)
        previous = header.read_bytes()
        self.compile(self.core, "int host_fixture(void) { return 8; }", "-dynamiclib", "-install_name", "@rpath/libsdrpp_core.dylib")
        command = [sys.executable, str(Path(__file__).parents[1] / "scripts/host_compat.py"), "--app", str(self.app), "--host", "sdrpp", "--source", str(source), "--sdk-file", str(catalog), "--guard-header", str(header)]
        result = subprocess.run(command, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Unreviewed host/header SDK pairing", result.stderr)
        self.assertEqual(header.read_bytes(), previous)

    def test_missing_symbol_is_rejected_by_real_loader(self):
        # Model an incorrectly assembled SDK/app pairing, despite a valid manifest.
        self.compile(self.core, "int unrelated(void) { return 7; }", "-dynamiclib", "-install_name", "@rpath/libsdrpp_core.dylib")
        self.seal()
        with self.assertRaisesRegex(ValueError, "loader validation failed"):
            compat.prepare(self.binary, self.root / "staged.dylib", self.app, "sdrpp")

    def test_plugin_without_runtime_guard_is_rejected(self):
        self.compile(self.binary, "extern int host_fixture(void); int value(void) { return host_fixture(); }", "-bundle", self.core)
        self.seal()
        with self.assertRaisesRegex(ValueError, "Missing Maverick runtime ABI guard"):
            compat.prepare(self.binary, self.root / "staged.dylib", self.app, "sdrpp")
        with self.assertRaisesRegex(ValueError, "Missing Maverick runtime ABI guard"):
            compat.seal(self.binary, self.app, "sdrpp", "1.0.51", {"id": "fixture", "host_headers_sha256": "0" * 64})
        self.assertFalse(self.binary.with_suffix(".host.json").exists())

    def test_loader_probe_does_not_invoke_uninitialized_host_teardown(self):
        self.compile(self.core, "int host_fixture(void) { return 7; } __attribute__((destructor)) static void teardown(void) { __builtin_trap(); }", "-dynamiclib", "-install_name", "@rpath/libsdrpp_core.dylib")
        self.seal()
        compat.prepare(self.binary, self.root / "staged.dylib", self.app, "sdrpp")

    def test_runtime_guard_rejects_host_drift_even_if_manifest_is_regenerated(self):
        compat.write_guard_header(self.app, self.root / "host_images.h")
        source = self.root / "guard.cpp"
        source.write_text('#include "host_compatibility.h"\nextern "C" bool maverick_host_compatible(){ return cw::host::compatibilityError().empty(); }\nextern "C" bool maverick_probe_host_libraries(){ return cw::host::compatibilityError(true).empty(); }\nextern "C" int host_fixture(void);\nextern "C" int use_host(){ return host_fixture(); }\n')
        project = Path(__file__).parents[1]
        compat.run("clang++", "-std=c++17", "-bundle", "-Wl,-headerpad_max_install_names", "-I", self.root, "-I", project / "src", source, project / "src/host_compatibility.cpp", self.core, "-o", self.binary)
        self.seal()
        good = self.root / "good.dylib"
        compat.prepare(self.binary, good, self.app, "sdrpp")
        # A non-host executable may pass only the explicit library probe. The
        # normal factory guard must reject it even with the right loaded core.
        code = "import ctypes, os, sys; lib=ctypes.CDLL(sys.argv[1]); lib.maverick_host_compatible.restype=ctypes.c_bool; lib.maverick_probe_host_libraries.restype=ctypes.c_bool; os._exit(0 if not lib.maverick_host_compatible() and lib.maverick_probe_host_libraries() else 1)"
        result = subprocess.run([sys.executable, "-c", code, str(good)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.compile(self.core, "int host_fixture(void) { return 8; }", "-dynamiclib", "-install_name", "@rpath/libsdrpp_core.dylib")
        self.seal()
        with self.assertRaisesRegex(ValueError, "runtime host ABI guard"):
            compat.prepare(self.binary, self.root / "bad.dylib", self.app, "sdrpp")

    def test_runtime_guard_checks_mapped_uuid_after_host_file_replacement(self):
        old_core = self.root / "old-core.dylib"
        shutil.copy2(self.core, old_core)
        self.compile(self.core, "int host_fixture(void) { return 8; }", "-dynamiclib", "-install_name", "@rpath/libsdrpp_core.dylib")
        compat.write_guard_header(self.app, self.root / "host_images.h")
        source = self.root / "guard.cpp"
        source.write_text('#include "host_compatibility.h"\nextern "C" bool maverick_host_compatible(){ return cw::host::compatibilityError().empty(); }\nextern "C" bool maverick_probe_host_libraries(){ return cw::host::compatibilityError(true).empty(); }\nextern "C" int host_fixture(void);\nextern "C" int use_host(){ return host_fixture(); }\n')
        project = Path(__file__).parents[1]
        compat.run("clang++", "-std=c++17", "-bundle", "-Wl,-headerpad_max_install_names", "-I", self.root, "-I", project / "src", source, project / "src/host_compatibility.cpp", self.core, "-o", self.binary)
        self.seal()
        staged = self.root / "staged.dylib"
        compat.prepare(self.binary, staged, self.app, "sdrpp")
        replacement = self.root / "new-core.dylib"
        shutil.copy2(self.core, replacement)
        os.replace(old_core, self.core)
        code = """import ctypes, os, sys
core = ctypes.CDLL(sys.argv[1])
os.replace(sys.argv[2], sys.argv[1])
lib = ctypes.CDLL(sys.argv[3])
lib.maverick_probe_host_libraries.restype = ctypes.c_bool
os._exit(1 if lib.maverick_probe_host_libraries() else 0)
"""
        result = subprocess.run([sys.executable, "-c", code, str(self.core), str(replacement), str(staged)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_runtime_guard_finds_executable_after_inserted_dylib(self):
        executable = self.app / "Contents/MacOS/sdrpp"
        self.compile(executable, '#include <dlfcn.h>\n#include <stdbool.h>\nint main(int argc, char** argv){ if(argc!=2)return 2; void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL); if(!lib)return 3; bool (*guard)(void)=(bool(*)(void))dlsym(lib,"maverick_host_compatible"); return guard && guard()?0:4; }')
        compat.write_guard_header(self.app, self.root / "host_images.h")
        source = self.root / "guard.cpp"
        source.write_text('#include "host_compatibility.h"\nextern "C" bool maverick_host_compatible(){ return cw::host::compatibilityError().empty(); }\nextern "C" bool maverick_probe_host_libraries(){ return cw::host::compatibilityError(true).empty(); }\nextern "C" int host_fixture(void);\nextern "C" int use_host(){ return host_fixture(); }\n')
        project = Path(__file__).parents[1]
        compat.run("clang++", "-std=c++17", "-bundle", "-Wl,-headerpad_max_install_names", "-I", self.root, "-I", project / "src", source, project / "src/host_compatibility.cpp", self.core, "-o", self.binary)
        self.seal()
        staged = self.root / "staged.dylib"
        compat.prepare(self.binary, staged, self.app, "sdrpp")
        injected = self.root / "inserted.dylib"
        self.compile(injected, "int inserted_fixture(void){return 0;}", "-dynamiclib")
        env = dict(os.environ, DYLD_INSERT_LIBRARIES=str(injected))
        result = subprocess.run([str(executable), str(staged)], env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_missing_and_corrupt_dependencies_are_rejected(self):
        self.seal()
        self.volk.unlink()
        with self.assertRaisesRegex(ValueError, "Missing host library libvolk.3.1"):
            compat.prepare(self.binary, self.root / "staged.dylib", self.app, "sdrpp")
        self.volk.write_bytes(b"not a Mach-O image")
        with self.assertRaisesRegex(ValueError, "libvolk.3.1.dylib changed"):
            compat.prepare(self.binary, self.root / "staged.dylib", self.app, "sdrpp")

    def test_real_install_preserves_disabled_settings_and_allows_repeat(self):
        self.seal()
        profile = self.root / "profile"
        profile.mkdir()
        original = {"modules": [], "moduleInstances": {"CW Notebook": {"module": "cw_notebook", "enabled": False, "rttyEnabled": False}}}
        config = profile / "config.json"
        config.write_text(json.dumps(original))
        backup = installer.install(self.binary, profile, "sdrpp", self.app)
        self.assertEqual(json.loads((backup / "config.json").read_text()), original)
        second = installer.install(self.binary, profile, "sdrpp", self.app)
        self.assertTrue((second / "cw_notebook.dylib").is_file())
        self.assertEqual(json.loads(config.read_text())["moduleInstances"], original["moduleInstances"])
        compat.probe(profile / "plugins/cw_notebook.dylib")


if __name__ == "__main__":
    unittest.main()
