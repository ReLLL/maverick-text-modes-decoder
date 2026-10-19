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

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--binary", type=Path, required=True)
parser.add_argument("--host", choices=("brown", "sdrpp"), default="brown")
args = parser.parse_args()
version = re.search(r"project\(cw_notebook VERSION ([0-9.]+)", (ROOT / "CMakeLists.txt").read_text()).group(1)
if not args.binary.is_file():
    parser.error("Binary not found; build Release first.")
manifest = args.binary.with_suffix(".host.json")
if not manifest.is_file() or json.loads(manifest.read_text()).get("host") != args.host:
    parser.error("Matching cw_notebook.host.json required beside the binary.")
name = f"cw-notebook-{version}-{args.host}-macos-arm64"
folder = ROOT / "dist"
folder.mkdir(exist_ok=True)
archive = folder / f"{name}.zip"
files = {"cw_notebook.dylib": args.binary, "cw_notebook.host.json": manifest}
for path in ("scripts/install.py", "README.md", "LICENSE", "CREDITS.md", "THIRD_PARTY_NOTICES.md", "CHANGELOG.md", "docs/rtty.md", "docs/psk.md", "docs/fec-modes.md", "docs/callsigns.md", "resources/cty.dat", "resources/cty-LICENSE.txt", "resources/country-flags.tsv"):
    files[path] = ROOT / path
with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as output:
    for relative, source in files.items():
        output.write(source, f"{name}/{relative}")
checksums = []
for host in ("brown", "sdrpp"):
    path = folder / f"cw-notebook-{version}-{host}-macos-arm64.zip"
    if path.is_file():
        checksums.append(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}\n")
(folder / "SHA256SUMS.txt").write_text("".join(checksums))
print(archive)
print(folder / "SHA256SUMS.txt")
