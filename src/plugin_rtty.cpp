// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "notebook_style.h"
#include "reception_view.h"
#include "rtty_presentation.h"
#include "rtty_report.h"
#include <cstdio>
#include <cmath>

void CWNotebook::setRttyMonitoring(bool enabled){
    rttyEnabled_=enabled;rttyEnabledView_=enabled;
    cw::host::setRttyEnabled(name_,enabled);
    syncDigitalMonitoring();
}
void CWNotebook::setPskMonitoring(bool enabled){
    pskEnabled_=enabled;cw::host::setPskEnabled(name_,enabled);syncDigitalMonitoring();
}
void CWNotebook::setFecMonitoring(bool js8,bool enabled){
    if(js8)js8Enabled_=enabled;else oliviaEnabled_=enabled;
    cw::host::setFecEnabled(name_,js8,enabled);syncDigitalMonitoring();
}
void CWNotebook::syncDigitalMonitoring(){
    rttyEnabledView_=rttyEnabled_;pskEnabledView_=pskEnabled_;
    js8EnabledView_=js8Enabled_;oliviaEnabledView_=oliviaEnabled_;
    if(digitalEnabled()&&!rtty_){rtty_=std::make_unique<RttyReceiver>();rtty_->selection(rttySelection_);}
    if(rtty_){rtty_->modes(rttyEnabled_,pskEnabled_);rtty_->fecModes(js8Enabled_,oliviaEnabled_);rtty_->enabled(enabled_&&digitalEnabled());}
    metadata();
}
void CWNotebook::listenRtty(double mark){
    if(!cw::host::listenRTTY(streamName_,mark))rttyNotice_="Move the capture center so Radio's full 3.2 kHz filter fits before listening.";
    else rttyNotice_.clear();
}
void CWNotebook::rttyControls(){
    bool cwEnabled=cwTab_.enabled();
    if(ImGui::Checkbox("CW",&cwEnabled))setCWMonitoring(cwEnabled);
    notebook::hint("Open or close the CW tab and start or stop its receiver and timing learning. Closing the tab's X does the same as turning this off. Text and Markdown logs are kept; turning CW on starts fresh timing acquisition. Digital modes continue independently.");
    ImGui::SameLine();
    bool enable=rttyEnabled_;
    if(ImGui::Checkbox("RTTY",&enable))setRttyMonitoring(enable);
    notebook::hint("Scan the full captured RF spectrum for RTTY. All eligible pairs inside Radio's current filter get first priority, followed by pins and other signals. Turn off to stop RTTY processing and keep existing text. This choice is remembered.");
    ImGui::SameLine();bool psk=pskEnabled_;
    if(ImGui::Checkbox("PSK31/63",&psk))setPskMonitoring(psk);
    notebook::hint("Scan the full capture continuously for BPSK31 and BPSK63 together, independent of Radio mode. Priority: Radio's passband, pins, common PSK windows, then other signals. Eight PSK decoder slots share the existing FFT and worker. Off keeps received text. This choice is remembered; QPSK and PSK63F are not supported.");
    notebook::flow(75);bool js8=js8Enabled_;if(ImGui::Checkbox("JS8",&js8))setFecMonitoring(true,js8);
    notebook::hint("Receive JS8Call text at all five speeds. The full capture stays surveyed; four bounded receive windows prioritize Radio's filter and pins. FEC runs on one shared background worker, separate from CW/RTTY/PSK. This choice is remembered.");
    notebook::flow(85);bool olivia=oliviaEnabled_;if(ImGui::Checkbox("Olivia",&olivia))setFecMonitoring(false,olivia);
    notebook::hint("Receive Olivia 4/125, 4/250, 8/250, 8/500, 16/500, 16/1000 and 32/1000 with forward error correction. Two candidate frequencies run together; pins and Radio's filter take priority. This choice is remembered.");
    notebook::flow("Close all tabs");
    if(ImGui::Button("Close all tabs")){if(rtty_)rtty_->closeAll();selectCW_=true;}
    notebook::hint("Close all digital frequency tabs and discard their text. CW keeps its current on/off state. Monitoring continues, so fresh traffic can open new tabs. Copy wanted text first; these tabs are not autosaved.");
    notebook::flow("CW Settings");if(ImGui::Button("CW Settings"))openNotebookModal(1);
    notebook::hint("Open CW speed, tone, recovery and diagnostic controls. Digital modes have separate controls in their tabs and Signals.");
    notebook::flow("How to use");if(ImGui::Button("How to use"))openNotebookModal(2);
    notebook::hint("Open the operating guide for CW, RTTY, PSK, JS8 and Olivia, including tuning, copying, pins and receiver limits.");
    notebook::flow("Signals...");if(ImGui::Button("Signals..."))openNotebookModal(3);
    notebook::hint("Choose digital frequencies or JS8 receive windows to decode. Pin wanted stations, ignore unwanted ones, or decode only your pins. The table stays still until Refresh list.");
}
void CWNotebook::rttyTabs(float footer){
    if(!rtty_)return;
    auto snapshot=rtty_->snapshot();
    for(auto it=rttyViews_.begin();it!=rttyViews_.end();){
        bool exists=std::any_of(snapshot->stations.begin(),snapshot->stations.end(),[&](const auto& s){return s.id==it->first;});
        if(!exists)it=rttyViews_.erase(it);else ++it;
    }
    for(const auto& s:snapshot->stations){
        char title[96];std::snprintf(title,sizeof(title),"    %.3f kHz %s###rtty_%llu",s.frequency/1000,rtty::modeName(s),static_cast<unsigned long long>(s.id));
        bool open=true;
        auto& view=rttyViews_[s.id];
        auto flags=view.seen==s.text?ImGuiTabItemFlags_None:ImGuiTabItemFlags_UnsavedDocument;
        const bool selected=notebook::beginModeTab(title,&open,flags,rtty::modeName(s));
        notebook::tabActivity(rtty::recentText(s.lastTextAt,rtty::monotonicSeconds()));
        if(selected){
            view.seen=s.text;
            if(!s.mode.empty())ImGui::TextDisabled("Estimated RF %.0f Hz | %s | %s",s.frequency,s.format.c_str(),s.active?"Recent frame":"Inactive");
            else if(s.psk)ImGui::TextDisabled("Estimated carrier %.0f Hz | %s | %.2f baud | %s",s.frequency,rtty::modeName(s),s.baud,s.active?"Receiving":"Inactive");
            else ImGui::TextDisabled("Estimated mark %.0f Hz | %.2f baud | %.0f Hz shift | %s | %s",s.frequency,s.baud,s.shift,s.reverse?"Reverse":"Normal",s.active?"Receiving":"Inactive");
            notebook::rxSnr(s,s.active);
            notebook::flow(310);notebook::lastReceived(s);
            if(!s.mode.empty()){
                ImGui::TextDisabled("Last header: Call %s | Grid %s | Remote report %s",s.call.empty()?"--":s.call.c_str(),s.grid.empty()?"--":s.grid.c_str(),s.remoteReport.empty()?"--":s.remoteReport.c_str());
                notebook::hint("Fields carried in decoded protocol headers, when present. A received Maidenhead grid describes the station's approximate area. Remote reports are separate from the SNR measured by this receiver. Missing fields stay blank.");
                if(s.mode=="JS8"){
                    if(view.lastCall!=s.call){view.lastCall=s.call;view.country=radio::callsignCountry(s.call);}
                    if(!view.country.name.empty()){
                        flags_.draw(view.country.flag);ImGui::Text("%s",view.country.name.c_str());
                    }else ImGui::TextDisabled("Country --");
                    notebook::hint("Country/DXCC entity estimated offline from the last received header callsign, including known exact-call exceptions and portable prefixes. A flag represents the associated country. This does not verify identity or locate a transmitter. The received grid is shown separately; no grid is invented from a callsign.");
                }
            }
            if(ImGui::Button("Copy to clipboard"))view.transcript.copyRTTYReport(s,*snapshot,rtty::isRtty(s)&&view.compact);
            notebook::hint("Copy displayed text with date, local time zone, band, estimated frequency, mode, baud and capture details. RTTY also includes shift and polarity. Turn off Compact RY for every RTTY test letter.");
            notebook::flow("Explain with AI...");
            if(ImGui::Button("Explain with AI..."))ImGui::OpenPopup("notebook_rtty_ai_actions");
            notebook::hint("Choose ChatGPT, Claude or Gemini. Shares this tab's displayed text and reception metadata when you choose a site, and copies the complete prompt. You still press Send.");
            controlsPopupOpen_|=ImGui::IsPopupOpen("notebook_rtty_ai_actions");
            if(ImGui::BeginPopup("notebook_rtty_ai_actions")){
                const bool hasText=!s.text.empty();
                if(!hasText)ImGui::TextDisabled("Receive text first.");
                for(int provider=0;provider<3;++provider){
                    const char* names[]{"ChatGPT","Claude","Gemini"};
                    if(ImGui::MenuItem(names[provider],nullptr,false,hasText))
                        openAnalysisPrompt(provider,rtty::analysisPrompt(s,*snapshot,rtty::isRtty(s)&&view.compact,std::time(nullptr),cw::host::grid(name_)));
                    notebook::hint(provider==2?"Copy the full analysis prompt and open Gemini. Paste and send it yourself.":"Open the site with this tab's displayed text and metadata. The full prompt is also copied for pasting; you still press Send.");
                }
                ImGui::Separator();ImGui::TextDisabled("Full prompt copied for every site.");
                ImGui::TextDisabled("Gemini / long transcripts: paste and send.");
                ImGui::EndPopup();
            }
            notebook::flow(110);std::string follow=std::string("Follow ")+(s.psk?"PSK":rtty::modeName(s));ImGui::Checkbox(follow.c_str(),&view.follow);
            notebook::hint("Automatically scroll to the newest text in this tab. Turn off or select text to hold your reading position. Decoding continues either way.");
            notebook::flow(100);bool pinned=rtty::contains(rttySelection_.pinned,s.center,s.shift);
            if(ImGui::Checkbox("Pin signal",&pinned))pinRtty(s.center,s.shift,pinned);
            notebook::hint("Give this frequency priority after Radio's passband when slots are busy: 16 RTTY, 8 PSK, 4 JS8 windows and 2 Olivia candidates. A pin is remembered and works with Decode only pinned signals. It does not tune Radio or freeze this tab.");
            if(rtty::isRtty(s)){notebook::flow("Compact RY");ImGui::Checkbox("Compact RY",&view.compact);
            notebook::hint("Show long, exact RYRY test sequences as [RY test pattern] to keep messages readable. These letters are transmitted test traffic. Turn off to display and copy them all. The report labels which view you copied.");}
            notebook::flow("Ignore & close");if(ImGui::Button("Ignore & close"))ignoreRtty(s.center,s.shift,true);
            notebook::hint("Stop decoding this frequency, discard its tab and prevent it reopening. Undo in Signals by unticking Ignore or choosing Clear ignored. Copy wanted text first.");
            notebook::flow("Tune Radio");if(ImGui::Button("Tune Radio")){
                listenRtty(s.frequency);
            }
            notebook::hint("Listen through Radio's current audio output. Selects USB / 3.2 kHz with this estimated signal at 2125 Hz; CW relearns at the new dial. Only this button changes tuning; opening a tab does not.");
            ImGui::TextDisabled("Received text is held in these tabs; copy it before closing. CW Markdown logging remains separate.");
            if(view.rawCache!=s.text){view.rawCache=s.text;view.compactCache=rtty::compactRY(s.text);}
            view.transcript.draw(rtty::isRtty(s)&&view.compact?view.compactCache:s.text,std::max(80.f,ImGui::GetContentRegionAvail().y-footer-24),textSize_,view.follow);
            if(view.transcript.popupOpen())controlsPopupOpen_=true;
            ImGui::EndTabItem();
        }
        if(!open)rtty_->close(s.id);
    }
}
