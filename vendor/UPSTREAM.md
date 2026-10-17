# Receive DSP source map

- `gfsk8/`: [jfrancis42/gfsk8-modem-clean](https://github.com/jfrancis42/gfsk8-modem-clean),
  commit `bd6457e0bd4c3a10e74d1c1aaa5e86f67565168a` (GPL-3.0).
  Original notices remain. This includes JS8Call message decompression tables;
  they are protocol data, not vocabulary correction. KissFFT retains its BSD
  notices and full license alongside the sources.
- `olivia/`: [w1hkj/fldigi](https://github.com/w1hkj/fldigi), commit
  `61b97f4133c488063f3de1795c894d22d5032e8a` (GPL-3.0-or-later), the eight
  `src/include/jalocha/pj_*` headers needed by the Olivia receiver.

Local changes:

1. JS8 positive compile-time rounding uses `value + 0.5` before integer
   conversion, since Apple's C++20 library does not make `std::round` constexpr.
2. JS8 exposes a submode-mask setter and receive-history clearing. One worker
   reuses one core across RF windows; soft bits never cross window boundaries.
3. JS8 submode storage uses lazy `unique_ptr` alternatives rather than putting
   the largest mode inline in every variant. FFT plans remain reusable.
4. Olivia array allocation counts elements once, instead of multiplying an
   element count by `sizeof` inside `new[]`.
5. Olivia's synchronization noise lookup selects the same phase/frequency as
   its signal peak. The upstream post-loop pointer was one past the row and
   could read past the buffer on the final phase. The integrated ASan replay
   reproduces this before the fix.
6. Olivia caps strong synchronization scores at 100 rather than resetting them
   to zero, which otherwise rejects valid strong frames.
7. The project supplies an isolated CMake target; unused upstream apps/tests
   and the obsolete `js8codec.cpp` implementation are not compiled.
8. JS8 synchronization clears the final unused spectral columns on every
   search. The FFT window spans eight steps, but NHSYM reserves three trailing
   steps, leaving four unwritten columns that the timing search can read.
   Repeated all-mode fixtures exposed intermittent Ultra misses before this
   initialization; no sensitivity threshold is changed.

Independent JS8 physical-layer oracle: original Fortran `genjs8`, `encode174`
and Boost CRC-12 from `a7ff1be0b389d287fdc56e2ea0d06962aa68127d` of
[js8call](https://github.com/js8call/js8call). Frozen normal/modified Costas
tone sequences are checked in `tests/fec_tests.cpp`.
