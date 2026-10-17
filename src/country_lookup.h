// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
namespace radio {
struct Country {std::string name,flag;};
// Offline CTY estimate from a received callsign, never a transmitter position.
Country callsignCountry(const std::string& call);
}
