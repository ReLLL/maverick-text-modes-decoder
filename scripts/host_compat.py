#!/usr/bin/env python3
# Copyright (c) 2026 ReLLL and contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Offline macOS host ABI manifests and staged dependency-path recovery.

A version string or exported-symbol list cannot establish C++/ImGui ABI safety.
Only payloads built and validated for the exact host images are eligible.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import plistlib
import re
import shutil
import subprocess
import sys
import tempfile


SYSTEM_PREFIXES = ("/usr/lib/", "/System/Library/")
SDK_IDENTITY_KEYS = ("host_core_sha256", "host_executable_sha256", "libraries", "host_headers_sha256")


def run(*args):
    try:
        result = subprocess.run([str(a) for a in args], capture_output=True, text=True, timeout=30)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise ValueError(f"Cannot run {args[0]}: {error}. Install Apple's Command Line Tools if missing.") from error
    if result.returncode:
        raise ValueError(f"{args[0]} failed: {(result.stderr or result.stdout).strip()}")
    return result.stdout


def sha256(path):
    with Path(path).open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest() if hasattr(hashlib, "file_digest") else hashlib.sha256(source.read()).hexdigest()


def image(path, arch=None):
    """Read load commands rather than treating LC_ID_DYLIB as a dependency."""
    args = ["otool"] + (["-arch", arch] if arch else []) + ["-l", path]
    loads, rpaths, minimum, uuid = [], [], "0.0", ""
    command = ""
    for line in run(*args).splitlines():
        fields = line.strip().split()
        if fields[:1] == ["cmd"]:
            command = fields[1]
        elif command in ("LC_LOAD_DYLIB", "LC_LOAD_WEAK_DYLIB", "LC_REEXPORT_DYLIB", "LC_LOAD_UPWARD_DYLIB") and fields[:1] == ["name"]:
            loads.append(line.strip()[5:].rsplit(" (offset", 1)[0])
        elif command == "LC_RPATH" and fields[:1] == ["path"]:
            rpaths.append(line.strip()[5:].rsplit(" (offset", 1)[0])
        elif command == "LC_BUILD_VERSION" and fields[:1] == ["platform"] and fields[1] not in ("1", "MACOS"):
            raise ValueError(f"{Path(path).name} is not a macOS binary.")
        elif command == "LC_BUILD_VERSION" and fields[:1] == ["minos"]:
            minimum = fields[1]
        elif command == "LC_VERSION_MIN_MACOSX" and fields[:1] == ["version"]:
            minimum = fields[1]
        elif command == "LC_UUID" and fields[:1] == ["uuid"]:
            uuid = fields[1].replace("-", "").lower()
    return {"loads": loads, "rpaths": rpaths, "minimum": minimum, "uuid": uuid}


def architectures(path):
    return run("lipo", "-archs", path).strip().split()


def system_library(name):
    return name.startswith(SYSTEM_PREFIXES)


def library_closure(binary, app):
    """Resolve only the selected bundle; never Homebrew or a similarly named fork."""
    frameworks = (app / "Contents/Frameworks").resolve()
    libraries, pending = {}, [binary]
    while pending:
        path = pending.pop()
        for arch in architectures(path):
            for name in image(path, arch)["loads"]:
                if system_library(name):
                    continue
                leaf = Path(name).name
                resolved = (frameworks / leaf).resolve()
                if resolved.parent != frameworks or not resolved.is_file():
                    raise ValueError(f"Missing host library {leaf}; reinstall the matching host or choose --app. No library aliases are created.")
                if leaf not in libraries:
                    libraries[leaf] = resolved
                    pending.append(resolved)
    return libraries


def bundled_volk(app):
    libraries = library_closure(app / "Contents/Frameworks/libsdrpp_core.dylib", app)
    matches = {path for name, path in libraries.items() if re.fullmatch(r"libvolk(?:\.[0-9]+)+\.dylib", name)}
    if len(matches) != 1:
        raise ValueError("The selected host core must resolve exactly one bundled VOLK library; no version is guessed.")
    return matches.pop()


def sdk_identity(source, app):
    """Pair complete public host headers with exact application/dependency images."""
    headers = []
    for relative in ("core/src", "decoder_modules/radio/src"):
        headers += [path for path in (source / relative).rglob("*") if path.is_file() and path.suffix in (".h", ".hpp", ".inl")]
    if not (source / "core/src/module.h").is_file() or not headers:
        raise ValueError("Matching host source headers are missing.")
    digest = hashlib.sha256()
    for path in sorted(headers):
        digest.update(path.relative_to(source).as_posix().encode() + b"\0")
        digest.update(bytes.fromhex(sha256(path)))
    core = app / "Contents/Frameworks/libsdrpp_core.dylib"
    libraries = library_closure(core, app)
    libraries[core.name] = core
    return {"host_core_sha256": sha256(core),
            "host_executable_sha256": sha256(app / "Contents/MacOS/sdrpp"),
            "host_headers_sha256": digest.hexdigest(),
            "libraries": {name: sha256(path) for name, path in sorted(libraries.items())}}


def verify_sdk(source, app, host, sdk_file):
    identity = sdk_identity(source, app)
    catalog = json.loads(sdk_file.read_text())
    for sdk in catalog.get("sdks", []):
        if sdk.get("host") == host and all(sdk.get(key) == identity[key] for key in SDK_IDENTITY_KEYS):
            return sdk
    raise ValueError("Unreviewed host/header SDK pairing. The host or headers changed; select matching headers and add a reviewed contract to the SDK catalog before rebuilding. Regenerating fingerprints with stale headers is unsafe.")


def version_tuple(value):
    return tuple(int(part) for part in value.split(".")) + (0,) * (3 - len(value.split(".")))


def check_images(binary, app, libraries):
    arch = platform.machine()
    executable = app / "Contents/MacOS/sdrpp"
    for path in [binary, executable, *libraries.values()]:
        if arch not in architectures(path):
            raise ValueError(f"Architecture mismatch: {path.name} does not support {arch}. Use a matching host, Python and plugin architecture.")
        required = image(path, arch)["minimum"]
        if version_tuple(required) > version_tuple(platform.mac_ver()[0]):
            raise ValueError(f"{path.name} requires macOS {required} or newer.")


def read_manifest(binary):
    path = binary.with_suffix(".host.json")
    if not path.is_file():
        raise ValueError("Missing compatibility manifest; download a current matching package or rebuild.")
    data = json.loads(path.read_text())
    if not isinstance(data, dict) or data.get("schema") != 2:
        raise ValueError("This older package has no verified ABI manifest; download a current package or rebuild against this host.")
    libraries = data.get("libraries")
    if not isinstance(libraries, dict) or not libraries or any(
        not isinstance(name, str) or Path(name).name != name or not name.endswith(".dylib")
        or not isinstance(digest, str) or not re.fullmatch(r"[0-9a-f]{64}", digest)
        for name, digest in libraries.items()
    ):
        raise ValueError("Invalid library manifest; re-extract the original package.")
    return data


def validate(binary, app, host):
    data = read_manifest(binary)
    if data.get("host") != host:
        raise ValueError("Payload targets a different host; choose the matching package and --host.")
    if sha256(binary) != data.get("binary_sha256"):
        raise ValueError("Plugin checksum differs from its manifest; re-extract the package or rebuild.")
    core = app / "Contents/Frameworks/libsdrpp_core.dylib"
    executable = app / "Contents/MacOS/sdrpp"
    if not core.is_file() or not executable.is_file():
        raise ValueError("Selected --app is not a complete SDR++ host application.")
    if sha256(core) != data.get("host_core_sha256") or sha256(executable) != data.get("host_executable_sha256"):
        raise ValueError(f"Host ABI is unverified for this payload (built for {data.get('host_version', 'unknown')}); use a matching packaged variant or rebuild with this host's matching headers. Renaming VOLK cannot verify C++/ImGui layouts.")
    for name, digest in data["libraries"].items():
        path = app / "Contents/Frameworks" / name
        if not path.is_file():
            raise ValueError(f"Missing host library {name}; reinstall the matching host.")
        if sha256(path) != digest:
            raise ValueError(f"Host library {name} changed since this payload was validated; use a matching package or rebuild.")
    libraries = library_closure(binary, app)
    if set(libraries) != set(data["libraries"]):
        raise ValueError("Dependency closure differs from the manifest; re-extract the package or rebuild.")
    check_images(binary, app, libraries)
    return data


def select_payload(binary, app, host, candidates=()):
    errors = []
    for candidate in dict.fromkeys([binary, *candidates]):
        try:
            validate(candidate, app, host)
            return candidate
        except (ValueError, OSError) as error:
            errors.append(f"{candidate.parent.name}/{candidate.name}: {error}")
    raise ValueError("No compatible local payload.\n" + "\n".join(errors))


def probe(binary):
    """Bind symbols in a disposable process; never call plugin lifecycle methods."""
    # The host normally owns core-global teardown. Brown's globals cannot be
    # destructed safely without its application initialization. Finish only
    # after RTLD_NOW succeeds, then bypass this artificial process teardown.
    code = """import ctypes, os, sys
lib = ctypes.CDLL(sys.argv[1], mode=os.RTLD_NOW | os.RTLD_LOCAL)
guard = getattr(lib, 'maverick_probe_host_libraries', None)
if not guard or not getattr(lib, 'maverick_host_compatible', None):
    print('Missing Maverick runtime ABI guard; rebuild the matching current plugin.', file=sys.stderr, flush=True)
    os._exit(2)
guard.restype = ctypes.c_bool
guard.argtypes = []
if not guard():
    print('Maverick runtime host ABI guard rejected the loaded images.', file=sys.stderr, flush=True)
    os._exit(2)
os._exit(0)
"""
    env = {key: value for key, value in os.environ.items() if not key.startswith("DYLD_")}
    try:
        result = subprocess.run([sys.executable, "-c", code, str(binary.resolve())], env=env, capture_output=True, text=True, timeout=30)
    except subprocess.TimeoutExpired as error:
        raise ValueError("Plugin loader validation timed out; installed files unchanged.") from error
    if result.returncode:
        raise ValueError(f"Plugin loader validation failed (exit {result.returncode}); installed files unchanged:\n{result.stderr.strip() or result.stdout.strip()}")


def prepare(binary, staged, app, host):
    """Mutate only a staging copy, after verifying the source and host identity."""
    data = validate(binary, app, host)
    shutil.copy2(binary, staged)
    if sha256(staged) != data["binary_sha256"]:
        raise ValueError("Plugin copy checksum mismatch; installed files unchanged.")
    commands = image(staged)
    for name in dict.fromkeys(commands["loads"]):
        if not system_library(name):
            target = str((app / "Contents/Frameworks" / Path(name).name).resolve())
            if name != target:
                run("install_name_tool", "-change", name, target, staged)
    frameworks = str((app / "Contents/Frameworks").resolve())
    for path in dict.fromkeys(commands["rpaths"]):
        run("install_name_tool", "-delete_rpath", path, staged)
    run("install_name_tool", "-add_rpath", frameworks, staged)
    run("codesign", "--force", "--sign", "-", staged)
    run("codesign", "--verify", "--strict", staged)
    probe(staged)
    # Refuse a host/package changed during preflight before committing anything.
    validate(binary, app, host)
    return data


def seal(binary, app, host, version, sdk=None):
    """Build-time operation; creates a manifest for an explicit matching SDK/app."""
    libraries = library_closure(binary, app)
    check_images(binary, app, libraries)
    commands = image(binary)
    for name in dict.fromkeys(commands["loads"]):
        if not system_library(name) and name != "@rpath/" + Path(name).name:
            run("install_name_tool", "-change", name, "@rpath/" + Path(name).name, binary)
    for path in dict.fromkeys(commands["rpaths"]):
        run("install_name_tool", "-delete_rpath", path, binary)
    run("install_name_tool", "-add_rpath", "@executable_path/../Frameworks", binary)
    run("codesign", "--force", "--sign", "-", binary)
    with (app / "Contents/Info.plist").open("rb") as source:
        info = plistlib.load(source)
    data = {"schema": 2, "host": host, "version": version,
            "host_version": str(info.get("CFBundleVersion", "unknown")),
            "binary_sha256": sha256(binary),
            "host_core_sha256": sha256(app / "Contents/Frameworks/libsdrpp_core.dylib"),
            "host_executable_sha256": sha256(app / "Contents/MacOS/sdrpp"),
            "libraries": {name: sha256(path) for name, path in sorted(libraries.items())}}
    if sdk:
        data.update({"sdk_id": sdk["id"], "host_headers_sha256": sdk["host_headers_sha256"]})
    manifest = binary.with_suffix(".host.json")
    manifest.write_text(json.dumps(data, indent=2) + "\n")
    if sdk:
        try:
            with tempfile.TemporaryDirectory(prefix="cw-build-preflight-") as stage:
                prepare(binary, Path(stage) / binary.name, app, host)
        except (OSError, ValueError):
            manifest.unlink()
            raise


def write_guard_header(app, destination):
    core = app / "Contents/Frameworks/libsdrpp_core.dylib"
    libraries = library_closure(core, app)
    libraries[core.name] = core
    libraries["sdrpp"] = app / "Contents/MacOS/sdrpp"
    lines = ["// Generated host image fingerprints; no host paths are embedded.",
             "#pragma once", "namespace cw::host {", "struct ExpectedImage { const char* name; const char* sha256; const char* uuids; };",
             "inline constexpr ExpectedImage expectedImages[] = {"]
    for name, path in sorted(libraries.items()):
        uuids = [image(path, arch)["uuid"] for arch in architectures(path)]
        if any(not re.fullmatch(r"[0-9a-f]{32}", value) for value in uuids):
            raise ValueError(f"Missing Mach-O UUID for {name}; cannot guard the loaded image.")
        lines.append(f'    {{"{name}", "{sha256(path)}", "{",".join(uuids)}"}},')
    lines += ["};", "}", ""]
    contents = "\n".join(lines)
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.is_file() or destination.read_text() != contents:
        destination.write_text(contents)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--guard-header", type=Path, help="Generate compile-time runtime-guard fingerprints")
    parser.add_argument("--print-volk", action="store_true", help="Resolve the VOLK actually required by the selected core")
    parser.add_argument("--app", required=True, type=Path)
    parser.add_argument("--host", choices=("brown", "sdrpp"))
    parser.add_argument("--version")
    parser.add_argument("--source", type=Path, help="Matching host source header tree")
    parser.add_argument("--sdk-file", type=Path, default=Path(__file__).parents[1] / "resources/host-sdks.json")
    args = parser.parse_args()
    if args.print_volk:
        print(bundled_volk(args.app.resolve()))
    elif args.guard_header:
        if not args.source or not args.host:
            parser.error("--guard-header requires --source and --host for SDK verification.")
        verify_sdk(args.source.resolve(), args.app.resolve(), args.host, args.sdk_file)
        write_guard_header(args.app.resolve(), args.guard_header)
    elif args.binary and args.host and args.version:
        if not args.source:
            parser.error("Sealing a build requires its matching --source and reviewed SDK contract.")
        sdk = verify_sdk(args.source.resolve(), args.app.resolve(), args.host, args.sdk_file)
        seal(args.binary.resolve(), args.app.resolve(), args.host, args.version, sdk)
    else:
        parser.error("Provide --guard-header or --binary, --host and --version.")


if __name__ == "__main__":
    main()
