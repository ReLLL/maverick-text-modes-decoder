// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "notebook_style.h"
#include <cstdio>

void CWNotebook::receiveStatus(const cw::Snapshot& cw){
    auto r=rtty_?rtty_->snapshot():std::make_shared<const rtty::Status>();
    const char* state=!digitalEnabled()?"off":lastRate_>rtty::Skimmer::maxRate?"unsupported rate":!playing_?"receiver stopped":"monitoring";
    char line[512],capture[180];
    std::snprintf(line,sizeof(line),"%s | RTTY %zu/16%s | PSK %zu/8%s | JS8 %zu/4%s | Olivia %zu/2%s | %zu/32 tabs | %zu waiting | %llu IQ / %llu FEC gaps",state,r->decoders,rttyEnabled_?"":" off",r->pskDecoders,pskEnabled_?"":" off",r->js8Decoders,js8Enabled_?"":" off",r->oliviaDecoders,oliviaEnabled_?"":" off",r->stations.size(),r->limited,static_cast<unsigned long long>(r->dropped),static_cast<unsigned long long>(r->fecDropped));
    std::snprintf(capture,sizeof(capture),"Captured RF: %.3f-%.3f kHz at %.3f MS/s.",(lastCenter_-lastRate_/2)/1000,(lastCenter_+lastRate_/2)/1000,lastRate_/1e6);
    std::string summary=line,details=summary+"\n"+capture;
    if(r->focus.valid()){
        char priority[180];std::snprintf(priority,sizeof(priority),"\nRadio priority: %.3f-%.3f kHz. Every eligible pair here is checked first; the rest of the capture is still scanned.",r->focus.low/1000,r->focus.high/1000);details+=priority;
    }
    if(r->limited)details+="\nChoose priority frequencies in Signals, or close inactive tabs if tab capacity is full.";
    if(r->dropped)details+="\nInput gaps restart digital decoding. Lower the capture rate if the gap count keeps increasing.";
    if(lastRate_>rtty::Skimmer::maxRate)details+="\nSelect a capture rate of 4.096 MS/s or less for digital monitoring.";
    if(digitalEnabled()&&r->stations.empty())details+="\nTabs open after protocol checks. JS8 frames need a complete transmission; Olivia acquisition can take several FEC blocks. Frequency candidates alone do not confirm a mode.";
    if(!digitalEnabled())details+="\nDigital processing is stopped; existing tab text remains available to copy.";
    details+="\nJS8 and Olivia share one bounded background FEC worker.";
    if(r->fecDropped)details+="\nFEC queue gaps restart only the affected text decoder; CW/RTTY/PSK processing continues. Pin wanted frequencies or disable an unused mode if gaps keep increasing.";
    std::string notice=!cw.error.empty()?"CW: "+cw.error:!r->error.empty()?"Digital: "+r->error:!rttyNotice_.empty()?rttyNotice_:"";
    if(cw.dropped)details+="\nCW input gaps: "+std::to_string(cw.dropped)+". CW detection restarted.";
    if(notice.empty()&&ImGui::GetTime()-aiNoticeAt_<20&&!aiNotice_.empty())notice=aiNotice_;
    if(!notice.empty()){summary=notice+" | "+summary;details=notice+"\n"+details;}
    else if(!cw.notice.empty())details+="\nCW: "+cw.notice;
    details+="\nDigital tab circles: green for one minute after confirmed activity. Each returning burst restarts the minute; yellow otherwise. Hover controls for help.";
    notebook::statusBar(summary.c_str(),details.c_str(),!notice.empty()||r->limited||r->dropped||r->fecDropped||cw.dropped);
}
