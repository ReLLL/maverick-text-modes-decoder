# Release process

Maintainer: [ReLLL](https://github.com/ReLLL). Repository: [maverick-text-modes-decoder](https://github.com/ReLLL/maverick-text-modes-decoder).

## Prepare

1. Increment the three-part version in `CMakeLists.txt` and update `CHANGELOG.md`.
2. Run `sh scripts/check.sh` and the sanitizer commands in `CONTRIBUTING.md`.
3. Build Release against the supported Brown headers and app. Set the deployment target explicitly; current public Apple Silicon target is macOS 15.0.
4. Install the candidate with Brown closed. Check live reception, pause/resume, text selection, metadata copying, clear/archive, input isolation and app restart.
5. Review every staged path for credentials, private paths, recordings and personal information. Keep Git author/committer identity at the intended public GitHub username and noreply address.
6. Sanitize prose without rewriting upstream license texts or source code.

## Package

```sh
python3 scripts/package.py --binary build-release/cw_notebook.dylib
python3 scripts/package.py --host sdrpp --binary build-sdrpp/cw_notebook.dylib
```

This creates separate versioned brown/sdrpp ZIPs and a combined `SHA256SUMS.txt` in ignored `dist/`, using an explicit allowlist. Each archive includes its binary and host manifest, installer, README, changelog, license and third-party notices. It does not include logs, fixtures, config or private build directories. Brown is the primary target; test both hosts' native receiver integration, replay/retune, UI library and installer before claiming support. Never interchange binaries.

Inspect `otool -L`, `vtool -show-build`, and `strings` for dependency paths, minimum OS and personal build paths. Do not re-sign or modify Brown. This community plugin is not a notarized standalone application; document that accurately. Publish source at the same tag as any binary.

For the Windows SDR# preview, follow `docs/WINDOWS.md` and run
`scripts/package_windows.py` with the native build, managed adapter, matching
MSYS2 runtime/package manifest and `--runtime-sources` directory. That directory
contains `runtime-sources.json`, `sources.json` and the exact source archives.
The script checks binary packages, extracted DLLs and source hashes, then emits
the plugin ZIP plus a separate matching runtime-source ZIP. Publish both with
checksums and the exact project source. Never include the SDR# host/SDK,
AGENTS.md, private profiles, logs, recordings or validation-console files.

Validate fresh install, upgrade, configured plugin paths, damaged-package
rejection and host-architecture rejection. Record actual-host and synthetic
decoder evidence separately. Keep Windows labeled preview until its documented
operator/performance gaps are resolved. Do not announce both projects until
their Windows work and the owner's joint-announcement hold are cleared.

## Publish and verify

1. Commit reviewed source, push the default branch and create a version tag.
2. Create a GitHub release for that exact tag with the ZIP and checksum file.
3. Download both release assets into a fresh temporary directory and verify the checksum and archive contents.
4. Verify the public repository, license detection, tag/commit, release notes and contributor links.
5. Record tested OS, CPU architecture and Brown revision. Do not claim untested platform support.
6. For fixes, publish a new patch version. Do not silently replace existing release assets.

Issues and pull requests are the update/contribution queue. Users update manually with the backed-up installer and may subscribe to GitHub release notifications. There is no automatic updater or network executable delivery.
