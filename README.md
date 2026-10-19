# CW Notebook for SDR++Brown and SDR++

Decode one CW/Morse signal into a selectable transcript and recoverable Markdown log, alongside RTTY, BPSK31/63, JS8 and Olivia streams in frequency tabs across the captured spectrum. Maintained by [ReLLL](https://github.com/ReLLL). Free and open source under GPL-3.0-or-later.

Version 1.0.49 adds BPSK31/63, all five JS8 speeds and seven common Olivia
formats to the public RTTY monitor. Modes share capture and spectrum analysis,
with bounded decoder slots, tuned-passband priority and pins for wanted signals.
See [PSK operation](docs/psk.md) and [FEC modes and limits](docs/fec-modes.md).

Paged Settings groups decoding, monitoring and display controls. Optional
callsign flags, QRZ tags and colored HAM terms help read received text.
Signal-qualified RX SNR stays visible through gaps, and each stream shows its
last reception time. A conservative RTTY ambiguity check withholds misleading
prefix-extension flags without changing received characters. See
[callsigns and display settings](docs/callsigns.md).

If the transcript is hidden or outside the visible area, use **Bring CW Notebook to front** in the sidebar. The window returns centered and in front without interrupting reception. Disabling and re-enabling also reopens it.

## Features

- Full-capture RTTY discovery and simultaneous frequency tabs, with automatic priority for Radio's tuned passband, manual pins/ignores, up to 16 decoders and 32 retained tabs.
- BPSK31/63, all five JS8 speeds and seven Olivia formats share that spectrum scan. Bounded decoder slots protect responsiveness; pins and Signals controls help select wanted streams when the band is crowded.
- Estimated local RX SNR for CW and every digital mode, with JS8 protocol header callsigns, grids and remote reports when present.
- Automatic timing defaults to 5-45 WPM, with an optional 3-200 WPM range and manual speed/carrier controls.
- Key and gap durations are held during learning and replayed when timing becomes usable. Long buffers spill to a temporary file instead of dropping their beginning.
- Speed learning uses the latest 16 marks and recent gaps. Poor evidence leaves timing unchanged; sustained, strongly supported new timing can correct a mistaken automatic speed lock. Manual speed stays fixed.
- Provisional letters appear after two seconds of detected keying and update as timing improves. The labeled draft is replaced by confirmed replay; only confirmed text is autosaved.
- CW, USB, LSB, DSB and AM reception follows Radio's full selected passband. AM offers keyed RF carriers or Morse audio tones. CW and received digital streams use tabs; Settings and How to use open as modals.
- Closing CW turns off its private receiver and learning. Enabling CW reopens its tab with fresh timing acquisition, preserving confirmed text and logs.
- Readable, resizable text; selection, Cmd+C, and right-click Copy selection / Copy all.
- **Copy to clipboard** exports the displayed transcript with local SQL-style date/time, weekday, UTC offset, current frequency, speed, and carrier offset.
- **Explain with AI...** opens ChatGPT, Claude or Gemini in the default browser with an analysis prompt and reception context. The complete prompt is copied for every provider; Gemini and long transcripts require pasting.
- Digital frequency tabs offer their own **Explain with AI...** button, using that tab's received text and mode metadata. RTTY includes baud, shift and polarity; JS8 includes received header fields.
- Markdown autosave, rename, new log, Finder reveal, and one-click clear with a dated backup.
- Select and scroll while incoming text continues; plugin gestures do not retune the waterfall.
- No extra blank pages during silence or silent tuning.
- Retuning continues on one new line, with automatic reacquisition and stalled-channel recovery.
- Original decode plus an optional timing-recovery view. No vocabulary correction or suggestions.
- Local processing. No accounts, microphone permission, virtual audio cable, telemetry, or network decoder.

## Compatibility

Separate binaries target **Apple Silicon Macs, macOS 15 or newer**. Brown remains the primary, native host with its full integration. The Brown build targets **SDR++Brown 1.2.1, August 4, 2026 build**; the standard build targets **SDR++ 1.2.0, May 28, 2024 build**. Both are tested on macOS 26. Older macOS versions meeting the build target have not been tested.

SDR++ forks do not provide a stable plugin ABI. Never interchange the two binaries. A different host build may require rebuilding against its matching headers. Intel, Windows, Linux host plugins and other forks are not currently validated. The standalone decoder tests need neither host.

| Capability | SDR++Brown (primary) | Standard SDR++ |
| --- | --- | --- |
| CW/USB/LSB/DSB/AM full-filter capture, timing/recovery and reconnect | Full | Full |
| Full-capture RTTY, tuned-passband priority, frequency tabs and RTTY AI handoff | Full | Full |
| Transcript, Markdown logs, archive/rename, selection and clipboard | Full | Full |
| Explain with AI in default browser | Full | Full |
| Receiver modulation and Use CW mode | Native Brown commands | Native SDR++ radio interface |
| Location context | Brown's native My Grid | Optional Receiver grid in Settings > Monitoring; press Enter to save |
| Brown HTTP diagnostic/automation commands | Retained | Host has no equivalent HTTP server |

Each host keeps its own logs and decoder preferences. Changing hosts does not move, erase or merge logs.

This is an independent community plugin, not an official host release. The binaries are not Developer ID signed or notarized. If macOS or your host rejects one, build locally with matching headers; do not modify the host's signature or disable security protections.

## Install

1. Install the compatible [SDR++Brown](https://github.com/sannysanoff/SDRPlusPlusBrown), launch it once, then **quit it**.
2. Download the Apple Silicon ZIP and its `SHA256SUMS.txt` from [Releases](https://github.com/ReLLL/cw-notebook/releases/latest).
3. In Terminal, open the download folder and verify the ZIP:

   ```sh
   shasum -a 256 -c SHA256SUMS.txt
   ```

4. Extract the ZIP, open its folder in Terminal, and run:

   ```sh
   python3 scripts/install.py --binary cw_notebook.dylib
   ```

5. Restart Brown. **CW Notebook** appears in the sidebar. Start the receiver, choose **CW**, **USB**, **LSB**, **DSB** or **AM** in Radio, and tune one Morse signal inside its filter.

Python 3 is required; no Python packages are needed. The installer refuses to run while Brown is open and checks the binary architecture. It backs up the configuration and previous plugin, then installs outside the signed app bundle.

### Standard SDR++ installation

Use the **sdrpp** package and its included `cw_notebook.host.json`; the **brown** package is incompatible. Launch the matching SDR++ once, then quit it. From the extracted standard package run:

```sh
python3 scripts/install.py --host sdrpp --binary cw_notebook.dylib
```

Restart SDR++ and enable CW Notebook. The same decoding and log controls apply. The notebook continues updating when the host sidebar is hidden. For location in AI prompts, open Settings > Monitoring, enter a Maidenhead grid and press Enter. An empty grid is reported as unconfigured.

The installer checks the package's host manifest before changing files, refuses known profiles belonging to the other host, and preserves an existing disabled-plugin preference. Enable the module in Module Manager if an update finds it disabled. `--host brown` is the default; custom profiles still use `--config-dir`.

- Plugin: `~/Library/Application Support/sdrpp-brown/plugins/cw_notebook.dylib`
- Backups: `~/Library/Application Support/sdrpp-brown/cw-notebook-backups/`
- Logs: `~/Documents/SDR++Brown/CW Logs/`

For standard SDR++, the profile is `~/Library/Application Support/sdrpp/`, the plugin is in that profile's `plugins/` folder, and logs/preferences are `~/Documents/SDR++/CW Logs/`.

For a custom Brown profile, add `--config-dir "/path/to/profile"`.

### Update or roll back

Download a newer compatible release for your host, quit that host, and run the same installer with its matching host option. Your logs and decoder preferences remain. There is no automatic updater. Use GitHub's **Watch → Custom → Releases** to follow updates.

To roll back, quit the selected host and restore its previous `cw_notebook.dylib` from the dated backup. Restore the matching `config.json` only if needed; that also restores other settings to the backup time. Keep your current config copy first.

To uninstall, disable/remove the **CW Notebook** instance in Module Manager. With the selected host closed, remove its absolute plugin path from the `modules` array in that host's `config.json` and move only `cw_notebook.dylib` to Trash. Keep the logs.

## Receiving Morse

1. Choose **CW**, **USB**, **LSB**, **DSB** or **AM** and put one Morse signal inside Radio's filter. USB captures above the dial, LSB below it, and DSB/AM on both sides. CW Notebook follows the full selected width, including the tested Brown build's 12 kHz sideband and 15 kHz AM maxima; narrow it to exclude nearby stations. In AM choose **Keyed RF carrier** for ordinary on/off CW (default), or **AM audio tones** for Morse modulating a carrier. Speaker mute does not stop decoding. Voice, overlapping carriers and interference can produce incorrect letters; this remains a one-signal decoder.
2. Tune another station or band normally. Acquisition restarts automatically; the next decoded text starts on one new line. Earlier text remains. Changing modulation also clears unfinished acquisition and starts a new text line. Manual speed/tone locks are released by default; disable **Release manual locks when tuning a new station** in CW Settings to retain them. An intentional Pause remains paused.
3. Learning starts from zero after retuning. Key/gap durations stay buffered until mixed dot/dash evidence and a character gap establish timing. A clean D can unlock at its first character gap. Ambiguous sequences show a labeled provisional decode after about two seconds; confirmed output waits for stronger evidence. Silence retains the buffer. Manual WPM in **CW Settings** can decode held intervals. **Relearn**, retuning, tone changes and stream resets discard uncommitted intervals; confirmed text remains.
4. **Follow** controls automatic scrolling only. Clicking never freezes new text. Selecting or scrolling back holds the scroll position while text keeps arriving; enable Follow to jump to the latest.
5. **Clear view** clears the screen only. **Clear log** archives the file and completely clears the displayed text, unfinished characters and queued audio. Newly received text can then appear.

**Copy to clipboard**, beside Clear log, copies the selected version without artificial display wrapping, including text received while scrolled back or on another tab. Its timestamp and metadata describe the moment of copying. Right-click **Copy all** copies text without metadata. **Log file...** contains Open, Show in Finder, Rename and New log.

**Explain with AI...** has ChatGPT, Claude and Gemini choices. It uses the displayed original or recovery version, including its labeled provisional section, and adds current frequency, band vicinity, approximate wavelength, Radio mode/filter, Brown's configured Maidenhead grid, local date/time and weekday, WPM, and carrier offset. AM audio-tone input reports the RF carrier offset as unavailable rather than confusing an audio pitch with RF offset. Earlier transcript lines may have been received at other frequencies; the prompt states that its metadata describes the current export.

The prompt asks for abbreviation/prosign/callsign explanations, conversation flow, explicit uncertainty and a distinction between received text and suggested corrections. Your original transcript is unchanged. Choosing a provider shares this context with that site when the link carries the prompt. No API, account credentials, background upload or browser extension is used. Normal site login and Send controls still apply. ChatGPT and Claude are opened with a prompt link when the encoded URL fits 6,000 bytes; site support can vary. Gemini has no verified browser prompt-link support here, so paste the copied prompt and send. Longer prompts use the same clipboard fallback without truncating text. Feedback in the notebook explains the next action.

Markdown logs use UTC section timestamps. Retuning starts a new frequency section. Rename never overwrites another log; the `.md` extension is automatic.

### Recovery controls

The original decode is the default. The selector above the transcript switches between **Original decode** and **Timing + fade recovery (unverified)**. Repair is enabled for the separate version by default in **Settings > CW decoding**. During learning, a labeled provisional section appears after two seconds of detected keying and can be revised every quarter second. Its first-pass speed stays within 5-45 WPM with a modest 20 WPM preference for ambiguous evidence. It does not call the confirmed-output callback or enter the Markdown log. Confirmed replay replaces it; already confirmed text is not retroactively rewritten. Retuning clears the provisional section and its measured intervals.

Recovery follows sudden amplitude fades at the learned element speed, so a previous loud character is less likely to hide a quieter dot. It keeps the original noise-floor and carrier checks. It also filters very short key glitches, estimates uneven character spacing, and relearns timing after a long quiet interval. It can still misinterpret a signal, and cannot reconstruct elements lost below the detectable noise level. No dictionary, station template, repeated-message substitution or vocabulary suggestions are used. Changing messages require no special action.

Autosave retains labeled original chunks and differing timing-recovery chunks in the same Markdown file. Turning an option off affects future decoding, not historical text. **Clear log** clears both views after archiving successfully. A restart restores settings and the log path; session text is available in the saved file, not reloaded into the box. Old log entries are preserved when updating from a version that included vocabulary hints.

If receiver samples stop arriving, the plugin retries its private channel after three seconds, backing off to 30 seconds. It leaves a stopped receiver or deliberate Pause alone. **Reconnect decoder** retries manually. **Save 20s signal** stores a private mono float32 capture under the log folder's `Diagnostics` directory: 8 kHz in CW; 16 kHz for other supported filters up to 6 kHz, 32 kHz up to 12 kHz, and 64 kHz for wider AM filters. Continuous raw audio is not recorded.

### Limits

- CW decodes one signal at a time. Optional RTTY has its own bounded spectrum monitor.
- On the tested older SDR++ 1.2.0 host, keep Radio's full filter within the captured RF spectrum. Extending it past an edge can trigger the host's spectrum bounds bug; see [validation](docs/validation.md).
- Automatic timing defaults to 5-45 WPM. Manual speed and the extended range support 3-200 WPM in synthetic tests; this is not a guarantee for every real signal or sending style.
- Learning buffers last for the current uninterrupted reception. Temporary storage does not survive a crash, restart or explicit reset. Disk failures are reported. Buffering cannot recover RF/audio samples missed before tone detection or during a stream interruption.
- Fading, interference, weak signals and nonstandard timing can cause errors in either view. `[?]` marks unknown patterns.
- Deep fading can destabilize automatic speed. A manual speed lock may help.
- The displayed transcript is bounded to roughly 80-100 KB; saved logs are not trimmed. Saving failures retain pending text up to a visible 2 MB safety limit.
- Silence cannot identify a station, and a continuous carrier is not Morse.

## RTTY monitor

The toolbar has independent **CW** and **RTTY** checkboxes. CW pauses and
resumes the existing decoder while retaining its text and log. RTTY scans
the captured RF spectrum and opens confirmed streams in frequency tabs, without
changing Radio's tuning, mode, filter or audio. All eligible pairs inside Radio's
current filter get first priority automatically, while the rest of the captured
band keeps scanning. Closing the CW tab's X turns CW off and stops its private
receive channel and timing learning. Turn **CW** on to reopen the tab and
restart acquisition; its confirmed text and Markdown log are retained.
New tabs keep your current focus. A filled circle before the frequency is green
for one minute after confirmed activity, then yellow. Every returning burst
restarts the minute; the trailing dot marks
unread text. **Close all tabs**
clears digital tabs and pending digital text while preserving CW's current
enabled/disabled state and its log.
**Signals...** lets you pin priority frequencies, decode only those pins, and
ignore unwanted traffic. Choices are remembered. **Settings** and **How to
use** open as modal windows, keeping the tab strip for received signals. Signals
rows stay fixed until **Refresh list**. Changing notices use a fixed status bar;
hover it for details and hover controls for explanations. **Compact RY** folds
long test patterns for reading. **Copy to clipboard** includes the displayed text
and reception metadata. Turn Compact RY off before copying to include all test
letters. The report labels the view and estimates; band labels describe frequency
ranges, not a station's identity.
**Explain with AI...** uses that same displayed view and tab metadata. Choose
ChatGPT, Claude or Gemini; the complete prompt is copied for every site. Gemini
and long transcripts require pasting. You still press Send in the browser.

Automatic detection covers common five-bit RTTY formats with 170, 200, 425, 450
and 850 Hz shifts, with 16 simultaneous
candidate decoders, 32 retained tabs, bounded input queues and visible overload
warnings. Receiver capture rates up to 4.096 MS/s are supported. Weak,
overlapping or unsupported signals may be missed; detection of every signal is
not guaranteed. RTTY text is held in memory: copy it before closing or quitting.
Repeated single-letter patterns alone stay unconfirmed. A short or repetitive
legitimate test may need varied traffic or an alternating RY test before opening
a tab. Confirmed text is retained literally, including repeated characters.
Existing CW Markdown autosave remains separate. The monitor defaults on when
no preference exists, and remembers your choice per host, including turning it off.

Building from source does not replace the installed plugin until you run the
installer. See [RTTY operation, architecture and test evidence](docs/rtty.md).

## Build from source

Install Xcode Command Line Tools and CMake. Homebrew headers needed by Brown's interfaces:

```sh
brew install cmake fftw volk
git clone https://github.com/ReLLL/cw-notebook.git
git clone https://github.com/sannysanoff/SDRPlusPlusBrown.git
git -C SDRPlusPlusBrown checkout 679f48deee5299a199d15d30e6e785fac6054896
cd cw-notebook
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBROWN_SOURCE="../SDRPlusPlusBrown" \
  -DBROWN_APP="/Applications/SDR++Brown.app" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
python3 scripts/install.py
```

The pinned headers match the supported August 4 Brown host. Brown is the default `CW_HOST=brown`; its native event and commands remain intact. Standard SDR++ uses a separate build directory:

```sh
git clone https://github.com/AlexandreRouma/SDRPlusPlus.git ../SDRPlusPlus
git -C ../SDRPlusPlus checkout 762444d3406d68fbc9d4dff8cc237d382793a6e3
cmake -S . -B build-sdrpp -DCW_HOST=sdrpp \
  -DSDRPP_SOURCE="../SDRPlusPlus" -DSDRPP_APP="/Applications/SDR++.app" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build build-sdrpp --parallel 4
python3 scripts/install.py --host sdrpp --binary build-sdrpp/cw_notebook.dylib
```

For a different host revision, use its matching source and verify the ImGui ABI. Do not replace the host's bundled ImGui. Each build emits `cw_notebook.host.json` beside the binary.

For core and installer tests without Brown:

```sh
sh scripts/check.sh
```

## Architecture

```text
Host IQ stream (Brown or SDR++)
  → private IQ channel following Radio's full passband (8 kHz CW, 16/32/64 kHz otherwise)
  → map the selected passband to positive mono tones, preserving key timing
  → bounded audio queue
  → one decoder / log worker
       ├─ tone detection → key transitions → Morse timing → text
       ├─ recoverable Markdown journal
       └─ snapshot → ImGui transcript and clipboard
```

The GUI never performs DSP or log writes. CW retains one stream-drain thread and one worker. Optional RTTY adds one raw-IQ drain and one independent worker, sharing spectrum analysis across its bounded decoders. There is no thread per station. Retuning and audio gaps reset affected acquisition. Input guards keep plugin mouse gestures out of the waterfall.

Details: [architecture](docs/architecture.md), [validation](docs/validation.md), [release process](docs/RELEASING.md).

## Contribute and license

Bug reports, patches, decoder improvements and ports are welcome through [Issues](https://github.com/ReLLL/cw-notebook/issues) and pull requests. See [CONTRIBUTING.md](CONTRIBUTING.md).

Copyright © 2026 ReLLL and contributors. Licensed under **GNU GPL version 3 or, at your option, any later version**. You may use, study, modify and redistribute the project, including commercially, under that license. Distributed derivatives must preserve the applicable license and source obligations. No warranty is provided.

See [LICENSE](LICENSE), [CREDITS.md](CREDITS.md), and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
