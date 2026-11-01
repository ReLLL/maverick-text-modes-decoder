#!/usr/bin/env python3
# Copyright (c) 2026 ReLLL and contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Package an explicit public release allowlist; never scan private folders."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import zipfile
import host_compat

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--binary", type=Path, required=True)
parser.add_argument("--host", choices=("brown", "sdrpp"), default="brown")
parser.add_argument("--variant", action="append", type=Path, default=[], help="Additional payload built for a different verified revision of the same host; repeatable")
args = parser.parse_args()
version = re.search(r"project\(cw_notebook VERSION ([0-9.]+)", (ROOT / "CMakeLists.txt").read_text()).group(1)
if not args.binary.is_file():
    parser.error("Binary not found; build Release first.")
manifest = args.binary.with_suffix(".host.json")
payloads = []
contracts = set()
for binary in [args.binary, *args.variant]:
    try:
        data = host_compat.read_manifest(binary)
        if data.get("host") != args.host or data.get("version") != version:
            raise ValueError("Payload host/version does not match this release.")
        if host_compat.sha256(binary) != data.get("binary_sha256"):
            raise ValueError("Payload checksum differs from its manifest.")
        if host_compat.architectures(binary) != ["arm64"]:
            raise ValueError("Public macOS package currently requires an arm64-only payload.")
        catalog = json.loads((ROOT / "resources/host-sdks.json").read_text())
        if not any(sdk.get("id") == data.get("sdk_id") and sdk.get("host") == args.host
                   and all(sdk.get(key) == data.get(key) for key in host_compat.SDK_IDENTITY_KEYS)
                   for sdk in catalog["sdks"]):
            raise ValueError("Payload does not match a reviewed public host/header SDK contract.")
        identity = {key: data.get(key) for key in ("host_core_sha256", "host_executable_sha256", "libraries")}
        key = hashlib.sha256(json.dumps(identity, sort_keys=True).encode()).hexdigest()[:24]
        if key in contracts:
            raise ValueError("Duplicate host/dependency variant; select one payload per verified image set.")
        contracts.add(key)
        payloads.append((binary, key))
    except (OSError, ValueError) as error:
        parser.error(str(error))
name = f"maverick-text-modes-decoder-{version}-{args.host}-macos-arm64"
folder = ROOT / "dist"
folder.mkdir(exist_ok=True)
archive = folder / f"{name}.zip"
files = {"cw_notebook.dylib": args.binary, "cw_notebook.host.json": manifest}
for binary, key in payloads[1:]:
    relative = f"variants/{key}"
    if f"{relative}/cw_notebook.dylib" in files:
        parser.error("Duplicate host variant; select exactly one payload per host build.")
    files[f"{relative}/cw_notebook.dylib"] = binary
    files[f"{relative}/cw_notebook.host.json"] = binary.with_suffix(".host.json")
for path in ("scripts/install.py", "scripts/host_compat.py", "README.md", "LICENSE", "CREDITS.md", "THIRD_PARTY_NOTICES.md", "CHANGELOG.md", "docs/rtty.md", "docs/psk.md", "docs/fec-modes.md", "docs/callsigns.md", "docs/WINDOWS.md", "resources/cty.dat", "resources/cty-LICENSE.txt", "resources/country-flags.tsv"):
    files[path] = ROOT / path
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as output:
    for relative, source in files.items():
        output.write(source, f"{name}/{relative}")
checksums = []
for host in ("brown", "sdrpp"):
    path = folder / f"maverick-text-modes-decoder-{version}-{host}-macos-arm64.zip"
    if path.is_file():
        checksums.append(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n")
(folder / "SHA256SUMS.txt").write_text("".join(checksums))
print(archive)
print(folder / "SHA256SUMS.txt")
