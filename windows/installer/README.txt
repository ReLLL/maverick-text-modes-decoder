Maverick Text Modes Decoder: CW / RTTY / JS8 / Olivia / PSK31 / PSK63
SDR# .NET 9 x64 package. Developer: ReLLL.

Close SDR#, run Install.cmd and select SDRSharp.dotnet9.exe.
Keep the complete extracted package together for its integrity checks.

Manual install: copy the complete MaverickDecoder folder into SDR#'s Plugins
directory, or its configured core.pluginsDirectory. Keep both folder levels,
the adapter DLL, native executable, runtime DLLs and flags together.
Back up an older plugin outside the scanned plugins directory first.

Open Maverick Decoder from SDR#'s plugins panel. Closing the decoder window
hides it while enabled reception continues. CW logs and preferences stay in
%LOCALAPPDATA%\Maverick Decoder\SDRSharp. Digital tabs are memory-only.

See WINDOWS.md for requirements, limitations and build details.
Feedback: https://github.com/ReLLL/maverick-text-modes-decoder/issues
https://x.com/FoxOnTheRunTr
