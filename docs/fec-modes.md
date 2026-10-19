---
read_when: Changing JS8, Olivia, metadata, SNR, or FEC worker lifecycle
---

# JS8, Olivia and receive measurements

JS8 and Olivia default on. Each remembers an explicit off. CW, RTTY and PSK
remain independently controlled. The full captured spectrum is surveyed with
the existing wide FFT. At most four JS8 windows and two Olivia candidates run
together. One JS8 window can decode multiple streams into separate frequency
tabs. All digital modes share 32 retained tabs. Pin wanted frequencies or JS8
windows in Signals; Decode only pinned and ignores remain authoritative.

JS8 supports Normal, Fast, Turbo, Slow and Ultra. Olivia checks 4/125, 4/250,
8/250, 8/500, 16/500, 16/1000 and 32/1000. Other Olivia variants are currently
unsupported. FEC, synchronization and text gates qualify acquisition; energy alone never opens a tab.
JS8 needs a complete timed transmission and a reasonably accurate system clock.
Olivia needs at least 24 seconds of samples, a stable format winner for four
seconds, and varied text before a tab qualifies. Weak reception can take longer.
Discovery requires spectral energy spread, so a strong narrow carrier's FFT
skirts do not consume the limited Olivia probes.

Radio's passband comes first, then saved pins, common JS8 windows and other
captured candidates. Common JS8 references come from the pinned JS8Call source.
Tuned/pinned/common JS8 regions also get probes below the energy threshold.
Elsewhere, very weak signals below the spectrum gate may be missed. Olivia
discovery groups spectral energy; close/overlapping streams can merge into one
candidate. A pin can explicitly probe a wanted Olivia frequency. No mode can
guarantee reception through interference, missing samples or deep fading.

Unconfirmed JS8/Olivia probes rotate after 65/55 seconds, with a five-second
retry cooldown. This includes tuned and pinned probes: a false candidate must
not monopolize all slots. Confirmed JS8 stalls expire after 70 seconds, Olivia
after 25. Stopping, disabling, retuning and gaps discard partial blocks while
retaining text. Returning reception reuses the tab. Close all discards pending
digital text and preserves CW's current on/off state. Digital text is memory-only.

## Performance and isolation

The raw subscription, fixed raw packet pool and wide FFT are shared with RTTY
and PSK. FEC adds small channel inverse FFTs, not another wide transform or host
drain. One utility-priority worker handles all JS8/Olivia DSP. Its fixed pool
holds 256 narrowband packets, roughly one second at the maximum six-window hop
cadence. Overflow restarts affected FEC channels; it does not reset CW/RTTY/PSK.
Both raw-IQ and FEC gap counters are visible in the fixed status bar.

All JS8 windows share one decoder core and FFT plans. Receive history is cleared
between jobs so separate RF windows cannot combine each other's soft bits.
Submode storage is lazy; only due modes run at their period boundaries. Each
window retains a fixed 60-second PCM ring. Fractional low-pass interpolation and
spectral masks prevent resampling images from appearing as duplicate stations.
The GUI never runs FEC. No accounts, automatic downloads, telemetry or network
services are added. See validation.md for measured limits and completed checks.

## Metadata and SNR

Tabs show the estimated frequency and detected format above the text. JS8 can
carry callsigns, Maidenhead grids and signal reports in protocol fields. These
are displayed as the last received header, not verified station identity.
Missing fields remain unavailable. A grid is an approximate area, not GPS.
Olivia has no mandatory location header; text is never mined for invented
metadata. JS8's JSC dictionary is lossless protocol decompression, not language
correction or a source of guessed missing words.

RX SNR is an approximate locally measured ratio, referenced to 2500 Hz of
noise. JS8 supplies its frame decoder estimate. CW, RTTY, PSK and Olivia reuse
existing spectra: integrated occupied-band power minus estimated noise,
divided by local noise density times 2500 Hz. Median noise samples exclude the
signal and a guard band. Missing/invalid noise or clipped capture edges yield
unavailable, not a fabricated zero. Fading, keying, nearby interference and
receiver filtering can bias the estimate. This is not calibrated dBm or S-meter
strength. Remote reports carried over the air remain separate fields. Clipboard
reports and AI prompts preserve the distinction.

References: [JS8Call source](https://github.com/JS8Call-improved/JS8Call-improved),
[standalone JS8 core](https://github.com/jfrancis42/gfsk8-modem-clean),
[Fldigi Olivia](https://www.w1hkj.org/modes/olivia.htm).
