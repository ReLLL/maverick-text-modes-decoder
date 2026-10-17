---
read_when: Changing RTTY discovery, resource limits, tabs, or receive lifecycle
---

# RTTY monitor (1.0.42)

Local 1.0.43 adds [PSK31/63](psk.md) through this same capture, FFT and worker.
RTTY keeps its 16 decoder slots; PSK has eight slots. Both share 32 retained
digital tabs. Close all tabs closes both digital modes and retains CW.

RTTY is an optional receiver alongside the existing CW decoder. It scans the
host's captured RF spectrum, independently of Radio's mode, selected passband,
speaker volume and the selected notebook tab. It cannot receive outside that
capture or recover samples lost at the source.

## Operator workflow

- The toolbar starts with **CW**, then **RTTY**, for independent decoding control.
  Turn CW off, or close its tab's X, to stop CW reception and timing learning.
  Its tab closes; confirmed text and the Markdown log are retained.
  Turning it back on reopens/selects the tab and relearns timing. CW starts enabled each session, matching
  the previous Pause/Resume behavior. RTTY keeps running when CW is off.
- **RTTY** defaults on when this module instance has no saved preference.
  Choices, including off, are remembered separately for each host.
- Every eligible pair with both tones inside Radio's current passband receives
  first priority. Multiple pairs are decoded together, within the 16-decoder
  limit. Tuning, mode and filter-width changes move this priority automatically;
  other captured frequencies keep scanning. USB is above the dial, LSB below
  it, and AM/CW/DSB and the supported FM/raw filters are centered. The priority
  region is clipped to the RF capture. Pins come after this tuned priority;
  explicit ignores and **Decode only pinned signals** still apply. The fixed
  status bar shows the tuned decoder count; hover for the exact priority range.
- A confirmed stream opens a tab such as `14085.125 kHz RTTY`. The label is the
  estimated mark frequency rounded to 1 Hz, not Radio's dial frequency. Actual
  accuracy depends on the receiver calibration, signal and spectral resolution.
- New tabs never steal focus or tune Radio. Tabs can be reordered and scrolled.
  A filled circle before the frequency is green for one wall-clock minute after
  confirmed RTTY activity, then yellow. Each returning burst restarts the full
  minute, including valid received frames that add no printed letters.
  This works for unselected tabs and ages
  after reception stops. The trailing dot marks unread text. CW stays first
  and cannot be closed.
- **CW Settings** and **How to use** open modal windows from buttons beside
  **Close all tabs**. The tab strip contains only CW and received RTTY streams.
- Open **Signals...** on a crowded band. Pin frequencies to give them priority
  over automatic candidates, or select **Decode only pinned signals**. A newly
  pinned waiting pair can displace an automatic decoder at the 16-decoder limit.
  Pins, ignores and this selection mode are saved per host. Pinned frequencies
  remain listed when reception stops or they leave the capture.
  The table's rows, frequencies and checkbox targets stay fixed until **Refresh
  list**, or reopening the dialog. Signal states update in place. Saved ignores
  also remain listed for undoing them. Newly discovered pairs appear on refresh.
- **Ignore & close** removes an unwanted frequency's tab and prevents reopening.
  Remove that ignore in Signals, or use **Clear ignored**. Ordinary X / Close
  all closes text without ignoring the signal. Up to 16 pins and 64 ignores are
  retained; reaching either limit reports how to free space.
- Each RTTY tab has independent text selection and Follow controls. Select and
  copy with Cmd+C, use the context menu, or click **Copy to clipboard**. Scrolling
  back does not pause decoding. Baud, shift, polarity and receiving/inactive
  state remain visible.
- **Compact RY** folds exact alternating runs of at least 12 R/Y characters into
  `[RY test pattern]` in the display only. RY is transmitted teleprinter test
  traffic. Turn compaction off to display and copy every letter. Clipboard
  copying follows the displayed version, with its view explicitly labeled.
- **Copy to clipboard** includes local date/time, day, UTC offset, frequency-band
  range, estimated mark and pair-center frequencies, baud, nominal shift,
  polarity, receiving/inactive state, current capture range/rate and gap count.
  HF amateur-range labels follow the [IARU Region 1 plan](https://www.iaru-r1.org/reference/band-plans/hf-bandplan/);
  they do not identify the received station or grant transmission permission.
  Other HF receptions are labeled outside those listed ranges. Selection and
  context-menu copying remain available without metadata.
- **Explain with AI...** opens the same ChatGPT/Claude/Gemini menu as CW, using
  this tab's displayed compact/original RTTY view and reception report. It adds
  the configured approximate receiver grid when available and asks for uncertain
  interpretations to stay separate from literal reception. Choosing a site
  copies the complete prompt and opens the browser. Gemini and long prompts
  require pasting; you still press Send. There is no background AI analysis.
- Changing counts, gap warnings and notices use one fixed-height status bar,
  flush with the notebook edges. Hover for details; hover controls for help.
- **Close all tabs** closes all RTTY tabs, discards pending RTTY decoding and
  selects CW. It does not clear CW or its log. Monitoring continues; newly
  acquired traffic can open fresh tabs. Pins/ignores remain. Individual tabs have
  an X button.
- **Tune Radio** is an explicit listening action: USB, 3.2 kHz bandwidth and a
  2125 Hz mark tone through Radio's current audio output. It changes Radio's dial
  and restarts CW acquisition while preserving confirmed CW text. It is rejected
  if the complete Radio passband would exceed the capture, protecting the tested
  older host's known edge bug. Selecting a tab alone never tunes anything.
- Stopped reception or disabling monitoring keeps the existing RTTY text
  available to copy. RTTY text is bounded and held in memory; it is not autosaved
  or restored after quitting. CW's existing Markdown logging is unchanged.

## Supported receive formats and limits

Automatic trials cover 45.45, 50, 75 and 100 baud; 170, 200, 425, 450 and 850 Hz
nominal shift; normal and reversed polarity; five-bit ITA2/Baudot with a start
bit and at least 1.5 stop bits. National punctuation variants, other shifts,
other baud rates, ASCII, SITOR/NAVTEX, encrypted traffic and other FSK protocols
are outside this implementation. Two tones alone do not prove RTTY.

Both tones must be inside the usable captured spectrum. The first/last two FFT
bins are excluded from peak finding, and hardware filters can further attenuate
the capture edges. Weak signals, severe fading, overlapping signals, frequency
drift and nonstandard framing can prevent acquisition or corrupt text. The
current gates favor rejecting uncertain candidates; no universal detection rate
or on-air error rate has been established.

The monitor accepts 8 kS/s through 4.096 MS/s after host decimation. Above that
limit it detaches its raw stream and shows a warning. At most **16 candidates**
are decoded concurrently, with **32 retained tabs** and approximately 32-40 KB
of text per tab. A visible capacity warning reports excess candidates or full
tabs. Closing inactive tabs frees tab capacity; reducing capture bandwidth may
help with sustained CPU overload. Captured spectrum is scanned together, not
retuned or swept in pieces.

## Processing and safety boundaries

One shared complex FFT uses half-window overlap, a Hann window, roughly
15.625-31.25 Hz bins and Apple Accelerate on macOS. It runs continuously over
the capture. Power spectra are averaged; peak pairing runs at most eight times
per second. Pair matching considers both tone spacing and relative power.
Stronger occupied bands suppress overlapping sideband candidates. Ambiguous
425/450 Hz pairs select the nearest measured spacing.

At most 256 spectral peaks enter the bounded pairing pass. Each retained pair
has the same framing checks. Up to 64 of these existing peak slots are reserved
for peaks inside Radio's passband so distant strong signals cannot consume the
entire budget first. Its noise estimate uses that passband's own median with
the same multiplier and local peak checks. No extra wide FFT is added.
Pairs inside the tuned passband are considered before occupied-band suppression
and decoder allocation. New priority pairs can displace lower-priority decoders,
including confirmed ones, while retaining their text in inactive tabs. Existing
tuned decoders are protected from outside automatic candidates and pins.
No finite budget guarantees every stream when the tuned filter itself contains
more than 16 eligible pairs.

Each retained pair
uses a 128-point inverse transform and overlap-add channel extraction; channels
run at 2-4 kS/s. Matched tone filters feed independent baud/polarity framing
trials. A proposed pair must explain substantial channel power, not merely
decode bits leaking from another pair. At least eight valid frames and a bounded
framing-error count remain required. Initial acquisition additionally requires
either 12 exact alternating R/Y characters, or at least 12 non-space characters
with varied values, no character exceeding 75% of the recent sample, at least
four character changes covering a third of adjacent non-space pairs, and a
Shannon entropy of at least 2.2 bits per character. The
sample is bounded to the last 64 decoded bytes. This prevents periodic I/P-heavy
patterns, including occasional other letters, from qualifying on framing alone.
The strongest framing trial must qualify; a weaker wrong-rate/polarity reading
cannot bypass a stronger trial's ambiguous result. These are evidence gates,
not a language model or proof of station identity.

After acquisition the selected baud/polarity trial stays fixed for that decoder;
gap/stall recovery creates a fresh decoder as before. The evidence gate never
filters or rewrites an established transcript. Legitimate single-letter tests
can remain unconfirmed until more varied traffic arrives. RY-only test traffic
is still supported. No universal field false-positive rate is established.

Unconfirmed candidates rotate after four seconds with a cooldown, so steady
interference cannot permanently monopolize the decoder budget. Disappearing
tones expire after two seconds; stalled confirmed decoders expire after six.
Pinned and tuned candidates retry immediately instead of entering the automatic cooldown.
Confirmed text remains readable in inactive tabs. Selection controls are bounded,
validated, deduplicated and applied by the worker; they do not add decoder threads.
At the decoder limit, a new pair with over four times the weaker-tone power can
replace the weakest unconfirmed, unpinned candidate. This prevents startup
sidebands from blocking a stronger real pair. Confirmed streams are retained;
operator pins keep their existing priority.

The optional path adds one host drain thread and one RTTY worker, with no thread
per station and no worker pool. Four preallocated 262,144-sample packets plus a
worker packet bound input storage to about 10 MiB; queued data is additionally
limited to 250 ms. The host callback only tries the queue lock and copies data.
It never waits for DSP, allocates input buffers, writes files or touches ImGui.
Missing packets carry discontinuity generations; decoders reset before later
samples, preventing characters assembled across a gap. UI snapshots are
immutable and published at most four times per second during reception.

Retuning RF center, changing sample rate, stopping the receiver and clearing
tabs invalidate partial RTTY state. Moving Radio's dial/filter does not retune
the full-capture subscription or clear other RTTY streams; it changes priority.
The raw subscription is detached when monitoring/module operation is
disabled or the sample rate is unsupported. Only that private subscription is
changed. CW's channel, algorithms, worker, journal and preferences retain their
existing behavior.

Brown's new RTTY commands are `rtty_enable` (`1`/`0`), `rtty_close_all` and
`rtty_status`; `rtty_listen` accepts an explicit mark frequency in Hz.
`rtty_status` includes the worker's `receiving` state, `priority_low` and
`priority_high` RF bounds, and `priority_decoders`. These describe the current
Radio priority without relying on the host's separate playback-status endpoint.
`show_settings`, `show_help`, `show_signals` and `close_modal` provide the same
modal actions for Brown's existing automation. Mutations are handed to the GUI
thread before touching the host;
status uses an immutable snapshot. Standard SDR++ retains its separate native
frame hook and matching binary. No Brown HTTP interface is added to standard
SDR++.

For isolated host replays, set the process-local `CW_NOTEBOOK_LOG_DIR` to an
absolute owned temporary directory and use the host's separate `-r` profile.
The override changes neither host's normal log directory. Never feed synthetic
CW into normal reception history.

## Deterministic evidence

`rtty_tests` covers all 40 baud/shift/polarity combinations, ITA2 letters and
figures, arbitrary callback boundaries, four widely separated simultaneous
stations (including both 170 and 200 Hz) at 1.024 MS/s and 96 kS/s, exact 200 Hz
discovery at 8 kS/s, 96 kS/s and 4.096 MS/s in both polarities, capture-edge
signals at 8 kS/s, 96 kS/s,
2.4 MS/s and 4.096 MS/s, fixed-seed noise plus two steady carriers, queue limits,
retune/reset, individual close and Close all. A 24-station fixture must decode
16 streams while reporting the other eight as limited. A 96 kS/s regression
rejects duplicate tabs caused by a strong signal's modulation sidebands.
The crowded fixture also verifies that a pinned waiting signal displaces an
automatic decoder, Only pinned stops unwanted decoders, ignored traffic cannot
reopen a tab, and clearing choices restores automatic reception.

Representative Release DSP timings on the development Mac, excluding fixture
generation, host rendering and input copying: about 0.40 seconds per 10 seconds
for three streams at 1.024 MS/s; about 0.17 seconds per 10 seconds for 16 active
decoders at 96 kS/s. These are CPU-workload measurements, not total application
CPU percentages, hard deadlines or a portable performance guarantee. Higher
sample rates cost more; input gaps and capacity limits are visible.

See [validation](validation.md) for host results. Controlled signals prove
integration and reproducible behavior; they do not establish field sensitivity.

Protocol/operator references: [Fldigi RTTY manual](https://www.w1hkj.org/FldigiHelp/rtty_page.html),
[RTTY Skimmer Server](https://www.dxatlas.com/RttySkimServ/).
