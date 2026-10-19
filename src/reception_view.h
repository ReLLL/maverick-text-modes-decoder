// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "notebook_style.h"
#include "reception.h"
#include <array>
namespace notebook {
inline ImVec4 snrColor(double snr){
    if(!std::isfinite(snr))return muted;
    const ImVec4 red{.95f,.39f,.32f,1},yellow{1,.82f,.32f,1},green{.38f,.85f,.52f,1};
    const auto level=std::clamp((snr+30)/40,0.,1.);
    const auto from=level<=.5?red:yellow,to=level<=.5?yellow:green;
    const float mix=float(level<=.5?level*2:(level-.5)*2);
    return {from.x+(to.x-from.x)*mix,from.y+(to.y-from.y)*mix,from.z+(to.z-from.z)*mix,1};
}
inline void rxSnr(const radio::Reception& r,bool receiving){
    if(std::isfinite(r.snr))ImGui::TextColored(snrColor(r.snr),"RX SNR %+.1f dB%s",r.snr,r.currentSnr(receiving)?"":" (last)");
    else ImGui::TextDisabled("RX SNR --");
    hint("Received SNR measured here, referenced to 2500 Hz of noise. Red through yellow to green spans -30 to +10 dB; this is not a decoder confidence score. 'Last' retains the latest measurement from detected signal activity through gaps or pauses. Noise alone does not replace it. It is separate from a remote station's report. Different modes use different estimators.");
}
inline void lastReceived(const radio::Reception& r){
    ImGui::TextDisabled("Last received: %s",radio::receptionAge(r).c_str());
    hint("Latest newly decoded data in this frequency/mode stream. Shows elapsed seconds for under two minutes, then the saved local date, time and zone. Signal noise, duplicate JS8 frames and UI refreshes do not refresh this timestamp.");
}
}
