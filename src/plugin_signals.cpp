// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "notebook_style.h"

void CWNotebook::saveRttySelection(){
    rttySelection_=rtty::validated(std::move(rttySelection_));
    cw::host::setRttySelection(name_,rttySelection_);
    if(rtty_)rtty_->selection(rttySelection_);
}
void CWNotebook::pinRtty(double center,double shift,bool pin){
    rtty::remove(rttySelection_.pinned,center,shift);
    if(pin){
        if(rttySelection_.pinned.size()>=rtty::Skimmer::maxDecoders){rttyNotice_="All 16 priority slots are pinned. Unpin one in Signals first.";return;}
        rttySelection_.pinned.push_back({center,shift});rtty::remove(rttySelection_.ignored,center,shift);
    }
    rttyNotice_.clear();saveRttySelection();
}
void CWNotebook::ignoreRtty(double center,double shift,bool ignore){
    rtty::remove(rttySelection_.ignored,center,shift);
    if(ignore){
        if(rttySelection_.ignored.size()>=64){rttyNotice_="The 64-frequency ignore list is full. Clear ignored frequencies in Signals first.";return;}
        rttySelection_.ignored.push_back({center,shift});rtty::remove(rttySelection_.pinned,center,shift);
    }
    rttyNotice_.clear();saveRttySelection();
}
void CWNotebook::signalPicker(){
    auto s=rtty_?rtty_->snapshot():std::make_shared<const rtty::Status>();
    signalList_.update(*s,rttySelection_);
    ImGui::TextColored(notebook::brass,"Choose what to decode");
    ImGui::TextWrapped("Signals are candidates until protocol and text checks pass. Pin important frequencies for priority, or ignore unwanted ones. Choices are remembered for this receiver profile.");
    if(s->focus.valid())ImGui::TextDisabled("Radio priority %.3f-%.3f kHz | %zu tuned decoders",s->focus.low/1000,s->focus.high/1000,s->focusedDecoders);
    bool only=rttySelection_.onlyPinned;
    if(ImGui::Checkbox("Decode only pinned signals",&only)){rttySelection_.onlyPinned=only;saveRttySelection();}
    notebook::hint("Decode only frequencies you have pinned. Turn off to let the monitor choose other signals too. The list can still discover unpinned signals.");
    notebook::flow("Refresh list");
    if(ImGui::Button("Refresh list")){signalList_.refresh();signalList_.update(*s,rttySelection_);}
    notebook::hint("Replace this table with the latest RTTY pairs and PSK candidates plus saved choices. Until you refresh, row positions and checkbox targets stay fixed. Decoding continues.");
    notebook::flow("Clear ignored");
    if(ImGui::Button("Clear ignored")){rttySelection_.ignored.clear();saveRttySelection();}
    notebook::hint("Allow all previously ignored frequencies to decode and open tabs again. Pinned choices are kept.");
    notebook::flow("Unpin all");
    if(ImGui::Button("Unpin all")){rttySelection_.pinned.clear();saveRttySelection();}
    notebook::hint("Remove every priority pin. If Decode only pinned signals is on, decoding waits until you pin another signal.");
    if(!rttyEnabled_&&!pskEnabled_){
        if(ImGui::Button("Start RTTY monitoring"))setRttyMonitoring(true);
        notebook::hint("Start scanning the captured spectrum for RTTY. Then press Refresh list to see newly discovered pairs.");
    }
    ImGui::TextDisabled("%zu / 16 pinned | %zu ignored | %zu checking | %zu waiting",rttySelection_.pinned.size(),rttySelection_.ignored.size(),s->decoders+s->pskDecoders+s->js8Decoders+s->oliviaDecoders,s->limited);
    if(only&&rttySelection_.pinned.empty())ImGui::TextWrapped("Pin a signal below to start decoding, or turn off Decode only pinned signals.");
    ImGui::TextDisabled("%s",rttyNotice_.empty()?"Rows stay fixed until Refresh list. Signal states update live.":rttyNotice_.c_str());
    if(!rttyNotice_.empty())notebook::hint(rttyNotice_.c_str());
    if(signalList_.rows().empty())ImGui::TextWrapped("Start the receiver and enable RTTY or PSK31/63, then press Refresh list. Pinned and ignored frequencies remain available here.");
    const float height=std::max(100.f,ImGui::GetContentRegionAvail().y-ImGui::GetFrameHeightWithSpacing()-8);
    if(ImGui::BeginTable("rtty_signal_choices",5,ImGuiTableFlags_RowBg|ImGuiTableFlags_ScrollY|ImGuiTableFlags_BordersInnerH,{0,height})){
        ImGui::TableSetupColumn("Pin",ImGuiTableColumnFlags_WidthFixed,34);
        ImGui::TableSetupColumn("Ignore",ImGuiTableColumnFlags_WidthFixed,48);
        ImGui::TableSetupColumn("Center (kHz)",ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Mode / shift",ImGuiTableColumnFlags_WidthFixed,90);
        ImGui::TableSetupColumn("State",ImGuiTableColumnFlags_WidthFixed,150);
        ImGui::TableSetupScrollFreeze(0,1);ImGui::TableHeadersRow();
        size_t row=0;
        for(const auto& signal:signalList_.rows()){
            ImGui::PushID(int(row++));ImGui::TableNextRow();ImGui::TableNextColumn();
            bool pinned=rtty::contains(rttySelection_.pinned,signal.center,signal.shift);
            bool ignored=rtty::contains(rttySelection_.ignored,signal.center,signal.shift);
            if(ImGui::Checkbox("##pin",&pinned))pinRtty(signal.center,signal.shift,pinned);
            notebook::hint(signal.shift==rtty::js8WindowTarget?"Pin this JS8 receive window, about 2.5 kHz wide. All decoded signals inside it can open separate frequency tabs. Decode only pinned accepts the whole window. To prioritize one decoded stream, use Pin signal in its tab.":"Give this frequency priority after Radio's currently tuned passband. Its text still arrives in its own tab. A pin is remembered, but does not tune Radio or follow it when you tune elsewhere.");
            ImGui::TableNextColumn();if(ImGui::Checkbox("##ignore",&ignored))ignoreRtty(signal.center,signal.shift,ignored);
            notebook::hint("Stop decoding this frequency and close its tab. It stays ignored until you untick this box or use Clear ignored.");
            ImGui::TableNextColumn();ImGui::Text("%.3f",signal.center/1000);
            ImGui::TableNextColumn();if(rtty::isJs8(signal.shift))ImGui::TextUnformatted(signal.shift==rtty::js8WindowTarget?"JS8 window":"JS8 signal");else if(signal.shift==rtty::oliviaTarget)ImGui::TextUnformatted("Olivia");else if(signal.shift==0)ImGui::TextUnformatted("PSK31/63");else ImGui::Text("RTTY %.0f",signal.shift);
            ImGui::TableNextColumn();
            auto live=std::find_if(s->signals.begin(),s->signals.end(),[&](const auto& v){return rtty::matches({signal.center,signal.shift},v.center,v.shift);});
            const bool available=live!=s->signals.end()&&live->available;
            const bool confirmed=live!=s->signals.end()&&live->confirmed;
            const bool decoding=live!=s->signals.end()&&live->decoding;
            const bool outside=std::abs(signal.center-lastCenter_)>lastRate_/2;
            const bool monitoring=rtty::isJs8(signal.shift)?js8Enabled_:signal.shift==rtty::oliviaTarget?oliviaEnabled_:signal.shift==0?pskEnabled_:rttyEnabled_;
            const char* state=ignored?"Ignored":!monitoring?"Monitor off":!playing_?"Receiver stopped":outside?"Outside capture":!available?"Waiting for signal":confirmed?"Decoded":decoding?"Checking":pinned?"Priority queued":"Waiting for slot";
            ImGui::TextColored(confirmed?notebook::receiving:notebook::muted,"%s",state);
            notebook::hint("Checking means protocol tests are running. RTTY needs framing and varied text or an RY test; PSK needs phase, timing and varied ASCII Varicode. A candidate alone does not confirm the mode or identify a station.");
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}
