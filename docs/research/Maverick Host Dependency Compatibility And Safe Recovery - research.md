# Maverick Host Dependency Compatibility And Safe Recovery

Date: 2026-11-01. Status: implementation and validation for 1.0.51.

The [Kondor installation report](https://github.com/ReLLL/kondor-weak-signal-modes-decoder/issues/4) prompted an independent Maverick check. Maverick's existing standard payload also requested `/usr/local/lib/libvolk.3.1.dylib`; loading it against the current SDR++ bundle reproduced the missing-library failure. Its installer previously checked only the host family and CPU architecture, so installation could succeed without proving that the payload could load.

The [official nightly release](https://github.com/AlexandreRouma/SDRPlusPlus/releases/tag/nightly) currently contains SDR++ 1.3.2, updated October 23. Its historical release target is not the rebuilt binary's source revision. The [successful official CI run](https://github.com/AlexandreRouma/SDRPlusPlus/actions/runs/39896187183) identifies `f10fb98af8b80de01de94e60c17608e4281e2515`; that source reports 1.3.2 and supplied the candidate's matching headers. Existing Brown and legacy standard source snapshots remain separate.

Library filenames, semantic versions and exported symbol inventories do not establish C++/ImGui data-layout compatibility. The implementation therefore pins reviewed source-header/host-image contracts, builds separate payloads, and automatically selects only matching local variants. Recovery changes dependency paths in a staged plugin copy, never host files, library aliases or unverified version substitutions. Real `RTLD_NOW` binding and the mandatory runtime library guard precede replacement.

The runtime factory checks the actual `MH_EXECUTE` image, mapped Mach-O UUIDs and on-disk image hashes. The explicit installer-probe export omits the main executable because the installer checks the selected executable separately. An ordinary factory call cannot use that exemption. Inert instances preserve the user's requested enablement without creating workers or touching host/UI state.

Independent review caught macOS process-query syntax, stale-header recertification, missing-guard acceptance, implicit executable-check exemptions and relative launch paths. These were corrected. A real hidden-host check also exposed that an inserted dylib can occupy dyld index zero; the main executable must be identified by `MH_EXECUTE`, with a permanent native regression fixture.

Validation separates module lifecycle completion from zero host-process exit. A minimal Brown fixture lacking a Radio stream exposed a shutdown failure in `QSOAudioRecorder::end`; its source leaves `radioStream` unset when there are no streams and dereferences it during cleanup. A separate startup report stops in HIServices/AppKit before plugin loading. Subsequent startup hangs were sampled in AppKit's crash-state restoration modal. Test-only volatile restoration settings, hidden windows and controlled nonzero failure exits avoid changing saved application preferences or disguising failed checks. The valid smoke fixture includes the host's own Radio module and a null output sink.

See [validation](../validation.md) for verified results and remaining limits. No source push, release, user contact, host replacement or user-profile installation is part of this task.
