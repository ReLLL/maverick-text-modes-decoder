---
read_when: Changing decoding, host integration, threading, or log persistence
---

# Architecture

## PSK extension

The local 1.0.43 candidate adds PSK downstream of the existing digital wide FFT.
One raw subscription, fixed packet pool and worker serve RTTY and PSK. RTTY's
DSP and 16 channel slots are unchanged; eight PSK slots add small inverse
transforms and narrowband decoding. Both share 32 tabs. Mode requests cross the
worker command boundary; host lifecycle stays on the GUI thread. PSK has separate
saved enablement, mode-tagged reports and high-bit station IDs. Selection targets
with shift zero identify PSK separately from RTTY. See [PSK](psk.md).

PSK discovery keeps scanning the full captured spectrum at up to eight updates
per second. Allocation ranks tuned, pinned, common PSK windows, then other
candidates. Common windows provide soft preference: they cannot interrupt a
confirmed channel and leave one probation slot available to ordinary candidates.
Tuned/pinned candidates can displace lower priorities. All automatic candidates,
including common windows, use the same retry cooldown.

## Host integration

Brown is the primary compile-time target (`CW_HOST=brown`). Its existing onWaterfallDrawn event, native get_demod/set_demod commands, operatorLocation and HTTP command override remain unchanged. The standard target (`CW_HOST=sdrpp`) uses ModuleComManager's radio interface for mode reading and Use CW mode. Its EndFramePre ImGui hook updates the notebook even when the sidebar is hidden, and is removed before module destruction. Each binary links only to its selected host's core and bundled VOLK, using matching headers and ImGui.

Both targets share the decoder, IQ capture, UI, recovery, file worker, watchdog and browser handoff. Their log/preference folders are separate. Standard SDR++ lacks Brown's HTTP server and native location field; an optional grid is saved within the notebook's module instance. Upstream removes unknown top-level configuration keys, so plugin settings never add them. Package manifests and installer checks reject the other host's binary/profile before mutations; updates preserve instance settings and intentional disablement.

The plugin creates a private IQFrontEnd channel following Radio's full passband. CW retains its 8 kHz input, centered filter and 800 Hz mono representation. Other supported modes use 16 kHz input for widths up to 6 kHz, 32 kHz up to 12 kHz and 64 kHz for wider AM filters, covering the tested Brown build's full 12 kHz sideband and 15 kHz AM ranges. USB selects above the dial, LSB below it and DSB/AM around it. LSB conjugates the complex signal; keyed-carrier paths shift the complete passband above 200 Hz before converting to real samples, preventing opposite frequencies from folding onto each other. AM audio-tone input instead extracts the magnitude envelope and removes carrier DC with a 50 Hz high-pass. Its audio pitch does not identify an RF carrier offset. This uses the host's bundled VOLK library; no extra waterfall cursor is created. One worker performs decoding, logging and file commands. The GUI takes a snapshot; disk operations never run in the GUI or stream callback.

An initial audio-stream implementation failed installed-host testing because Brown's CW AGC raised gaps and distorted the amplitude keying. The direct IQ path avoids that dependency and keeps speaker settings untouched.

The stream-drain thread is stopped before removing the private channel. Radio stream registration/deletion events reconnect or detach the decoder. Destruction unhooks UI and stream lifecycle events, stops the drain, and joins the worker. A playback/mode gate enables decoding with CW, USB, LSB, DSB or AM selected. Mode and filter-layout changes rebuild only the private channel, discard queued old-format samples and reset timing. Mode changes separate subsequent text and release manual locks by default; confirmed text and intentional Pause remain. A short mutex protects the callback's channel-layout snapshot.

Packets carry sample rate, tuned RF frequency, and a discontinuity generation. Retuning, sample-rate changes, and overflow reset the detector. Unsaved segments preserve their own frequency and speed, including when autosave is disabled. File errors retain pending segments. A short queue lock contains no DSP, disk operations, or UI calls.

The CW decoder downsamples to 4 kHz after two low-pass stages and retains its 1024-point Hann FFT. Other input stays at 16/32/64 kHz and uses a 4096/8192/16384-point FFT, preserving the same frequency resolution, window duration and update cadence while covering the whole selected passband. Both use global/local spectral contrast and three agreeing estimates before tone lock. Quadrature filtering creates a 1 ms envelope in either path. A rolling noise estimate, fading-aware peak, hysteresis, and speed-dependent debounce produce key transitions. Two duration clusters estimate dot/dash timing; spaces follow learned timing. Unknown patterns are displayed literally as `[?]`.

Browser analysis is an explicit GUI action. A portable prompt builder formats the current snapshot and configured grid, preserves the displayed text and labels uncertainty. URL encoding is byte-safe, and oversized URLs fall back to the complete clipboard prompt. The GUI launches the default browser through argv-based `posix_spawn`, never a shell, API client or account automation. AI popup gestures share the existing waterfall input guard.

RTTY tabs share that same browser launcher. Their prompt builder uses the tab's
displayed compact/original view and reception report, with RTTY baud/shift/polarity
and estimated mark frequency rather than CW speed or Radio's unrelated dial.
The provider menu participates in the notebook popup/input guard. Prompt creation
and browser launch happen only after an explicit provider choice.

The detector is deterministic. No language model or word dictionary fills in missing characters. Signal status and WPM are estimates, not probabilities. A continuous carrier cannot produce valid text.

Ordinary 5-45 WPM acquisition uses a 90 Hz tone-envelope low-pass cutoff and 6 ms edge debounce. This is an internal noise filter after tone selection, separate from Radio's selected channel width. Extended-speed acquisition retains 200 Hz and 2 ms. The ordinary amplitude gate requires three times the estimated noise envelope and a peak above four times noise; the extended path and manual speeds outside 5-45 retain the earlier five/six-times gates. Tone lock, hysteresis and the absolute peak floor remain required. The previous ordinary gate fragmented weak but audible dashes, leading to false fast timing or indefinite learning.

Automatic acquisition starts uncalibrated after frequency changes greater than 0.5 Hz. Default estimates must fall within 5-45 WPM; an explicit extended preference permits 3-200 WPM. Out-of-range estimates are rejected, never clamped. Speed fitting uses only the latest 16 marks and recent gaps, independently of the full replay buffer. A bounded-loss search fits the Morse 1:3 duration model with at least eight marks, three dots, three dashes, 75% mark support and compatible inner gaps. This avoids short interference pulses defining the dot cluster. A clean mixed three-mark character can still establish timing earlier through the stricter cluster path. Acquisition waits for a live gap exceeding 2.2 candidate units, so output does not require another key-down transition. Single-class marks remain ambiguous. Established timing rejects candidate jumps beyond 0.7-1.4 times its unit and retains its estimate when recent evidence lacks support.

Measured key/gap durations are replayed in order using learned timing. RunBuffer keeps 4096 intervals in memory, then spills to an anonymous temporary file on the worker. Old intervals are not silently truncated. Storage failures surface through the worker error state. Replay skips leading marks outside the plausible range as startup interference. Uncalibrated quiet periods keep the buffer. Manual WPM interprets held intervals without restarting tone detection. Retuning, tone changes, explicit resets and stream discontinuities clear uncommitted runs. Temporary buffering is not crash recovery or an RF recorder.

Until calibrated, WPM is zero and the UI reports learning plus a buffered interval count. Confirmed originals stay append-only; reinterpretation is confined to held intervals. Manual WPM overrides the automatic range; default retuning releases it. The extended preference is appended to the settings file, with false used for older files.

After two seconds of detected keying, Timing also produces a separate provisional string. A bounded 512-run tail is rescored at most four times per second against 5-45 WPM, with a modest 20 WPM prior. All measured runs remain available for confirmed replay, even when the displayed preview uses only a tail. Preview may include an unfinished character and is allowed to change. It never calls the confirmed-output callback. DecoderStats carries the preview to the GUI; transcriptText composes the confirmed text and a labeled provisional section for display/copy only. Acquisition or reset removes the draft. This offers early feedback without saving guesses as confirmed reception.

The same worker runs two detectors on each packet. The original disables timing recovery; recovery uses symmetric key debounce and a bounded character-gap estimate. It resets calibrated timing once after 12 seconds without keying, preserving uncalibrated buffers. Both transcripts are bounded and retained independently. The default UI shows the original. Changing views never feeds text back into either detector.

Once timing is calibrated, the recovery detector uses a peak-memory time constant of 1.5 dot units, bounded to 30-300 ms, instead of the original fixed roughly 333 ms. This prevents a loud character's peak from masking a following attenuated dot. Acquisition, noise-floor multipliers, tone-lock requirements and debounce limits are unchanged. The original detector keeps its earlier threshold behavior; the recovery checkbox controls the alternative. Tests use abrupt attenuation and varied letters/digits, without station-specific logic.

The window has an optional CW tab and digital frequency tabs. CW Settings,
How to use and the RTTY signal picker open as modals from the shared toolbar.
CW's log toolbar remains inside its tab. Report copying reads the worker snapshot
even when the transcript is hidden. Modal cleanup on disable/destruction closes
only this notebook's popups, preserving other plugins' dialogs and host input.
The log menu participates in the same waterfall input guard as transcript popups.
Closing CW or turning its checkbox off pauses the worker, releases its private
receive channel and hides its tab. Re-enabling reopens/selects the tab, attaches
the channel again and starts fresh timing acquisition, retaining confirmed text
and logs. A worker pause revision acknowledges requests so old snapshots cannot
undo a rapid off/on sequence. Digital monitoring is independent. Close all
closes digital tabs and preserves CW's current enabled/disabled state.
An automatic worker safety pause leaves the CW tab's save/log controls visible
so a storage error cannot hide the recovery actions.

There is no vocabulary suggestion or substitution stage. Markdown saves original chunks and differing recovery chunks. The retired vocabulary preference slot is read and ignored, and written as zero for compatibility with older settings files.

Retuning clears stale queued audio and resets both detectors. By default a station change releases manual speed/tone locks; that behavior is independently configurable. A callback-count watchdog reconnects only the private channel after three seconds without data, with retry delays capped at 30 seconds. It never treats silence or missing decoded characters as a stalled stream, and respects Pause and receiver stop. Failed diagnostic recording disables capture while decoding continues.

Markdown writes use a regular-file check, `O_NOFOLLOW`, append mode, and `fsync`. Rename uses an exclusive hard link followed by unlink, avoiding replacement races. Clear archives with an exclusive link before replacing the active file. The last log filename is stored separately and atomically replaced. Log operations are serialized by the worker.

There is no universal stable binary ABI across SDR++ forks. Rebuild and revalidate this plugin after replacing Brown. Do not attempt to repair ABI mismatches by re-signing or modifying the application bundle.

The transcript uses a read-only ImGui multiline text control with cached word wrapping. Its buffer always receives incoming text, including while selected or scrolled back. Follow text controls scrolling only; a plain click leaves it enabled, while selection or wheel scrolling disables automatic scrolling. Appending preserves selection indices; replacement, trimming, resize or view changes clear stale selection. Cmd+C copies the selection; the context menu also offers the full current transcript. Only the UI thread accesses selection buffers. The host's internal multiline child scroll API is used for auto-follow; recheck matching ImGui headers when rebuilding for a different host.

The plugin claims waterfall input over its visible window, throughout gestures that start there, while its copy menu is open, and while a focused text control handles keys. A drag can leave the window without reaching the waterfall. The guard releases for a fresh outside click, preserving normal tuning.

Enabling the plugin or reopening its transcript requests one centered, focused presentation within the host viewport. The sidebar also offers Bring transcript to front without restarting the stream. Size constraints shrink with the viewport; ordinary frames do not steal focus or recenter the window.

Clear view and successful Clear log operations increment a transcript reset counter in the worker snapshot. The GUI uses that counter to discard held text and selection and resume following live text, even if its window was hidden during the command. Failed archival leaves both the transcript and reset counter unchanged. Clear log is a single-click operation with automatic archival. After archival succeeds it resets Morse timing, discards the worker's current audio packet and clears the queued audio so old characters cannot refill the cleared box.

## Correcting an established speed

Every duration-fit candidate is rejected if two or more recent marks exceed 4.3 proposed dot units. Repeated long marks contradict the fast interpretation even when its bounded-loss score fits the shorter majority. This asymmetric check still tolerates one long transient and avoids forcing a specific WPM or station message.

Ordinary tracking stays within 0.7-1.4 times the current unit. A separate candidate search can escape that range only with 16 recent marks, at least three dots and three dashes, at least six compatible inner gaps, 90% mark support and mean bounded mark loss no greater than 0.12. Four agreeing updates from distinct completed marks must agree within 12%. The change occurs after a gap longer than 2.2 times both timing units, following completion of the current character. This corrects a false startup lock or a sustained operator-speed change without replacing earlier text. Manual speed changes clear any pending automatic correction; manual locks never run the candidate search.

## Boundaries

JS8 and Olivia share the RTTY/PSK raw subscription and wide FFT. One bounded,
utility-priority FEC worker consumes extracted narrowband packets. Four JS8
windows and two Olivia candidates share that worker, with one reusable JS8
core. No per-frequency threads or additional host drains are created. Queue
loss resets only the affected partial FEC blocks. CW and the original digital
worker remain independent. See [FEC modes](fec-modes.md) for acquisition,
metadata, SNR, scheduling and format limits.

Optional RTTY uses a separate full-capture subscription and a bounded single
worker. It shares no decoder state or log queue with CW. Reception tabs, format
coverage, discovery/framing gates, lifecycle and overload limits are documented
in [RTTY monitor](rtty.md). CW remains a one-channel Morse decoder.

GUI metadata publishes Radio's RF passband as an independent RTTY priority
update. The worker applies the latest bounds without resetting unrelated
channels or changing the raw subscription. The tuned region gets local noise
measurement, reserved slots within the existing spectral-peak budget and first
decoder allocation, followed by manual pins and automatic candidates. Explicit
ignore/Only pinned filters remain authoritative. The worker snapshot reports
the region, tuned decoder count and actual receiving state. This adds no DSP
thread, wide FFT, input packet pool or decoder slot.

Retuning, sample-rate changes and dropped-audio resets mark a pending line boundary without adding text. The next decoded character follows exactly one newline after trimming trailing whitespace from the preceding text. Silence and repeated resets cannot accumulate blank lines or start an empty transcript with whitespace. Normal Morse word gaps and message paragraph breaks remain intact within a reception run.

- Each live log write can be at most about one second behind reception; a sudden crash can lose that unsynced tail.
- An abrupt system failure during a disk error cannot guarantee recovery of text still only in memory.
- This is a one-channel decoder, not a band-wide Morse skimmer.
- The UI uses Brown's bundled ImGui version and shares the application's existing accessibility limits.
- No commercial services, update agent, account, license gate, or analytics are part of this local plugin.
