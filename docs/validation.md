---
read_when: Changing thresholds, supported ranges, compatibility, or reliability claims
---

# Validation

## Maverick rename and Windows preview 1.0.50, 2026-10-19

The existing GitHub repository was renamed in place, preserving earlier
releases and redirects. Public links and display names use Maverick Text Modes
Decoder; module/config IDs, log paths and the stable ImGui window ID remain
unchanged. The installer regression preserves disabled CW/RTTY state and saved
grid while removing previous menu aliases.

Fifteen shared core targets, six macOS installer tests and both matching native
ImGui suites pass. Both macOS host modules build. The prior 1.0.50 full
ASan/UBSan run passed all fifteen core targets. No changed decoder thresholds
or live Brown installation are included in the rename.

The official Windows SDR# 1.0.0.1921 host loads the separate adapter/native
worker. The operator confirmed synthetic CW/RTTY reception; CW log content and
parent-exit cleanup were checked, then synthetic recordings/logs removed.
Eight native decoder/core targets, the real Windows transcript suite,
stop/retune/filter/epoch IPC test and five installer scenarios pass. Production
SDK downconversion/resampling/filtering passes at 192 kS/s through 3.2 MS/s
without modifying host buffers. Detailed evidence and emulated queue-gap
limits are recorded in [Windows preview](WINDOWS.md). This does not establish
on-air sensitivity, gap-free full-rate reception or long-soak stability.

## Local 1.0.49 conservative RTTY annotations, 2026-10-19

The supplied YL3NU/LYL3NU pattern first failed a production-parser regression:
the single-letter extension acquired a conflicting country flag. The new
RTTY-only context check withholds that flag/tag when the shorter recognized
call occurs at least twice and the longer token occurs once. Every received
character remains intact. X/G/B examples, lowercase text, incomplete tokens,
hashed calls, provisional text, portable calls and repeated longer calls are
covered. This does not prove which callsign was transmitted.

Fifteen core targets and five installer tests pass. Both matching host modules
build, both native-ImGui suites pass, and the changed parser/transcript pass
ASan/UBSan. A widget regression preserves selection during ordinary appends
and clears it safely when later corroboration removes an earlier flag. Copied
reports retain the original raw text. One Release run used 6.36 CPU milliseconds
per 100,000-character guarded parse versus 4.42 without the guard; parsing
remains cached until text/options change and counts track at most 256 tokens.

This candidate changes no DSP, acquisition, framing, tone, baud, scan budgets
or worker lifecycle. It improves annotation caution, not RF decoding accuracy.
Version 1.0.49 was installed into Brown on 2026-10-19 while the host was closed.
The loaded module matches the tested Release binary, SHA-256
`07b3b850c1bfe550bea18f7f065122464e6964ddbe19cf50ce1f98ca97276407`.
Native monitoring and live reception were observed after launch. All 34 saved
CW log prefixes verify; Radio and RTL source configuration are unchanged.
The dial and capture center changed during live use after launch; no retune
was performed by this installation. HTTP remains off. The installer backed up
the previous plugin and configuration.

Release preparation repeated all fifteen core targets, five installer checks,
all fifteen ASan/UBSan targets, both matching host builds and both native-ImGui
suites successfully. Both 15-file host packages match their binary/manifests
and exclude project instructions, receiver data and personal build paths.

## Local 1.0.48 signal-qualified SNR retention, 2026-10-17

A production-decoder regression reproduced manual CW treating noise as a
retained signal measurement, and RTTY replacing its reading after frames
stopped (20.53 to 3.40 dB in the deterministic fixture). Requiring keyed
spectral evidence for CW and newly accepted RTTY frames fixes both cases.
PSK now requires fresh symbols and a recently detected carrier before refreshing
metadata. Decoder acquisition, text and thresholds are unchanged.

The new signal -> noise -> returning-signal checks pass for automatic/manual
CW, RTTY and both PSK rates. They verify that noise neither changes the held
value nor refreshes its measurement time, while returning signals update it.
All fifteen core targets, five installer checks and fifteen ASan/UBSan targets
pass. Both matching host modules build. Existing JS8/Olivia observations remain
attached to accepted decoded frames. There is no additional FFT, scanner or
worker; the existing host UI widget is unchanged.

Version 1.0.48 is installed and loaded in live Brown after the approved restart.
Installed SHA-256: `8eaf9d266e1025a76814b2024b80077c0dddeb51e452e934ae32af25d3094c2f`.
All 34 saved CW log prefixes verify. Source, rate, gain, decimation and Radio
configuration are preserved; normal HTTP remains off. The saved 14.217 MHz USB
dial and capture center were restored after Brown's startup VFO clamp.
Only three digital-tab reports were exported before UI interference prevented
completing the backup.
The user approved restarting with twelve open memory-only digital tabs.
Native quit input could not complete through the changing UI; the exact host
process was terminated before installation. Both 15-file host archives match
their binary/manifests and exclude private paths and project instructions.
These fixture and startup checks do not establish field accuracy or a long soak.

## Local 1.0.47 Settings and transcript annotations, 2026-10-17

Fourteen core targets, five installer checks, fourteen ASan/UBSan targets,
both matching host UI suites and the standard-host transcript memory check
pass. Both host modules build for arm64, minimum macOS 15.0. The production
multiline widget tests cover selection during append, Unicode flag copying,
glyph coloring, annotation toggles and the paged Settings layout. A warm
100,000-character annotation parse used 5.26 CPU milliseconds in one Release
run; parsing is cached until text/options change. No DSP workers or spectrum
scans were added.

An isolated native Brown replay decoded CW, RTTY, PSK31, PSK63, JS8 and Olivia.
All four Settings pages were inspected. Turning callsign detection off removed
annotations; turning it back on restored them. Native JS8 and Olivia views
showed colored callsigns, inline flags and callsign tags. The JS8 pane showed
the received K1ABC/FN42 header and inferred United States flag/country.
Finite SNR and last-received timestamps remained unchanged after stopping
input. The final replay checkpoint reported zero IQ/FEC gaps. QRZ URL encoding
is tested; no tag was opened and no received text was sent externally.

Version 1.0.47 is installed outside Brown's app bundle and receiving live RF.
Installed SHA-256: `805545f1a28015e317c33d039c81c73c49ee3c2bdc00bf648b129863507cdd4b`.
The loaded module matches the Release build. All 34 saved CW log prefixes
retain their hashes. The three pre-restart digital tabs were privately
exported; digital tabs remain memory-only. Source, rate, gain, decimation and
Radio mode/filter configuration were preserved. Brown again clamped the
saved VFO offset on startup; the native frequency control restored the saved
14.217 MHz USB dial. Its tuning policy moved the capture center 24.924 kHz
lower. Normal-profile HTTP is off. Live RTTY also displayed an Italian
callsign tag and inline flags. These are bounded checks, not RF sensitivity
or long-soak stability measurements. Public release remains 1.0.42.

## Local 1.0.46 reception metadata checkpoint, 2026-10-17

The inactive-report SNR regression fails before the fix and passes afterward.
Metadata checks cover NaN/infinity rejection, zero dB, current/held labels,
the 119/120-second age boundary, portable calls, exact-call exceptions,
unknown/maritime calls and copied report provenance. CW worker checks retain
SNR and received time across pause/resume. Both matching host UI suites pass
the colored SNR/age rendering and existing tab close/reopen checks.

The earlier JS8 Ultra flake reproduced in five of 30 repeated all-mode trials.
Thirty Ultra-only trials passed. Synchronization reads four trailing spectral
columns whose FFT windows exceed the input and were never initialized.
Zeroing these unused columns produced 150 successful decodes over 30 repeated
five-mode trials. Thirteen core targets and five installer checks pass.
Native metadata validation is still pending at this checkpoint; the installed
receiver remains 1.0.45.

## Local 1.0.45 CW tab lifecycle, 2026-10-17

Twelve core targets and five installer checks pass. Both matching host modules
build, and both real-ImGui suites exercise the production CW tab close button,
reopening, rapid off/on requests, stale worker snapshots and automatic safety
pauses. The changed CW worker and standard-host UI also pass ASan/UBSan.
One initial unchanged JS8 Ultra fixture failed; the complete rerun passed.
The intermittent failure's cause remains unresolved; this patch changes no FEC
decoder thresholds.

An isolated native Brown replay verifies that the tab X disables CW, clears
learning state and stops its private channel input counter. Confirmed text
stays intact while RTTY, PSK31 and PSK63 continue receiving. Close all leaves CW
disabled. Re-enabling the checkbox selects CW, retains its text, resumes input
and reacquires the fixture's 20 WPM timing. The toolbar and help no longer
advertise the removed proprietary modem integration.

The tested 1.0.45 binary is installed outside Brown's app bundle. Normal RTL-SDR
reception resumed with all native modes enabled and HTTP off. All 34 saved CW
log prefixes retain their hashes. Installer backups preserve the prior binary
and configuration. Public release remains 1.0.42; this update is unpublished.
Brown clamped Radio's saved offset during startup. Moving the capture center
40.145 kHz lower and using a -280 kHz offset restored the saved 6958.000 kHz
dial without changing source, rate, gain, decimation, mode or filter. This is
a local profile adjustment, not a change to Brown or a plugin tuning policy.
Both local 1.0.45 archives contain the matching host binary/manifest and the
11-file public allowlist, with no project instructions or private build paths.

## Local 1.0.44 JS8/Olivia candidate, 2026-10-17

Twelve portable core targets, five installer checks, twelve ASan/UBSan targets
and both matching real-ImGui transcript suites pass. Both host modules build
for arm64, minimum macOS 15.0. The existing CW/RTTY/PSK regression checks remain
included. No on-air sensitivity or long-soak claim follows from these fixtures.

JS8 encoding matches frozen normal/modified Costas tone sequences generated
independently by the original JS8Call Fortran/Boost CRC implementation. All five
receive speeds unpack reference text. A noisy weak normal-speed fixture decodes
the received K1ABC/FN42 header; noise alone does not qualify. All seven Olivia
formats decode their generated reference text with the correct format.
Separate Gaussian noise, carrier and RTTY interference fixtures do not qualify
as Olivia. SNR tests check the 2500 Hz reference, a tenfold power increase and
unavailable capture-edge measurements. A discovery regression prevents strong
narrow carriers with broad FFT skirts from monopolizing Olivia probes.

The integrated 90-second IQ fixture decodes JS8 and Olivia together without
FEC packet loss or an aliased duplicate JS8 tab. Disabling both releases their
slots and retains text; Close all removes it. One measured Release run used
10.1 total CPU seconds, including 9.65 FEC worker seconds, for 90 seconds of
input. Seven separate Olivia format trials used 0.82-1.07 CPU seconds each for
100 seconds of audio. These are local workload measurements, not total
receiver CPU guarantees or a calibrated sensitivity comparison.

An isolated, real-time Brown replay at 96 kS/s decoded CW, 200 Hz RTTY, PSK31,
PSK63, JS8 Normal and Olivia 8/250 concurrently. One JS8 tab appeared at the
reference RF frequency, with received K1ABC/FN42 header fields. Every mode
reported finite local RX SNR during reception. At about 116 seconds, no IQ/FEC
packets had dropped and the FEC worker had used 9.17 CPU seconds. Later replay
wrap/stop activity reported IQ discontinuities; FEC loss remained zero. Native
Olivia text, format, activity, header placeholders, clipboard report and AI
provider menu were checked. No provider was selected and no received text was
sent. Turning Olivia off released its slots while keeping text and other modes.

An earlier integration run exposed an out-of-bounds lookup in the vendored
Olivia synchronization noise metric. The receiver now selects the noise value
for the actual best phase/frequency; full memory checks pass with that fix.
Strong synchronization scores are capped rather than discarded. Format
selection requires a sustained winner, varied text and valid recent controls.
The replay generator also uses double-precision oscillators to avoid creating
artificial carriers through float phase quantization.

JS8 SNR comes from
its decoder and has not been calibrated against the spectral estimates used
by the other modes. Header fields are received claims, not verified identities.
Public release remains 1.0.42; this candidate is unpublished.

The final local 1.0.44 module is installed outside Brown's app bundle and
matches the tested binary. Normal-profile reception is running with CW, RTTY,
PSK, JS8 and Olivia enabled. HTTP is off. Source, gain, decimation, receiver
location, time correction and stream settings match their pre-update values;
the user's subsequent live tuning is retained. All 33 existing CW log prefixes
match their recorded hashes. Four still-open RTTY reports were exported before
restart. Installer backups retain the prior plugin and configuration. Both
local archives pass explicit allowlist, binary, manifest, architecture,
minimum-OS and private-path checks. Project AGENTS.md stays ignored.

## Local 1.0.43 PSK candidate, 2026-10-17

Ten portable core targets, five installer tests, ten ASan/UBSan targets and
both matching real-ImGui suites pass. Both host modules build. PSK fixtures
use an independently transcribed Varicode reference and cosine phase reversals.
Checks cover both rates, arbitrary mid-transmission joins, carrier offsets,
800 ppm clock error in both directions, conjugated IQ, fading/noise, rejection
of silence/carriers/keying/repeated characters, mixed RTTY/PSK, priority under
capacity pressure, ignore/only-pinned selection, gaps, retunes and 32-tab limits.
Common-window tests cover reference boundaries, hard-priority protection and
access for ordinary full-capture candidates when common windows are crowded.

For two PSK streams over 16 seconds of generated IQ, the final Release test
measured 0.029 CPU seconds at 96 kS/s, 0.185 at 1.024 MS/s and 0.675 at
4.096 MS/s. These timings exclude waveform generation, host UI and stream
copies; they are local measurements, not a total receiver CPU guarantee.
Eight crowded channels at 96 kS/s took 0.154 CPU seconds including generation.
PSK shares the existing raw queue, worker and wide FFT; RTTY retains 16 slots
and PSK has eight. Full-spectrum discovery runs at most eight times per second.

Isolated, paced Brown replay decoded RTTY, PSK31 and PSK63 concurrently with
zero dropped IQ packets. Native PSK tabs displayed the received reference text,
green activity, Copy to clipboard, Follow PSK and the AI provider menu. The
menu was dismissed without selecting a provider. Report tests verify PSK
carrier/baud metadata and preserve literal RY text. Synthetic replay establishes
controlled behavior; on-air sensitivity, error rate and long-soak stability
remain unmeasured. QPSK, PSK63F and extended character sets are unsupported.

The final common-window build also decoded all three streams in Brown replay;
one queue gap was reported during that run. Turning PSK off released its four
candidate slots while retaining both PSK tabs and continuing RTTY decoding.
The native checkbox and remembered setting agreed. The gap-free result above
belongs to the earlier paced run; it is not a claim that queue gaps never occur.

Brown now runs the local 1.0.43 candidate from its normal profile, with CW,
RTTY and PSK enabled and reception resumed at the previous 7.116 MHz LSB,
3300 Hz filter. HTTP remains off. The installed module matches the tested build.
All 33 pre-update CW log prefixes match their recorded hashes; saved frequency,
gain, source, decimation and signal choices are unchanged. Installer backups
contain the prior plugin and configuration. Both local host archives passed
allowlist, binary, manifest, dependency and private-path checks. Public release
remains 1.0.42; the PSK candidate has not been committed or published.

## 1.0.42 RTTY AI handoff, 2026-10-17

All nine core targets, five installer tests, nine ASan/UBSan targets and both
matching real-ImGui suites pass. Both host modules build. Handoff tests cover
compact and original RTTY text, reception metadata, optional receiver grid,
all three providers and long-prompt clipboard fallback. Tab frequency remains
distinct from the current capture, and RTTY prompts contain no CW speed claim.

Installed Brown 1.0.42 displays the RTTY Explain with AI button and its ChatGPT,
Claude and Gemini menu, including clipboard fallback instructions. The menu was
opened and dismissed without selecting a provider or sharing live reception.
The installed binary matches the tested build. Existing CW log prefixes remain
byte-for-byte intact; pre-update RTTY text was exported privately before restart.
This AI addition changes no decoder thresholds or DSP.

## Local 1.0.41 tuned priority and activity, 2026-10-17

All nine core targets, five installer tests, nine ASan/UBSan targets and both
matching real-ImGui suites pass. Both host modules build. A 20-station fixture
first fills all 16 decoders outside a new priority region. Moving priority admits
all three tuned pairs on the next discovery pass (checked within 0.3 seconds),
decodes their messages together and retains 13 other decoders. Retuning priority
admits the new pair without deleting prior tab text. Ignore and Only pinned
continue to override eligibility.

A separate fixture contains over 256 stronger steady carriers plus one weaker
200 Hz RTTY stream. The ordinary global peak budget excludes that stream; tuned
priority retains it and decodes the message. This tests a specific discovery
failure, not universal weak-signal sensitivity. Existing ambiguity checks also
run with tuned priority for repeated I/P patterns at 170 and 200 Hz. They still
produce no tabs. Framing, format trials and payload qualification are unchanged.

Pure passband tests cover USB/LSB reference direction, centered AM/CW/DSB/FM
filters, invalid widths, capture clipping and requiring both tones inside the
priority region. Worker updates are tested without input. Activity tests cover
the complete 60-second window, expiry and a returning burst restarting the
window; native ImGui tests retain tab colors and stable status-bar geometry.

During live inspection, the installed host's playback HTTP endpoint reported
false while the native GUI showed reception and RTTY processing time advanced.
The plugin's immutable RTTY status now reports the worker's actual receiving
state, avoiding reliance on that host endpoint. No host code or signature changed.
The observed 7045 kHz snapshot had no RTTY input drops or capacity limit, so
priority alone is not established as the cause of every missed/slow field decode.

Installed Brown 1.0.41 reports receiving normally, with priority bounds matching
the latest saved LSB dial and filter width. It was resumed after the update.
Saved host settings and existing CW log bytes were verified preserved; all
three pre-update RTTY tabs were exported privately before restart. The installed
binary and both host packages match their tested builds. No improved on-air
accuracy or universal acquisition-time claim is made.

## Local 1.0.40 exact 200 Hz shift, 2026-10-16

Exact 200 Hz is now part of automatic pair discovery and validated saved signal
choices. The earlier shift list failed to discover the new 200 Hz fixture.
All 40 baud/shift/polarity decoder combinations pass, as do full-scanner 200 Hz
checks in both polarities at 8 kS/s, 96 kS/s and 4.096 MS/s. Four simultaneous
signals at 96 kS/s and 1.024 MS/s include separate 170 and 200 Hz streams.
Selection validation retains 200 Hz pins/ignores and distinguishes 170 Hz.

Adding the preset initially broke the existing five-second 850 Hz capture-edge
fixture at 2.4 MS/s. A comparison using the old discovery list passed. A targeted
trace showed weak 200 Hz modulation sideband pairs occupying all 16 decoder
slots before the real 850 Hz pair appeared. A pair with over four times the
weaker-tone power can now displace the weakest unconfirmed, unpinned automatic
candidate. This restores the original fixture without changing its duration or
text requirement. Confirmed decoders and pins retain their existing protection;
the decoder budget and worker count are unchanged. Trace code stayed outside
the source tree and temporary probes were moved to Trash.

All nine core tests, five installer tests, nine ASan/UBSan tests and both
matching real-ImGui suites pass. Separate repeated I/P fixtures at 170 and
200 Hz produce no tabs in Release and sanitizer runs. Both host modules build.
These controlled fixtures do not establish an on-air error rate or guarantee
detection of weak, overlapping or nonstandard signals. CW DSP is unchanged.

Brown's installed 1.0.40 binary and both packaged host binaries match the tested
builds byte-for-byte. The normal profile's settings outside module registration
were unchanged, and previously saved CW log bytes were verified preserved.
The native operating guide visibly lists 200 Hz. No on-air 200 Hz reception
was established during this installation check.

## Local 1.0.39 decoder controls, 2026-10-16

The toolbar now starts with independent CW and RTTY checkboxes. CW maps to the
existing worker pause command; no decoder algorithms or stream lifecycle changed.
The duplicate Pause/Resume button was removed. All nine core tests, five installer
tests and both matching real-ImGui suites pass, and both host modules build.

In the installed Brown candidate, clicking CW off set the worker's paused state,
showed CW off, and left RTTY enabled. Clicking it on cleared the paused state.
The hover explanation and checkbox order were verified in the native window.
The receiver remained stopped throughout; this check does not claim new on-air
reception results. Existing CW log hashes were unchanged. The installed binary
and both host packages match their tested builds, with AGENTS.md excluded.

## Local 1.0.38 acquisition and clipboard fix, 2026-10-16

Both host modules build. All nine core tests, five installer tests, nine
ASan/UBSan tests and both matching real-ImGui suites pass. The final Brown
candidate is installed outside the signed host bundle. Existing CW file contents
were verified preserved; pre-update RTTY text was exported privately. Reception
was stopped before updating and remains stopped. No new on-air error-rate or
universal-detection claim is made.

The original live receiver had I/P-heavy tabs, including long I runs with rare
U/C/P/W characters. A deterministic repeated-symbol fixture reproduced the
false confirmation. Expanded native replay also exposed weaker interpretations
that turned those blocks into mixed letters/figures, bypassing a simple
single-character check. The final acquisition gate requires sustained character
variety and transitions, plus framing; the strongest framing trial must qualify.
Once acquired, the chosen baud/polarity remains fixed until normal reacquisition.
Confirmed text is never filtered or corrected.

The regression includes rare other letters, spaces, CR/LF, stereo signed-16 IQ,
four simultaneous streams and repeated discontinuities. It rejects the two
ambiguous streams while retaining both message streams. All 32 supported format
combinations are checked during acquisition, and pure RY tests acquire within
the four-second candidate lifetime. Legitimate repeated I/P text following
qualified reception remains intact. An isolated native Brown four-signal replay
also retained exactly the two message streams and no I/P-heavy tabs. This proves
the tested cases, not every possible false signal or the physical origin of the
live interference.

Copy to clipboard reports are checked for date/day/time zone, band, estimated
mark and pair-center frequencies, baud, shift, polarity, receiving state and
current capture/gap metadata. Both original and compact-RY views are covered;
the report labels which view was copied and the time of copying. Real-ImGui
clipboard tests use the actual report-copy method for both matching hosts.

## Local 1.0.37 UX follow-up, 2026-10-16

Both host modules build. Nine core tests and five installer tests pass; all nine
core tests also pass under ASan/UBSan. Both matching real-ImGui suites pass,
including activity dots on inactive tabs, both dot colors, fixed tab positions
under changing status text, and a footer touching the window edges. The existing
CW transcript, selection and modal cleanup regressions remain passing.

The Signals regression first reproduced live snapshots removing a row between
checkbox press and release. The UI now uses a held list; discovery changes and
frequency drift cannot change targets until explicit refresh. Tests cover refresh,
saved pins/ignores and in-place matching. Presentation tests cover the five-second
activity deadline and exact RY folding without editing the original input.

An isolated native Brown replay with no saved RTTY preference enabled monitoring
by default and decoded two simultaneous 50-baud streams (170/450 Hz, opposite
polarities). Native screenshots confirm the flush footer and stable signal picker.
The test profile and synthetic reception are separate from normal logs.

A deterministic 120-second deep-fade/noise fixture retained one baud/polarity
choice without replacing earlier displayed text; it still lost characters during
fades. No DSP thresholds or CW algorithms were changed for this UI fix. The live
receiver also reported intermittent input gaps. RY compaction improves reading,
not reception accuracy; it does not repair or invent missing text.

## Local 1.0.36 RTTY candidate, 2026-10-15

Both matching Apple Silicon host modules build. All eight native decoder/core
tests and five installer tests passed, as did the full eight-test ASan/UBSan
suite. The final RTTY-only sanitizer rerun includes selection under capacity
pressure and 425/450 Hz reacquisition without duplicate tabs or replacing earlier
confirmed text. Both real-ImGui suites passed transcript refresh/selection,
hook removal, scoped theme restoration and modal cleanup that leaves another
plugin's modal intact.

Isolated actual Brown and standard SDR++ replays decoded three simultaneous
RTTY streams outside Radio's 400 Hz CW passband while CW decoded
`CQ CQ DE TEST 123 K`. The controlled source used 96 kS/s complex IQ, a 20 WPM
center carrier, and RTTY pairs at -18 kHz (45.45 baud/170 Hz), +21 kHz
(50 baud/425 Hz, reversed) and +44 kHz (75 baud/850 Hz). CW's startup prefix
sometimes read `TVV`, consistent with the earlier baseline; no perfect startup
transcription is claimed. The portable 1.024 MS/s fixture verifies wider capture.

Brown's saved Only pinned configuration restored one decoder/tab while other
pairs remained discoverable. The native Tune Radio action selected USB/3.2 kHz
and the expected dial with mark at 2125 Hz, preserving CW text and the RF capture
center. A request whose full passband would cross the capture edge was rejected.
Native Close all removed every RTTY tab without clearing the paused CW
transcript. Settings, help and signal-picker modals rendered at 740 by 600 in
the isolated 1280 by 850 viewport. Existing normal profiles and logs were not
used as replay destinations.
Disabling the actual Brown module with its signal-picker modal open released
host input; re-enabling resumed the pinned decoder. That short resume check
reported zero input gaps. All owned replay writers were stopped and the test
fixture, profiles and CW logs were removed from their temporary location after
validation. Only aggregate checks and UI screenshots were retained privately.

Capture/queue gaps can occur under load; the monitor reports them and resets
affected acquisition. Resource bounds are 16 simultaneous candidate decoders,
32 tabs, 250 ms queued IQ and 4.096 MS/s maximum input. Tests demonstrate
controlled decoding and integration, not universal detection, on-air accuracy,
or a months-long soak. RTTY text remains memory-only. This candidate is local,
unpublished and not installed into the normal receiver profiles.

Detailed behavior, selection controls and performance measurements:
[RTTY monitor](rtty.md).

## Local 1.0.35 host compatibility checks, 2026-10-13

Brown keeps its native receiver commands, draw event and HTTP override. Standard SDR++ uses the radio interface and an EndFramePre hook; real-ImGui regression tests passed against both pinned host revisions, including drawing without a sidebar, continued transcript delivery and removing the hook before destruction. Installer tests cover wrong-host rejection without changes, separate profiles, retained grid/disabled preferences and rollback.

All seven portable C++ targets and five Python installer tests passed. Both host-specific real-ImGui suites passed; the standard hook/transcript suite also passed ASan/UBSan. The installed hosts themselves were not sanitizer-instrumented. Separate Apple Silicon builds use the documented matching headers and each app's bundled core and VOLK libraries.

Actual standard SDR++ 1.2.0 (May 28, 2024) replay decoded `CQ CQ DE TEST 123 K` in CW with a 400 Hz filter, USB/LSB/DSB at 12 kHz and keyed RF AM at 15 kHz. Native clipboard copying retained the transcript and reception metadata. SDR++Brown 1.2.1 (August 4, 2026) replay passed the same five modes and AM audio tones, with zero reported drops or errors. Brown's intentional Pause survived a mode change; unsupported NFM stopped capture and returning to AM resumed it. Estimated speed was 19.5 WPM for the 20 WPM fixture. Startup prefixes sometimes contained errors; these controlled replays do not establish on-air accuracy or perfect transcription.

Both matching binaries were installed outside the signed app bundles and verified byte-for-byte. Brown SHA-256: `8689703e8bc1cd335bc94b7243bea34b762930492237cedc92aece599b4696c1`. Standard SHA-256: `ac2ad695d1d0647874850706a528c1dccf08fbf93ccba207e2a1acaecaaeee1d`. Isolated test profiles were closed and original log preferences restored; the user's latest real RTL-SDR profiles were preserved. The two release packages include distinct host manifests and no recordings or private profiles.

Standard SDR++ restarted successfully with its normal RTL-SDR profile, CW Notebook visible, the previous 6.988 MHz LSB dial and 3281 Hz filter, receiver stopped and Ankara's coarse KM69 grid visible in Decoder. Brown's native location remains unchanged. Package inspection verified eight allowlisted files per ZIP, matching binary/manifest pairs and both archive checksums.

During standard-host IQ replay testing, tuning a 15 kHz AM filter to the edge of a 96 kHz capture crashed the host in WaterFall::calculateVFOSignalInfo. An isolated ASan reproducer compiled that exact upstream function without CW Notebook and reproduced a heap-buffer-overflow: its clamped upper FFT index can equal rawFFTSize, but the maximum-search loop includes that index. This is a host boundary bug; the plugin creates no visible waterfall VFO. No host binary or signature was modified. Keep Radio's whole filter within the captured spectrum on this tested older host. Controlled decoding checks use valid in-capture passbands.

### Brown OMLSA crash reports, 2026-10-16

Brown 1.2.1 crash reports from October 14 and October 16 both identify `noise_reduction_logmmse.dylib`, in `MY_B4_FFT::base4_fft` called through `LSA_denoise::Denoise_process`, on the `AFNR_OMLSA_MCRA` worker. The same noise-reducer binary UUID appears in both reports. The older report has no RTTY worker; the newer report has CW and RTTY workers waiting. This points to the OMLSA noise-reduction path, but does not establish the underlying memory error or exclude corruption elsewhere.

The October 16 report records a crash about 15 minutes after process launch. It does not establish the separately reported failed startup. A subsequent normal-profile launch was running with CW Notebook 1.0.38 and no plugin-reported error during inspection. No reproduction or plugin-disabled comparison was performed; the running receiver and its settings were left unchanged. If this signature recurs, compare OMLSA disabled/enabled in an isolated profile before changing the decoder. Do not replace or re-sign the host as a diagnostic shortcut.

## Local 1.0.34 check, 2026-10-13

All seven portable C++ targets, three installer tests, ASan/UBSan targets and the real-ImGui transcript regression passed. New channel tests cover CW, USB, LSB, DSB, AM keyed carriers and AM audio tones through ReceiveChannel, ChannelMixer and Decoder, in both decoding paths. They include signals near both passband edges, carrier-offset sign, silence rejection and unsupported-mode rejection. Standalone AM tests also cover 24 kHz for compatible hosts; the installed Brown build limits AM to 15 kHz.

Synthetic IQ replay in the actual Brown host decoded the complete body `CQ CQ DE TEST 123 K` in CW at 400 Hz width; USB/LSB/DSB at the full 12 kHz width; and both AM input choices at 15 kHz width. The first startup prefix sometimes read `TVV` or `AVV` instead of `VVV`; no perfect transcript is claimed. All modes reported zero dropped packets and no errors. USB/LSB carrier offsets retained their correct signs, and AM audio tones reported an unavailable RF carrier offset. Intentional Pause survived a mode change, unsupported NFM stopped capture, and returning to AM resumed it. These are controlled replays, not on-air reception evidence.

Browser handoff tests cover context, verbatim text, URL encoding, all three providers and complete clipboard fallback for long transcripts. Clicking the actual popup's ChatGPT item opened the default Chrome browser with the transcript, frequency, band, grid, local date/time, WPM and carrier-offset context filled into the composer. The frequency stayed unchanged. The user still presses Send. Claude prefilling could not be verified past its sign-in page; Gemini opens its site with the full prompt copied for pasting. No browser API or account automation is used by the plugin.

Installed Release binary matches the build byte-for-byte, SHA-256 `105200bdfa19caa9898aeaaf89d9aa5ddf41d61905ad63cbc5c57cab3a488f25`. Original log preferences were restored after the isolated test profile was closed. The normal RTL-SDR profile resumed at its previous 7.092 MHz LSB setting; the plugin followed the 3039 Hz Radio filter at 16 kHz input. Input and processing counts advanced, with zero drops or errors and one Brown instance. No on-air CW transcript was established at that voice-band setting.

## Public, reproducible checks

Version 1.0.32 tests distorted slow marks with two long elements that a faster fit previously discarded as outliers. The preceding build incorrectly acquires at 37.5 WPM; the revised fit must withhold that fast lock and acquire 20 WPM as more slow timing arrives. Both original and recovery timing paths are covered.

Version 1.0.31 adds 20 WPM audio mixed with quadrature band-limited noise around the wanted tone. The preceding build fails to calibrate on the first fixture. Three deterministic seeds must retain the complete test body at the correct speed in both decoding paths; matching noise-only fixtures must produce no letters. Existing extended-speed, fading and clean-audio checks remain required. This covers a specific noise-fragmentation failure, not arbitrary weak-signal reception.

Version 1.0.30 reproduces a 40 WPM automatic lock followed by sustained 20 WPM timing. The previous bounded tracker stays at the wrong speed in this controlled test. Guarded reacquisition must resume correct characters, track a sustained return to 40 WPM, ignore a brief different-speed fragment and preserve an explicit manual lock, in both decoding paths.

Version 1.0.29 adds amplitude-step audio tests at 8, 12, 20, 35 and 40 WPM. A roughly 19 dB reduction between characters stays above the noise floor but caused missing elements with the original fixed peak memory. The recovery path must retain the full test body, including a separate varied-letter/digit message, while matching the original on clean audio and producing no additional text from noise or carrier loss. This models one class of fading, not arbitrary ionospheric reception.

Version 1.0.27 checks provisional output by 2.4 seconds of ambiguous keying, replacement by confirmed replay without duplicates, and clearing the draft on reset. Provisional letters are excluded from the confirmed-output callback and autosaved log.

Version 1.0.26 adds first-character acquisition checks at 5, 8, 12, 18, 25 and 40 WPM in both timing paths: a clean D must appear by its closing character gap. A 2300-character ambiguous prefix exercises temporary-file spill and exact replay without loss or duplication. Audio tests keep ambiguous data across 15 seconds of silence. Manual speed, extended-range replay, default rejection of 80 WPM, persisted range choices and one-Hz retunes clearing both learning buffers are covered. The existing startup-pulse rejection and noisy/fading audio checks remain.

The transcript widget test also checks that metadata copying includes text received while the transcript tab is hidden. Synthetic timing checks establish controlled latency and data retention, not a universal on-air error rate.

Run `sh scripts/check.sh` for the decoder/worker tests and Python installer tests. These checks need no SDR hardware, Brown installation or private recording.

Coverage includes ideal timing and noisy/fading synthetic audio at 3, 5, 12, 20, 40, 80, 120 and 200 WPM; sample-rate and tone variations; noise-only input; log rename/collision/path/symlink protections; clear/archive/restart behavior; manual-speed persistence; two minutes of silence; silent retuning/rate resets; and recovery of normal text afterward.

Installer tests use temporary profiles and dummy files. They check initial install, update backups, duplicate prevention, preservation of unrelated settings, invalid-config refusal and rollback after a failed config replacement. They do not replace the real plugin.

The standalone suite is also run with AddressSanitizer and UndefinedBehaviorSanitizer. The live host itself is not sanitizer-instrumented.

Recovery tests inject brief key dropouts, false pulses, compressed character gaps and stretched element gaps at the timing layer. Clean randomized Morse is compared against the original at 5-200 WPM. Worker tests check retained originals, separate views, persisted choices, retuning with exactly one newline, automatic lock release, optional lock retention, and intentional Pause. A fake clock exercises stream watchdog grace, retry, backoff and stop/Pause behavior. These are deterministic tests, not evidence of a measured field error rate.

To test the transcript against matching host ImGui sources:

```sh
cmake -S . -B build-ui -DBUILD_PLUGIN=OFF -DCW_IMGUI_SOURCE=/path/to/Brown/core/src/imgui
cmake --build build-ui --target cw_transcript_tests --parallel 4
ctest --test-dir build-ui -R cw_transcript_tests --output-on-failure
```

The headless test drives the actual TranscriptView and ImGui multiline widget. It reproduces the pre-1.0.22 freeze after a plain click, then verifies incoming text while selected, scrolled or Follow-disabled, active widget buffer updates, returning to automatic scrolling, and clear/replacement behavior. Only Brown's unrelated widget-registry hook is stubbed. This target is optional for portable builds without host sources, and required when modifying the transcript UI.

## Optional local recordings

Private reception fixtures are not distributed. Maintainers can configure `CW_RECORDING_FIXTURE` with a local 8 kHz float32 recording matching the optional checks in `tests/tests.cpp`. The companion `4xz-fading-8k.f32` must be beside it. These checks are skipped explicitly when not configured; synthetic coverage still runs.

Manual-speed operation was checked on a faded live CW recording; automatic speed was less stable. No perfect live transcript or universal real-world speed range is claimed.

## Host checks

Development validation used an Apple Silicon Mac, macOS 26, RTL-SDR Blog V4, and SDR++Brown 1.2.1 built August 4, 2026.

Verified during development:

- Live decoding, receiver stop/start, plugin disable/enable and app restart.
- File rename, continued append, clear with recovery archive, and restoring the last log.
- Text selection, Cmd+C and context-menu copying.
- No waterfall retuning from physical mouse gestures started inside the plugin.
- Clipboard summary containing date/time, weekday, frequency, speed, carrier offset and decoded text.
- Single-click Clear log empties the transcript and archives the previous file.
- Silent stream changes no longer fill the box with blank pages.

Multiple-display automation coordinates were occasionally unreliable; physical mouse checks were used for gesture isolation. These are bounded checks, not an all-day stability guarantee.

For each release, record the actual host smoke test and artifact checksum in its release notes. A deployment target is not evidence that every older OS version has been tested.

## Local 1.0.32 check, 2026-10-10

Private captures at 22.193 MHz reproduced false 37.5 and 42 WPM startup locks. Instrumented timing showed the fit discarding two long marks to fit the shorter majority at twice the apparent station speed. The new repeated-long-mark veto removes both failures. Across 28 starting points from seven captures, all acquired timing remained between 15 and 30 WPM. This check examines intermediate estimates, not just the final speed. Recordings and diagnostic traces remain private.

The preceding 1.0.31 noise-filter update improved replay acquisition but still produced one false fast lock in three live relearns. It was therefore followed by 1.0.32, rather than treated as a complete fix. Moving the source centre 100 kHz below the wanted station did not by itself eliminate the false locks; centre interference was not established as their root cause.

All five core test targets, three installer tests and ASan/UBSan targets passed. The unchanged real-ImGui transcript regression also passed. Installed Release binary SHA-256: `ae6b067e96dfc1a0141b48ca066ef748515d1ae31498907734765f763dcf1ff7`.

Brown loaded 1.0.32 at 22.193 MHz, CW, 287 Hz Radio filter, automatic speed and tone, with no forced WPM. Three live startup/relearn trials acquired about 20-22 WPM; none of their calibrated samples exceeded 22 WPM. Acquisition appeared after approximately 7, 3 and 2 seconds of polling, respectively. All trials reported zero dropped packets and no errors. Live text included the expected station identifier and repeated call sequence, but fading still caused missing, extra and incorrect characters. These are bounded startup checks, not a guarantee of perfect decoding or acquisition within three seconds on every signal.

## Local 1.0.27 check, 2026-10-09

Core, recovery, acquisition, installer and ASan/UBSan checks passed. The real-ImGui transcript regression passed after the UI changes. The installed binary matched the Release build, SHA-256 `6bcbadc64eebdfe045a9684a8a8c76b6deed2282a9e0a484d1670bbfc8cd3b13`.

At 7.006190 MHz, live status polling observed provisional letters roughly two seconds after the first detected intervals. Timing subsequently calibrated around 22-24 WPM, and the actual Transcript window visibly continued receiving confirmed letters. The plugin followed Radio's 312 Hz CW filter, with automatic 5-45 WPM enabled, no manual locks, and zero reported drops or errors. A separate private recording first produced provisional text about 2.03 seconds after detected keying.

The live transcript still contained decoding errors. These checks establish startup responsiveness, continued output and UI operation, not accurate transcription of every character. The provisional view is explicitly tentative and may change.

## Local 1.0.28 check, 2026-10-09

A 20-second private recording at 7856.970 kHz reproduced persistent learning with no confirmed output in 1.0.27. Manual 20 WPM recovered recognizable fragments from the same audio. Instrumentation showed short fading/noise pulses pulling the dot cluster down, leaving too few inliers across the old 64-mark window. The new recent-window fit starts confirmed output at 3.16 seconds in that recording and recovers `VVV DE 4XZ` without any station-specific decoder logic.

A synthetic regression first failed on interference followed by distorted marks, then passed with the fix. Core, recovery, acquisition, installer and ASan/UBSan checks passed. The installed binary matched the Release build, SHA-256 `448c1e39fa9c69700dab5abbeaff47fc76d4af9c89df7d76cd865ef075544440`.

Three live relearn trials at the same frequency produced confirmed text at 3.54, 3.03 and 3.03 seconds after reset, approximately 1.5-2.5 seconds after the first observed key intervals. All three calibrated, with no reported errors. Brown's visible Transcript showed Decoding, an estimated 21 WPM, no provisional section, and `VVV DE 4XZ 4XZ =`. Other repetitions still contained errors during fading; this verifies acquisition recovery, not perfect transcription. Radio remained in CW at 7856.970 kHz with its 400 Hz filter and automatic speed/tone.

## Local 1.0.21 check, 2026-10-09

Core/recovery tests, three installer tests, and AddressSanitizer/UndefinedBehaviorSanitizer checks passed. The installed Apple Silicon binary matched the Release build byte-for-byte. Five live retunes between 7.021960 and 14.021960 MHz kept sample counts advancing, with zero dropped packets or reported errors; manual private-channel reconnect resumed reception. The original/recovery view switch and expanded controls were inspected in Brown. The receiver was left at 14.021960 MHz, CW, with center tuning enabled and Signal Snap Auto disabled.

No complete live CW transcript was obtained during this check. The new timing-recovery accuracy evidence is synthetic; the live checks establish stream continuity and UI operation, not a field error-rate improvement. Existing input-isolation code was retained; prior physical mouse checks remain the evidence for drag isolation.

## Local 1.0.22 check, 2026-10-09

The real-ImGui test first failed with `Clicking transcript froze incoming text`, then passed after separating content updates from automatic scrolling. Active selection, the widget's internal text length, Follow-disabled updates and resuming Follow were checked. Core/recovery, installer and ASan/UBSan checks also passed. Installed binary matched the Release build; Brown loaded 1.0.22 with original view and Follow enabled, receiver samples advancing and no errors or drops. Vocabulary controls and status fields were removed. The pre-restart receive frequency, 14.013980 MHz, was restored. Live reception did not provide a new transcript during the bounded smoke check; the deterministic widget test is the evidence for the refresh fix.

## Slow acquisition regression

Both original and recovery timing paths are tested at 5, 8, 12 and 18 WPM after six short startup pulses. The old algorithm failed by declaring a fast lock before the slow message. Tests require no output from those initial pulses, then the complete buffered slow prefix and subsequent message at the correct speed. Audio coverage includes 8 and 18 WPM alongside the existing 3-200 WPM cases. An engine test retunes from 20 WPM by just 1 Hz, requires uncalibrated zero WPM, and then decodes an 8 WPM station without carrying the previous estimate forward. These synthetic regressions do not establish accuracy for every noisy on-air signal.

## Local 1.0.24 check, 2026-10-09

Release preparation follow-up: macOS CI exposed a test assumption that a received phrase always fits in one autosave record. An explicit mid-message save reproduced the failure locally. Version 1.0.25 checks the original records joined in order, excluding recovery records, and keeps the same decoder behavior. Local core, recovery, installer and ASan/UBSan checks passed with the forced split.

A short clean CQ followed by silence reproduced missing output in 1.0.23. Starting candidate agreement earlier fixes this regression at 5, 8, 12, 18, 25 and 40 WPM in both timing paths while retaining the startup-pulse rejection tests. Core/recovery, three installer tests, ASan/UBSan and the real-ImGui test passed. The ImGui test also checks that reopening an off-screen window after resizing to 640 x 480 places it within view, focuses it and raises it above another panel.

The installed binary matched the Release build (SHA-256 `2ebb9fdea25144c320a2a27e3962901deb758278389800a23bc9c433a9a2b0d5`). Brown loaded version 1.0.24; disabling and re-enabling the plugin visibly restored its centered window. Reception resumed at the pre-update 14.031070 MHz with live text and no reported drops or errors. This verifies operation, not the accuracy of every received character. A separate 20-second fading recording produced no text in either 1.0.23 or 1.0.24; the startup improvement is demonstrated by the controlled short-CQ regression, not that recording.
