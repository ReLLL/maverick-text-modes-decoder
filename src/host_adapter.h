// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#include <string>
#include "rtty_selection.h"
#include "radio_text.h"
namespace cw::host {
#if CW_HOST_BROWN
inline constexpr bool brown=true;
inline constexpr const char* name="SDR++Brown";
#else
inline constexpr bool brown=false;
inline constexpr const char* name="SDR++";
#endif
std::filesystem::path logFolder();
std::string mode(const std::string& receiver);
void useCW(const std::string& receiver);
std::string grid(const std::string& instance);
bool setGrid(std::string value,const std::string& instance);
bool rttyEnabled(const std::string& instance);
bool pskEnabled(const std::string& instance);
bool fecEnabled(const std::string& instance,bool js8);
void setFecEnabled(const std::string& instance,bool js8,bool enabled);
void setPskEnabled(const std::string& instance,bool enabled);
void setRttyEnabled(const std::string& instance,bool enabled);
rtty::Selection rttySelection(const std::string& instance);
void setRttySelection(const std::string& instance,const rtty::Selection& selection);
bool listenRTTY(const std::string& receiver,double mark);
radio::TextOptions textOptions(const std::string& instance);
void setTextOptions(const std::string& instance,const radio::TextOptions& options);
}
