// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sharp_host.h"
#include <imgui.h>
// Standalone interface ownership. No SDR++ ABI or host library is loaded.
namespace ModuleManager {
struct Instance {virtual ~Instance()=default;virtual void postInit()=0;virtual void enable()=0;virtual void disable()=0;virtual bool isEnabled()=0;};
}
namespace sigpath {
struct ReceiverList {std::vector<std::string> getStreamNames()const{return {"Radio"};}};
inline ReceiverList sinkManager;
}
