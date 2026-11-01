// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
namespace cw::host {
// Uses only macOS image enumeration and file hashing, before host/UI/DSP access.
std::string compatibilityError(bool installerProbe=false);
}
