# Credits

## Project

Maverick Text Modes Decoder is maintained by [ReLLL](https://github.com/ReLLL). Its CW detector, timing engine, recoverable journal, transcript integration and installer were developed for this project. Contributions from the community are welcome and will be credited in the changelog and Git history.

## Host and dependencies

- **CTY.DAT** - Jim Reisert, AD1C, for the offline country-prefix reference.
  Redistribution notice: `resources/cty-LICENSE.txt`. Flag associations also
  use public-domain Natural Earth and tzdb ISO country-name data.
- **[SDR++Brown](https://github.com/sannysanoff/SDRPlusPlusBrown)**  -  sannysanoff and contributors. The host application, external-module interfaces, receiver lifecycle, IQ channel/DSP infrastructure and bundled UI. GPL-3.0.
- **[SDR++](https://github.com/AlexandreRouma/SDRPlusPlus)**  -  AlexandreRouma and contributors. Original SDR architecture on which Brown is based. GPL-3.0.
- **[Dear ImGui](https://github.com/ocornut/imgui)**  -  ocornut and contributors. Immediate-mode interface, multiline selection and context menus, provided by the host. MIT.
- **[JSON for Modern C++](https://github.com/nlohmann/json)**  -  nlohmann and contributors. Brown's bundled JSON header is used for module commands/status and configuration interfaces. MIT.
- **[stb_textedit](https://github.com/nothings/stb)**  -  nothings and contributors. Text-editing structures included by the host's Dear ImGui headers. Dual MIT/public-domain license; MIT notice retained.
- **[VOLK](https://www.libvolk.org/)** and **[FFTW](https://www.fftw.org/)**  -  their respective authors and contributors. DSP dependencies of Brown whose development headers are needed when compiling against Brown's interfaces. The macOS plugin links to Brown's bundled VOLK for channel filtering. The plugin does not ship separate copies of these libraries.
- **[Fldigi](https://github.com/w1hkj/fldigi)** - Dave Freese, W1HKJ, and contributors. The ASCII PSK Varicode table is adapted from fldigi's GPL-3.0-or-later table, itself adapted from gmfsk. The PSK demodulator is original project code.
- **[gfsk8-modem-clean](https://github.com/jfrancis42/gfsk8-modem-clean)** - standalone JS8 DSP and message unpacking, derived from JS8Call/WSJT-X. GPL-3.0. Credits include Jordan Sherer, KN4CRD, Allan Bazinet, W6BAZ, the WSJT-X authors and contributors. Pinned source and local changes: `vendor/UPSTREAM.md`.
- **[Olivia / Fldigi](https://github.com/w1hkj/fldigi)** - Pawel Jalocha's MFSK/FEC templates and supporting DSP, maintained by Dave Freese and contributors. GPL-3.0-or-later. Receive integration and resource limits are project code.
- **[KissFFT](https://github.com/mborgerding/kissfft)** - Mark Borgerding and contributors. Vendored by the standalone JS8 core. BSD-3-Clause; notices retained.

The release contains the plugin only, not the Brown application or its dependency libraries. Compiled inline/template portions may come from host/dependency headers. Preserve the host and dependency license notices when distributing combined builds.

## References, not copied decoder code

The [Fldigi documentation](https://www.w1hkj.org/FldigiHelp/cw_configuration_page.html) informed the receive controls and timing workflow. [Priyom](https://priyom.org/military-stations/israel/4xz) provided monitoring background for local tests. Neither project endorses CW Notebook. Adapted Fldigi components are listed above; no Priyom recordings are included.

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [LICENSE](LICENSE).

[ITU-R M.1677-1](https://www.itu.int/rec/R-REC-M.1677-1-200910-I/) supplies the Morse timing reference. [ARRL operating aids](https://www.arrl.org/operating-aids) informed an earlier vocabulary experiment, which was removed before this release. Timing recovery is original project code; no vocabulary substitutions or suggestions are applied. These references do not endorse the plugin.
