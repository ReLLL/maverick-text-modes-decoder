# Changelog

## 1.0.49 - 2026-10-19

Public release incorporating the local 1.0.43-1.0.48 builds below.

- Withhold an RTTY callsign flag/tag when a one-off token adds one leading
  letter to a recognized callsign repeated elsewhere in the same transcript.
  Keep every received character. Repeated longer calls and portable calls
  retain their annotations; other modes and decoder acquisition are unchanged.
- Clear stale text-selection indices if later reception removes an ambiguous
  earlier flag. Ordinary appends preserve selection.

## 1.0.48 - unreleased

- Retain RX SNR from the latest detected signal activity. An old RTTY/PSK
  decoder lock no longer refreshes the reading after incoming activity stops.
- Manual CW tone selection no longer qualifies noise as signal SNR. Require
  detected keying and spectral signal evidence, preserving the decoding path.
- Keep the last signal reading through noise and refresh it when reception
  returns, including weaker signals. JS8/Olivia retain decoded-frame SNR.

## 1.0.47 - unreleased

- Replace CW Settings with a paged Settings window: Text & callsigns,
  CW decoding, Monitoring and Diagnostics. Preferences apply immediately.
- Add saved callsign/country detection and HAM QSO highlighting options for
  all receive modes. Show inferred flags inline and recent callsign tags;
  clicking a tag opens its QRZ.com page in the default browser.
- Keep received text intact in full reports and logs. Selected annotations
  copy as Unicode flags. Preserve text selection, Follow and scrolling using
  the host's existing transcript widget; no extra DSP threads or scans.

## 1.0.46 - unreleased

- Retain the last valid local RX SNR through inactive periods, with a "last"
  label and a red/yellow/green signal scale. Clear CW metadata on retuning.
- Give each mode its own tab accent. Show the last reception age for under
  120 seconds, then its saved local date/time, including in copied reports.
- Add an offline JS8 header callsign country estimate and cached native flag.
  Keep inferred country separate from the received grid and remote report.
- Suppress interleaved duplicate JS8 frames without refreshing receipt times.
- Initialize unused JS8 spectral columns before synchronization searches;
  uninitialized edge power caused intermittent Ultra fixture misses.

## 1.0.45 - unreleased

- Closing the CW tab turns CW off and releases its private receive channel.
  Turning CW on reopens and selects the tab, with fresh timing acquisition.
  Keep confirmed text and logs; digital monitoring continues independently.
- Keep CW tab state consistent during quick toggles and delayed worker updates.

## 1.0.44 - unreleased

- Add native JS8Call reception at all five speeds and seven common Olivia
  formats, sharing full-capture discovery and frequency tabs with RTTY/PSK.
- Keep FEC in one bounded background worker, with a shared JS8 core, four JS8
  windows and two Olivia candidates. Preserve the existing CW/RTTY/PSK budgets.
- Show local RX SNR for CW and every digital tab, with a 2500 Hz reference.
  Include received JS8 header callsigns, grids and remote reports when present.
- Add metadata to clipboard reports and AI handoff. Preserve text and CW logs.

## 1.0.43 - unreleased

- Add receive-only BPSK31 and BPSK63 across the captured spectrum, sharing the
  existing raw subscription, packet queue, wide FFT and worker.
- Add independent PSK31/63 control and frequency tabs with tuned-filter priority,
  pins/ignores, Follow PSK, one-minute activity, clipboard reports and AI handoff.
- Prefer common PSK frequency windows after tuned/pinned signals, while keeping
  full-capture discovery and an ordinary candidate probation slot.
- Retain 16 RTTY slots and add eight bounded PSK slots; share 32 digital tabs.
  Close all tabs retains CW. CW decoding and logging remain independent.
- Require phase, timing, Varicode and varied-text evidence before PSK tabs open;
  expose capacity/gaps and preserve text through reacquisition.

## 1.0.42 - 2026-10-17

First public RTTY release, incorporating the local 1.0.36-1.0.41 builds below.

- Add Explain with AI to each RTTY tab, matching CW's ChatGPT/Claude/Gemini
  menu. Share the displayed compact/original text with RTTY frequency, band,
  baud, shift, polarity, time and receiver context. Copy the full prompt for
  every provider; use clipboard fallback for long transcripts and Gemini.
- Decode simultaneous RTTY streams across the captured spectrum in frequency
  tabs. Prioritize all eligible streams in Radio's selected passband, then
  manual pins and other candidates, within the existing 16-decoder budget.
- Include 170/200/425/450/850 Hz shifts, 45.45/50/75/100 baud and both polarities.
  Keep green tab activity for one minute; returning activity restarts the timer.
- Add independent CW/RTTY controls, stable signal selection, pins/ignores,
  metadata copying, optional compact RY display, modal help/settings and a
  fixed status bar. Keep CW decoding and Markdown logging unchanged.
- Keep project instructions local through .gitignore; remove AGENTS.md from
  the published tree. Existing Git history is retained.

## 1.0.41 (unreleased local candidate)

- Automatically prioritize all eligible RTTY pairs inside Radio's current
  passband, following frequency, mode and filter width. Keep scanning the
  entire capture with the same 16-decoder budget. Saved pins come next;
  explicit ignores and Only pinned remain effective.
- Reserve part of discovery's existing peak budget for the tuned passband,
  estimate its noise locally, and retry its candidates without cooldown gaps.
  Existing framing and repeated-letter rejection checks are unchanged.
- Keep each RTTY tab's circle green for one minute after confirmed activity.
  Returning activity restarts the full minute; inactive tabs age to yellow.

## 1.0.40 (unreleased local candidate)

- Add exact 200 Hz RTTY shift detection alongside 170/425/450/850 Hz, including
  remembered pins and ignores. Keep 170 Hz and 200 Hz selections distinct.
- Extend format, full-spectrum and simultaneous-signal regression coverage.
- Let a substantially stronger signal replace an unconfirmed automatic
  candidate when the decoder budget is full. Keep confirmed streams and pins.

## 1.0.39 (unreleased local candidate)

- Add independent CW and RTTY checkboxes at the start of the notebook toolbar.
  CW uses the existing pause/resume control and retains text and logs. Replace
  the duplicate Pause/Resume button with a visible CW off state.

## 1.0.38 (unreleased local candidate)

- Require varied payload evidence or a sustained alternating RY test before
  confirming a new RTTY stream. Periodic I/P-heavy patterns with occasional
  other letters no longer qualify on framing alone.
- Hold the chosen baud/polarity trial after acquisition; reacquire through the
  existing gap/stall lifecycle instead of replacing text with another trial.
- Replace Copy raw RTTY text with Copy to clipboard. Reports include the current
  displayed view, date/day/time zone, band range, estimated mark/pair frequencies,
  baud, shift, polarity, reception state and capture/gap metadata.

## 1.0.37 (unreleased local candidate)

- Hold Signals rows and checkbox targets steady until Refresh list, while
  continuing to update signal states. Retain saved ignored frequencies too.
- Add filled green/yellow circles before RTTY frequencies: green for five
  seconds after new decoded text, yellow otherwise, including inactive tabs.
- Move changing notices into a fixed status bar flush with the window edges.
  Add explanatory hover help to notebook controls.
- Fold exact long RY test sequences in an optional Compact RY display. Preserve
  the original reception and provide Copy raw RTTY text.
- Default Monitor RTTY on for profiles without a saved choice; honor saved off.

## 1.0.36 (unreleased local candidate)

- Add an optional full-capture RTTY monitor with shared FFT channel extraction,
  bounded simultaneous decoding and conservative automatic framing checks.
- Keep CW in a permanent tab; give RTTY streams frequency tabs, unread markers,
  independent Follow/copy controls and one-click Close all tabs.
- Preserve CW decoding and Markdown logging. RTTY text is currently memory-only.
- Add deterministic multi-station, format, noise, capacity, gap and close tests.
- Add remembered signal priorities, Only pinned decoding, ignored frequencies,
  explicit safe Radio listening, and a compact Signals picker for crowded bands.
- Refresh the notebook with a scoped graphite/brass theme and modal CW Settings
  and How to use windows; preserve the host's own theme and transcript behavior.

## 1.0.35 - 2026-10-13

- Add a separate standard SDR++ macOS build, retaining Brown as the primary native target. Keep all Brown receiver controls, UI event integration and HTTP automation.
- Standard SDR++ uses its radio interface and a frame hook, preserving full-filter CW/USB/LSB/DSB/AM decoding, recovery, logs, clipboard and AI controls even when the sidebar is hidden.
- Separate each host's logs/preferences; add an optional standard-host grid for AI context. Brown continues using its native grid.
- Add host manifests, installer checks against wrong-host packages/profiles, and preservation of disabled-plugin and instance settings. Package each host separately.
- Test transcript updates and hook removal against both hosts' ImGui sources; extend installer regression coverage.
- Document the tested older SDR++ host's spectrum bounds bug: keep Radio's full filter inside the captured RF spectrum to avoid its edge-triggered crash. This was reproduced without CW Notebook.

## 1.0.34 - 2026-10-13

- Add AM with the full selected RF bandwidth, including the tested host's 15 kHz maximum. Choose keyed RF carriers or envelope-demodulated Morse audio tones. Audio tones are not mislabeled as RF carrier offsets.
- Add Explain with AI: ChatGPT, Claude and Gemini in the default browser, with the displayed transcript and reception context. Copy the complete prompt for every provider; use clipboard fallback for Gemini and long transcripts. No API integration or background upload.
- Preserve originals and explicitly label provisional/recovery text. Analysis instructions distinguish literal reception from possible corrections and uncertain station identification.

## 1.0.33 - 2026-10-13

- Decode keyed CW carriers inside USB, LSB and DSB passbands. Replace the old centered 500 Hz sideband capture with Radio's actual filter position and full width, including its 12 kHz maximum.
- Preserve CW-mode processing. Extend sideband tone acquisition without aliasing and show carrier offsets relative to the dial with the correct sign.
- Restart acquisition on modulation changes while preserving confirmed text and intentional Pause. Update the operating guide and add synthetic IQ-to-text tests for both sidebands, DSB and CW.

## 1.0.32 - 2026-10-10

- Reject a speed candidate when at least two recent marks exceed 4.3 of its proposed dot units. The fit can no longer discard repeated slow dashes as outliers to manufacture a faster lock.
- Add a regression for distorted slow marks that previously calibrated at 37.5 WPM, then verify recovery at 20 WPM. Preserve the recent-window buffer and manual-speed behavior.

## 1.0.31 - 2026-10-10

- Reduce false fast-speed estimates caused by noisy, fragmented marks. Ordinary-speed acquisition uses a 90 Hz tone-envelope filter and 6 ms edge debounce; the extended-speed path retains its previous acquisition settings.
- Lower the ordinary-speed instantaneous noise gate from five to three times the measured noise envelope, retaining tone lock, hysteresis and peak checks. Weak dashes are less likely to break into short pulses.
- Add reproducible band-limited noise tests with varied Morse, three random seeds, both decoding paths and noise-only rejection. No expected station text or vocabulary is used.

## 1.0.30 - 2026-10-10

- Recover from an incorrect automatic speed lock when sustained new timing contradicts it. Require 16 recent marks, strong dot/dash and gap support, and four agreeing updates from distinct marks before changing speed at a character boundary.
- Preserve manual speed locks, existing transcript text and the ordinary speed range. Short interference cannot trigger this correction; no vocabulary or expected-message matching is used.
- Add regressions for slower/faster transitions, brief interference and manual locks. Compare 24 starting points from three private reception samples against the preceding build.

## 1.0.29

- Add speed-aware fade tracking to the optional recovery version: an earlier loud character no longer holds its peak threshold for a fixed 333 ms.
- Preserve the original detector and noise-rejection gates. Label the alternative Timing + fade recovery in the transcript, clipboard and log.
- Add audio regressions for abrupt attenuation at 8-40 WPM, varied letters/digits, clean-signal preservation and noise-only input.

## 1.0.28

- Fix persistent provisional output after interference: learn from the latest 16 marks and recent gaps while retaining the complete buffer for replay.
- Fit distorted dot/dash durations with bounded outlier influence; retain supported timing when new evidence is poor.
- Add a regression for startup interference followed by distorted Morse, alongside real reception replay checks.

## 1.0.27 - 2026-10-09

- Show labeled provisional letters after two seconds of detected keying, enabled by default. Reinterpret the draft as timing evidence changes; replace it with confirmed replay without duplicates.
- Keep tentative text out of autosaved logs. Clipboard copies include its provisional label. Retuning clears both the draft and held intervals.

## 1.0.26 - 2026-10-09

- Learn from a clean mixed character and flush at its closing gap. Retain measured intervals during learning, including long pauses; spill long buffers to temporary storage instead of truncating them.
- Manual WPM decodes held data. Default automatic range is 5-45 WPM; the optional extended range and manual settings support 3-200 WPM. Reject sudden speed-class jumps.
- Follow Radio's CW filter bandwidth. Keep direct IQ decoding independent of audio AGC.
- Consolidate controls into Transcript, Decoder and How to use tabs with one log toolbar. Copy includes text received while other tabs are open.

## 1.0.25 - 2026-10-09

- Make persistence tests verify original text across autosave record boundaries. Explicitly save mid-message to exercise the case that failed on macOS CI. Decoder behavior is unchanged from 1.0.24.

## 1.0.24 - 2026-10-09

- Begin checking speed agreement earlier, keeping the eight-mark minimum and dot/dash/gap validation. A clean short CQ no longer waits for a later transmission before showing text.
- Reopening or re-enabling CW Notebook centers its window in the visible host area and brings it to the front. Add a sidebar Bring transcript to front button without interrupting reception.
- Test short CQ acquisition at 5-40 WPM and window recovery after shrinking the host viewport, alongside existing slow-CW, noise and transcript-selection regressions.

## 1.0.23 - 2026-10-09

- Restart timing acquisition from zero even on a 1 Hz retune. Show learning rather than an assumed speed until calibrated.
- Buffer startup key durations and require supported dot/dash timing before emitting letters. Remove the early single-group fallback that could lock onto interference at a falsely high speed.
- Skip implausible leading startup marks when replaying the buffered first characters.
- Add 5, 8, 12 and 18 WPM startup tests with initial false pulses, slow audio checks, and a 20-to-8 WPM retune regression.

## 1.0.22 - 2026-10-09

- Fix incoming text freezing after clicking or scrolling in the transcript. Follow text now controls automatic scrolling only; both views keep updating without toggling the recovery checkbox.
- Plain clicks retain automatic scrolling. Selection and scroll position remain usable while new text arrives.
- Remove ham vocabulary suggestions, their controls, decoder stage and future log output. Preserve existing logs.
- Add a regression test using the host's real ImGui multiline widget, including active selection and updates while Follow text is off.

## 1.0.21 - 2026-10-09

- Preserve original decoding alongside a separately selectable timing-recovery transcript.
- Optional, default-on key-glitch filtering, adaptive character spacing, quiet-period relearning, and same-element ham vocabulary hints. Suggestions never replace the original text.
- Retuning preserves text and continues on one new line when the next character arrives. Release old station speed/tone locks by default, with a checkbox to retain them.
- Discard stale queued samples after retuning. Retry stalled private channels with bounded backoff while respecting Pause and receiver stop.
- Save both interpretations and clearly labeled hints to Markdown. Clear/archive clears both views and hints.
- Add a manual reconnect control and private 20-second diagnostic capture. Capture write failures do not stop decoding.
- Regression coverage for damaged timing, clean random Morse, retained originals, retuning, preferences, Pause, and watchdog behavior.

## 1.0.19  -  2026-10-09

First public release.

- Live CW decoding with automatic timing and optional manual speed/carrier controls.
- Selectable transcript, copy context menu and formatted clipboard reception summary.
- Recoverable Markdown logs, filename controls and one-click archive/clear.
- Silent retuning no longer accumulates blank lines.
- Full plugin mouse gestures stay isolated from waterfall tuning.
- Public installation/update instructions, architecture notes, contributor guide and upstream credits.
- Installer supports release binaries and custom profile folders, validates config before replacement, and restores the previous plugin if config replacement fails.
- Portable core tests and installer regression checks; private recordings excluded.

Earlier versions were local development builds.
