# Maverick Host Dependency Compatibility And Safe Recovery

## Files modified

- `CMakeLists.txt`, `scripts/host_compat.py`, `scripts/install.py`, `scripts/package.py`.
- `resources/host-sdks.json`, `src/host_compatibility.h`, `src/host_compatibility.cpp`, the factory boundary in `src/plugin.cpp`.
- Installer/compatibility regression suites and the README, architecture, release, validation and changelog documentation.

Existing hidden-startup and Windows work remains in place. No installed application bundle or existing user profile is replaced.

## Functions created or modified

- `sdk_identity` and `verify_sdk`: match reviewed header/image pairs before configuration and sealing.
- `library_closure` and `bundled_volk`: resolve the selected bundle's actual non-system dependencies.
- `seal`, `validate`, `select_payload`, `prepare`, `probe`: create manifests, select eligible local payloads, repair and sign a temporary copy, then bind and validate it.
- `pid_executable` and `running_host`: identify running hosts through kernel executable paths, including relative launches.
- `compatibilityError`, `maverick_host_compatible`, `maverick_probe_host_libraries` and the module factory: distinguish strict runtime checks from explicit installer probing and stop unsafe initialization.

## Implementation approach

Exact ABI contracts are the compatibility boundary. Build and installer checks reject unsupported pairs before installation; supported path relocation and local payload selection are automatic. Profile backups, atomic replacement and rollback preserve configuration and intentional disabled choices. No updater, downloader, extra DSP worker or host modification is introduced.

## Improvement opportunities

Add reviewed SDK contracts and payloads when upstream publishes another host build. Keep fixture topology valid, record whole-process exit separately from lifecycle completion, and run actual RF/replay reception before extending reception-quality claims. Maintain older host payloads when packaging a newer primary payload.

## Reflection

A permissive filename fallback would have hidden the actual compatibility question. Exact header/image pairs plus native binding, runtime image checks, regression fixtures and real-host lifecycle checks provide a reviewable boundary. AppKit startup and Brown's missing-Radio shutdown behavior must remain separate from plugin failures.
