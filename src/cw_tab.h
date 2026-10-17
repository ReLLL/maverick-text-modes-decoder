// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui.h>
#include "notebook_style.h"
#include <algorithm>
#include <optional>
#include <cstdint>

namespace notebook {
// GUI owns the requested state. A late worker snapshot must not undo a click.
class CWTab {
public:
    bool enabled() const{return enabled_;}
    bool open() const{return open_;}
    void request(bool enabled,uint64_t revision){
        enabled_=open_=enabled;pendingRevision_=std::max(pendingRevision_.value_or(0),revision)+1;
    }
    void sync(bool paused,uint64_t revision){
        if(pendingRevision_){if(revision<*pendingRevision_)return;pendingRevision_.reset();}
        enabled_=!paused;
    }
    bool begin(bool select,bool& closed){
        closed=false;
        if(!open_)return false;
        bool open=true;
        const bool visible=beginModeTab("CW",&open,ImGuiTabItemFlags_Leading|(select?ImGuiTabItemFlags_SetSelected:0),"CW");
        closed=!open;
        return visible;
    }
private:
    // An automatic worker safety pause keeps log/save controls accessible.
    bool enabled_=true,open_=true;
    std::optional<uint64_t> pendingRevision_;
};
}
