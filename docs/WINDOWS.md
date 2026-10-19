# SDR# Windows preview

Maverick Text Modes Decoder: CW / RTTY / JS8 / Olivia / PSK31 / PSK63 has a
Windows SDR# preview. The existing SDR++ and SDR++Brown integrations remain
separate. The official host loads the adapter and decodes the synthetic CW/RTTY
test capture. See [validation evidence and limits](#validation-and-limits).

## Download and install

Target: Windows x64, SDR# .NET 9, official revision 1.0.0.1921. Validation uses
Windows 11 ARM64 under Parallels x64 emulation. Other SDR# versions, x86 builds
and .NET 8 are not validated. Official SDK and host hashes are recorded in
`windows/source-pin.json`. SDK reference assemblies are not distributed.

Extract the complete Windows package. Quit the selected SDR# installation,
run `Install.cmd`, and select `SDRSharp.dotnet9.exe`. The PowerShell installer
checks package integrity, preserves data and backs up previous binaries outside
the scanned plugins directory. It never changes SDR# binaries.

For manual installation, copy the complete top-level `MaverickDecoder` folder
into SDR#'s `Plugins` directory. Keep both levels intact:

```text
Plugins/MaverickDecoder/SDRSharp.MaverickDecoder.dll
Plugins/MaverickDecoder/MaverickDecoder/cw_sdrsharp.exe
Plugins/MaverickDecoder/MaverickDecoder/<runtime DLLs and flags>
```

Respect `core.pluginsDirectory` if SDR# configures a different directory.
Move an old plugin folder to a backup outside that directory before replacing
it. No Plugins.xml entry is required.

Enable Maverick Decoder in SDR#'s plugins panel. Open restores the current
notebook. Closing the window hides it while enabled reception continues.
Disabling the plugin stops private capture and retains current text. Digital
tabs are memory-only; copy wanted reports before quitting. CW logs are saved
under `%LOCALAPPDATA%/Maverick Decoder/SDRSharp/CW Logs`; other preferences stay
in that SDRSharp data folder. Uninstalling the plugin preserves this data.

## Capture and isolation

The adapter receives the official raw-IQ hook. Native SDR# downconversion,
resampling and filtering prepare a private CW channel using the shared
CW/USB/LSB/DSB/AM passband plan. One bounded raw-IQ IPC path supplies the existing
full-capture RTTY/PSK/JS8/Olivia engines at supported rates up to 4.096 MS/s.
Higher rates are reported as unsupported rather than silently cropping RF.
No host sample buffer is modified. No per-station threads are added.

The 4.096 MS/s ceiling is an accepted input range, not a real-time performance
guarantee. Start with a moderate sample rate and watch the status bar for IQ or
FEC gaps. Lower the source sample rate or disable unused modes if gaps increase.
Windows ARM64 x64 emulation can require more CPU than native x64 hardware.

The decoder runs in a lower-priority process with bounded queues. Stops,
capture-center changes and gaps invalidate partial reception. Changing only
Radio's tuned passband reprioritizes digital streams without resetting them.
The worker watches its SDR# parent; closing the host stops its workers.
Each data folder permits one notebook instance. Unicode data paths are supported.

## Validation and limits

Validated on 2026-10-19 with official SDR# 1.0.0.1921 x64 .NET 9 under Windows
11 ARM64 Parallels emulation:

- Actual host installation and raw-IQ reception with a separate synthetic
  192 kS/s recording: a 20 WPM CW transcript, RX SNR, callsign flags and an RTTY
  frequency tab appeared together. The operator confirmed the manual test.
  CW log content was checked and all synthetic recordings/logs were removed
  after the test host and worker stopped. Normal receiver profiles were untouched.
- Production SDR# downconverter/resampler/filter tests at 192 kS/s, 256 kS/s,
  2.048 MS/s and 3.2 MS/s: output rates 8/16/32/64 kS/s, CW and wide sideband/AM
  plans, frequency direction, passband gain and unchanged host samples.
- Native Windows tests for RTTY/200 Hz shifts, BPSK31/63, every supported JS8
  speed, seven Olivia formats, interference rejection, SNR retention, callsign
  annotations and Unicode log operations. These are synthetic decoder tests;
  JS8/Olivia/PSK reception was not separately verified through the host GUI.
- The real ImGui transcript passes Ctrl+C, selection, append, flags and popup
  checks. IPC tests reject stale epochs and tuned CW layouts after stop, retune,
  mode or filter changes, while keeping unrelated raw digital input eligible.
- Closing the owned host releases the worker and its exclusive data lock.
  Native hide/reopen and real-radio performance across the full supported rate
  range still need broader operator testing.

The emulated all-mode host run accumulated IQ/FEC queue gaps during tuning and
test load. Queues remain bounded and affected decoders reacquire; this is not a
claim of gap-free reception or a field error-rate measurement. No decoder
thresholds were relaxed for this port. Report rate, enabled modes, gap growth
and host/CPU version with performance feedback.

## Build

Use MSYS2 UCRT64 GCC, CMake, Ninja and GLFW, plus the .NET 9 SDK and official
Airspy SDR# SDK reference assemblies. Supply the pinned Brown source tree only
for its shared ImGui sources and json.hpp; no SDR++ ABI is loaded on Windows.

```sh
cmake -S . -B build-windows -G Ninja -DCW_HOST=sdrsharp \
  -DCMAKE_BUILD_TYPE=Release -DCW_UI_SOURCE=/path/to/pinned/Brown/source
cmake --build build-windows --parallel 4
ctest --test-dir build-windows --output-on-failure
python windows/tests/check_ipc.py build-windows/windows/cw_sharp_ipc_tests.exe
dotnet build windows/SDRSharp.CWNotebook/SDRSharp.CWNotebook.csproj \
  -c Release -p:SdrSharpSdk=/path/to/official/sdk/lib
```

`scripts/package_windows.py` follows native DLL imports recursively, includes
runtime licenses, flag attribution and provenance, and writes a SHA-256
manifest. Preserve matching runtime source/provenance with any public package.
Actual-host tests must use a separate SDR# copy and an isolated
`MAVERICK_DECODER_DATA_ROOT`; remove synthetic receptions after writers stop.

Developer: [ReLLL](https://github.com/ReLLL). Feedback through
[GitHub](https://github.com/ReLLL/maverick-text-modes-decoder/issues) or
[FoxOnTheRunTr](https://x.com/FoxOnTheRunTr).
