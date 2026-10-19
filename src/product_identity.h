// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
namespace notebook {
inline constexpr const char* productName="Maverick Text Modes Decoder: CW / RTTY / JS8 / Olivia / PSK31 / PSK63";
inline constexpr const char* shortName="Maverick Text Modes Decoder";
#ifdef _WIN32
inline constexpr const char* copyShortcut="Ctrl+C";
inline constexpr const char* revealLabel="Show in Explorer";
inline constexpr const char* revealHint="Reveal the current CW Markdown log in Explorer.";
#else
inline constexpr const char* copyShortcut="Cmd+C";
inline constexpr const char* revealLabel="Show in Finder";
inline constexpr const char* revealHint="Reveal the current CW Markdown log in Finder.";
#endif
// Preserve the existing ImGui identity and saved window geometry.
inline constexpr const char* windowName="Maverick Text Modes Decoder: CW / RTTY / JS8 / Olivia / PSK31 / PSK63###cw_notebook_window";
}
