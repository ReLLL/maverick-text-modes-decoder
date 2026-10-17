# Third-party notices

## Offline country reference

`resources/cty.dat` is the 23 March 2024 CTY.DAT snapshot used by the matching
Brown/DX Notebook reference, maintained by Jim Reisert, AD1C. Its redistribution
permission remains verbatim in `resources/cty-LICENSE.txt` and in binary
packages. Country-prefix and exact-call rules estimate a DXCC entity; they do
not verify a transmitter's identity or location. This is an offline snapshot.

`resources/country-flags.tsv` associates entities with native country flags.
Country names/codes were derived from the DX Notebook Natural Earth snapshot
and tzdb's public-domain `iso3166.tab`, with regional associations. Country
names remain separate from received Maidenhead grids. Unmapped associations
use a neutral flag. Native flag artwork is rendered by macOS; no flag artwork
bundle or remote service is distributed.

CW Notebook uses headers and interfaces from SDR++Brown and its dependencies. See CREDITS.md for source links. The host application and its shared libraries are not bundled.

## SDR++Brown and SDR++

GPL-3.0; the full GNU GPL version 3 text is in LICENSE. Corresponding host source is available at https://github.com/sannysanoff/SDRPlusPlusBrown and https://github.com/AlexandreRouma/SDRPlusPlus.

## Dear ImGui 1.87

The MIT License (MIT)

Copyright (c) 2014-2022 Omar Cornut

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## JSON for Modern C++ 3.9.0

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2013-2019 Niels Lohmann <http://nlohmann.me>.

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## PSK ASCII Varicode table (GPL-3.0-or-later)

The 128-entry protocol mapping in `src/psk_varicode.h` is adapted from
[fldigi's pskvaricode.cxx](https://github.com/w1hkj/fldigi/blob/cc957f37cf89cecd167db8ef6c72486b5ef5a103/src/psk/pskvaricode.cxx).
Copyright (C) 2006 Dave Freese, W1HKJ; adapted by fldigi from gmfsk.
Distributed under GPL version 3 or later; see the included `LICENSE`.
The PSK demodulator and spectrum monitor are new project code.

## JS8 receive DSP and message unpacking (GPL-3.0)

Pinned standalone source: https://github.com/jfrancis42/gfsk8-modem-clean at
bd6457e0bd4c3a10e74d1c1aaa5e86f67565168a, derived from JS8Call and WSJT-X.
Original credits include Jordan Sherer (2018), Allan Bazinet (2025), the
WSJT-X authors and contributors. The GPL version 3 text is in LICENSE and
vendor/gfsk8/COPYING. Local changes are listed in vendor/UPSTREAM.md.

## Olivia receive templates (GPL-3.0-or-later)

Pawel Jalocha's MFSK/FEC and supporting DSP templates from Fldigi,
61b97f4133c488063f3de1795c894d22d5032e8a. Original notices include
Copyright 1999-2004 Pawel Jalocha and 2019 David Freese, W1HKJ.
The GPL version 3 text is in LICENSE and vendor/olivia/COPYING.

## KissFFT (BSD-3-Clause)

Copyright (c) 2003-2010 Mark Borgerding . All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## stb_textedit (MIT option)


Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
