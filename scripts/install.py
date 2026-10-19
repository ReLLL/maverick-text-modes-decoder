#!/usr/bin/env python3
# Copyright (c) 2026 ReLLL and contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Install outside the selected host's signed bundle; preserve configuration backups."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

import platform
import tempfile


HOSTS = {"brown": ("SDR++Brown", "sdrpp-brown"), "sdrpp": ("SDR++", "sdrpp")}


def validate_host(binary: Path, host: str, root: Path) -> None:
    if host not in HOSTS:
        raise ValueError("Unknown host; choose brown or sdrpp.")
    for other, (_, folder) in HOSTS.items():
        if other != host and root.name == folder:
            raise ValueError("Profile belongs to the other host; choose its matching binary and --host.")
    manifest = binary.with_suffix(".host.json")
    if not manifest.is_file():
        if host == "brown":
            return  # Earlier Brown releases have no manifest.
        raise ValueError("SDR++ requires its matching cw_notebook.host.json beside the binary.")
    data = json.loads(manifest.read_text())
    if not isinstance(data, dict) or data.get("host") != host:
        raise ValueError("Binary targets a different host; select the matching package and --host.")


def install(binary: Path, root: Path, host: str = "brown") -> Path:
    validate_host(binary, host, root)
    config = root / "config.json"
    if not binary.is_file() or not config.is_file():
        raise ValueError("Plugin or config missing. Build/download the matching plugin and launch the host once first.")
    data = json.loads(config.read_text())
    if not isinstance(data, dict):
        raise ValueError("Host config must be a JSON object.")
    modules = data.setdefault("modules", [])
    instances = data.setdefault("moduleInstances", {})
    menu = data.get("menuElements", [])
    if not (isinstance(modules, list) and all(isinstance(x, str) for x in modules)
            and isinstance(instances, dict) and isinstance(menu, list)
            and all(isinstance(x, dict) and "name" in x for x in menu)):
        raise ValueError("Unsupported host config structure; no files changed.")
    current = instances.get("CW Notebook")
    if current is not None and (not isinstance(current, dict) or current.get("module") != "cw_notebook"):
        raise ValueError("An unrelated module already uses the name CW Notebook.")
    target = root / "plugins/cw_notebook.dylib"
    if str(target) not in modules:
        modules.append(str(target))
    instances["CW Notebook"] = dict(current or {})
    instances["CW Notebook"].update({"enabled": current.get("enabled", True) if current else True, "module": "cw_notebook"})
    aliases = {"CW Notebook", "Maverick Decoder", "Maverick CW / RTTY / JS8 / Olivia / PSK31 / PSK63 Decoder", "Maverick Text Modes Decoder: CW / RTTY / JS8 / Olivia / PSK31 / PSK63", "Maverick Text Modes Decoder"}
    previous_menu = next((item for item in menu if item["name"] in aliases), {})
    entry = {"open": True, **previous_menu, "name": "Maverick Text Modes Decoder"}
    data["menuElements"] = [entry] + [x for x in menu if x["name"] not in aliases]
    encoded = (json.dumps(data, indent=4) + "\n").encode()
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%d-%H%M%S-%fZ")
    backup = root / "cw-notebook-backups" / stamp
    backup.mkdir(parents=True, exist_ok=False)
    shutil.copy2(config, backup / "config.json")
    had_binary = target.exists()
    if had_binary:
        shutil.copy2(target, backup / target.name)
    target.parent.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".cw-install-", dir=root) as stage:
        staged_binary = Path(stage) / target.name
        staged_config = Path(stage) / "config.json"
        shutil.copy2(binary, staged_binary)
        staged_config.write_bytes(encoded)
        staged_config.chmod(config.stat().st_mode & 0o777)
        if hashlib.sha256(staged_binary.read_bytes()).digest() != hashlib.sha256(binary.read_bytes()).digest():
            raise OSError("Plugin copy checksum mismatch; installed files unchanged.")
        os.replace(staged_binary, target)
        try:
            os.replace(staged_config, config)
        except OSError:
            if had_binary:
                shutil.copy2(backup / target.name, staged_binary)
                os.replace(staged_binary, target)
            else:
                target.unlink()
            raise
    return backup


def main() -> int:
    project = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=project / "build/cw_notebook.dylib")
    parser.add_argument("--host", choices=HOSTS, default="brown", help="brown (primary/default) or sdrpp; requires matching binary")
    parser.add_argument("--config-dir", type=Path, help="Custom profile; defaults to the selected host's macOS profile")
    args = parser.parse_args()
    if platform.system() != "Darwin":
        parser.error("The plugin installer currently supports macOS only.")
    name, folder = HOSTS[args.host]
    process_pattern = r"SDR\+\+Brown.app/Contents/MacOS/sdrpp|^./sdrpp" if args.host == "brown" else r"SDR\+\+.app/Contents/MacOS/sdrpp|^./sdrpp"
    processes = subprocess.run(["pgrep", "-f", process_pattern], capture_output=True)
    if processes.returncode == 0:
        parser.error(f"Quit {name} before installing; it writes its config on exit.")
    if processes.returncode != 1:
        parser.error("Could not check whether the host is running; no files changed.")
    binary = args.binary.expanduser().resolve()
    if not binary.is_file():
        parser.error("Plugin binary missing. Build it first or pass --binary from an extracted release.")
    probe = subprocess.run(["lipo", str(binary), "-verify_arch", platform.machine()], capture_output=True)
    if probe.returncode:
        parser.error("Plugin architecture does not match this Mac.")
    try:
        profile = args.config_dir or Path.home() / "Library/Application Support" / folder
        backup = install(binary, profile.expanduser().resolve(), args.host)
    except (OSError, ValueError) as error:
        print(f"Install failed: {error}", file=sys.stderr)
        return 1
    print(f"Installed Maverick Text Modes Decoder for {name}. Restart {name} to load it.")
    print(f"Configuration and previous plugin backup: {backup}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
