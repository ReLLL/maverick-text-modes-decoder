---
read_when: Changing PSK decoding, discovery, resource limits, or receive lifecycle
---

# PSK31 / PSK63 monitor

**PSK31/63** enables receive-only BPSK at 31.25 and 62.5 baud across captured IQ.
Both rates are checked together, independently of Radio's demodulator, audio
volume and the selected tab. QPSK, PSK63F and extended character sets are
unsupported. The checkbox defaults on and remembers an explicit off.

Confirmed streams open tabs such as `14071.234 kHz PSK31`. Frequency estimates
the RF carrier; it is not the Radio dial and displayed precision does not
guarantee receiver accuracy. Tabs open without stealing focus or tuning Radio.
Each has Follow PSK, text selection, metadata copying, Explain with AI, Pin
signal, Ignore & close and explicit Tune Radio. The AI menu uses PSK text and
metadata with the same provider/clipboard workflow as CW and RTTY.

Green means confirmed activity within the last minute; returning activity
restarts the minute. Valid idle after acquisition keeps activity alive without
printing characters. Idle alone cannot initially qualify a tab. Stopping or
disabling PSK preserves text. Digital text is memory-only; copy before closing
or quitting. CW logging is independent. Close all tabs retains CW.

## Selection and limits

Eligible candidates inside Radio's passband get first priority, then pins,
common PSK windows and other captured signals. An 80 Hz region around a candidate must fit inside the
filter for priority, covering either rate before identification. Moving Radio
does not reset other streams. Pins/ignores distinguish PSK from RTTY at the same
frequency; a PSK pin applies to either PSK rate. Decode only pinned applies to
both monitors. Signals rows stay fixed until Refresh list. Candidates are
labeled PSK31/63 until protocol checks identify the rate.

Common windows cover 4 kHz above the USB dial references 3580, 7040, 14070,
21070 and 28120 kHz from the [ARRL PSK31 handout](https://www.arrl.org/files/file/Get%20on%20the%20Air%20with%20HF%20Digital/Getting%20Started%20with%20PSK31%20and%20JT65--Handout.pdf).
These are receive hints, not exclusive scan ranges or transmission permissions.
The entire captured spectrum stays under continuous observation. Common-window
preference cannot interrupt confirmed streams or tuned/pinned candidates. When
ordinary candidates are also present, they can claim one unconfirmed slot so
common windows cannot occupy every probation slot. If all slots are confirmed
or protected by tuning/pins, other candidates wait until capacity is available.
Pin a wanted signal to give it priority over common windows.

PSK shares RTTY's raw subscription, fixed packet pool, wide FFT and worker.
It adds no raw-IQ copy, station thread, network service or second wide FFT.
Both digital checkboxes off detaches the raw stream. The existing 8 kS/s to
4.096 MS/s range and 250 ms queue bound remain. RTTY keeps 16 decoder slots;
PSK has eight separate slots. At most 24 candidate channels run together,
sharing **32 retained digital tabs**. The flush status bar shows both budgets,
waiting candidates, tabs and gaps. Pin wanted frequencies, disable an unused
monitor or lower capture rate if overloaded. Pins do not increase capacity.

## DSP and lifecycle

The existing averaged spectrum supplies narrow-energy candidates at most eight
times per second. Ranking and suppression retain at most 256 candidates.
Each PSK channel uses a 128-point inverse FFT and overlap-add extraction. A
finite anti-alias filter resamples to 1 kS/s. Two independently filtered trials
recover differential phase, carrier offset, symbol timing and ASCII Varicode.
Acquisition requires phase coherence, consistent Varicode and varied printable
text. A spectrum peak or plausible-looking letters alone is insufficient.
The selected rate stays fixed until loss/reacquisition; recent framing failures
can revoke lock without editing received text. Backspace appears as `[BS]`;
unsupported control codes never execute.

Gaps, RF-center/sample-rate changes, stopping and disabling a mode discard its
partial symbols. Close all also discards pending digital text. Normal
discontinuities retain tabs. Returning reception reuses an inactive tab at the
same frequency and rate. Unconfirmed candidates rotate after ten seconds with
a three-second automatic cooldown; tuned/pinned candidates bypass the cooldown.
Confirmed stalls expire after six seconds, disappearing candidates after two.

Short or repetitive real messages may stay unconfirmed. Finite filtering, noise,
fast drift, deep fades, closely spaced/overlapping signals and capture edges
can prevent acquisition or corrupt text. PSK discovery excludes the outer seven
spectrum bins; hardware filters may further reduce usable edges. There is no
forward error correction or automatic language correction in this BPSK path.
Field sensitivity and error rate remain unmeasured.

Controlled tests use an independently transcribed ASCII reference and cosine
phase-reversal waveform, separate from the decoder's mapping table. Completed
checks and processing measurements belong in [validation](validation.md).

The operator workflow draws on [Fldigi's signal browser](https://www.w1hkj.org/FldigiHelp/signal_browser_page.html),
using this project's stable signal table and individual tabs. Protocol references:
[G3PLX's PSK31 fundamentals](https://det.bi.ehu.es/jtpjatae/psk31theory.html),
[ARRL specification](https://www.arrl.org/psk31-spec),
[Fldigi mode table](https://www.w1hkj.org/FldigiHelp/mode_table_page.html).
