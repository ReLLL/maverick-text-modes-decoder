// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_skimmer.h"
#include <array>
#include <algorithm>
#include <cmath>
namespace psk {
// Receive hints: ARRL PSK31 USB dial frequencies plus a 4 kHz audio window.
// These never restrict discovery or change the receiver frequency.
inline bool commonWindow(double frequency){
    constexpr std::array<double,5> bases{3580000,7040000,14070000,21070000,28120000};
    return std::isfinite(frequency)&&std::any_of(bases.begin(),bases.end(),[&](double base){return frequency>=base&&frequency<base+4000;});
}
inline int priority(double frequency,const rtty::Focus& focus,const rtty::Selection& selection){
    return focus.contains(frequency,80)?3:rtty::contains(selection.pinned,frequency,0)?2:commonWindow(frequency)?1:0;
}
// Soft common-window preference cannot interrupt a confirmed stream or take
// the last ordinary slot. Ordinary candidates can claim one probation slot.
inline bool mayReplace(int incoming,int current,bool confirmed,size_t ordinary,double incomingPower,double currentPower){
    if(incoming>=2)return incoming>current;
    if(current>=2||confirmed)return false;
    if(incoming==1&&current==0)return ordinary>1;
    if(incoming==0&&current==1)return ordinary==0;
    return incoming==current&&incomingPower>currentPower*4;
}
}
