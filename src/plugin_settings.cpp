// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "notebook_style.h"
namespace {
void group(const char* title){ImGui::Spacing();ImGui::TextColored(notebook::brass,"%s",title);ImGui::Spacing();}
}
void CWNotebook::settings(const cw::Snapshot& s){
    settingsNavigation_.draw([&](int page){
        if(page==0){
            group("Station identification");
            if(ImGui::Checkbox("Detect Callsigns and Deduce Country",&textOptions_.detectCallsigns))cw::host::setTextOptions(name_,textOptions_);
            notebook::hint("Recognize plausible callsigns in confirmed received text across all modes. Add country flags beside them and clickable callsign tags above the text. RTTY withholds a one-off call's flag/tag if removing one leading letter matches another call repeated in this transcript; received letters stay unchanged. Clicking a tag opens QRZ.com in your default browser. Prefix estimates can be wrong; no lookup is sent automatically. Your original received text and logs are kept.");
            ImGui::TextWrapped("Offline callsign-prefix estimates. Received grids and remote reports stay separate.");
            group("Readability");
            if(ImGui::Checkbox("Highlight HAM QSO terms",&textOptions_.highlightTerms))cw::host::setTextOptions(name_,textOptions_);
            notebook::hint("Color calling terms such as CQ and QRZ, identity terms such as DE and QTH, reports such as 599 and SNR, and courtesy terms such as 73 and TNX. Display only; no received words are replaced.");
            ImGui::TextColored({.53f,.78f,.98f,1},"K1ABC  Callsigns");
            ImGui::TextColored({.98f,.76f,.38f,1},"CQ  QRZ  Calling");
            ImGui::TextColored({.50f,.82f,.76f,1},"DE  QTH  Identity");
            ImGui::TextColored({.81f,.68f,.94f,1},"599  SNR  Reports");
            ImGui::TextColored({.65f,.82f,.52f,1},"73  TNX  Courtesy");
            ImGui::Spacing();ImGui::TextWrapped("Flags are reading annotations. Selected text copies them as Unicode flags. Full reports and Markdown logs retain the original received text.");
        }else if(page==1){
            decoderControls(s);
        }else if(page==2){
            group("Receive modes");
            bool cw=cwTab_.enabled();if(ImGui::Checkbox("CW",&cw))setCWMonitoring(cw);
            notebook::hint("Open or close CW and start or stop its private receive channel and timing learning. Confirmed text and logs stay available when re-enabled.");
            bool rtty=rttyEnabled_;if(ImGui::Checkbox("RTTY",&rtty))setRttyMonitoring(rtty);
            notebook::hint("Scan the full captured spectrum for RTTY with up to 16 decoders. Radio's passband and pins receive priority.");
            bool psk=pskEnabled_;if(ImGui::Checkbox("PSK31/63",&psk))setPskMonitoring(psk);
            notebook::hint("Scan the full capture for BPSK31 and BPSK63, sharing the existing digital FFT and eight PSK decoder slots.");
            bool js8=js8Enabled_;if(ImGui::Checkbox("JS8",&js8))setFecMonitoring(true,js8);
            notebook::hint("Enable all five JS8 receive speeds in up to four shared receive windows. Keep your computer clock accurate.");
            bool olivia=oliviaEnabled_;if(ImGui::Checkbox("Olivia",&olivia))setFecMonitoring(false,olivia);
            notebook::hint("Check seven common Olivia formats using two candidate slots on the shared FEC worker.");
            group("Receiver context");
            ImGui::Text("%s | %s | %.0f Hz",streamName_.c_str(),mode_.c_str(),bandwidth_);
            if(!cw::host::brown){
                ImGui::SetNextItemWidth(140);
                if(ImGui::InputText("Receiver grid (optional)",grid_,sizeof(grid_),ImGuiInputTextFlags_EnterReturnsTrue))gridError_=!cw::host::setGrid(grid_,name_);
                notebook::hint("Optional Maidenhead grid for reception context. Press Enter to save. Leave blank if unknown.");
                if(gridError_)ImGui::TextColored(notebook::brass,"Use a 4, 6 or 8 character grid.");
            }
            group("CW logging");
            bool save=s.autosave;if(ImGui::Checkbox("Autosave Markdown",&save))engine_.command("autosave",save?"1":"0");
            notebook::hint("Automatically save confirmed CW text. Digital frequency tabs are held in memory; copy their reports before closing them.");
            if(ImGui::Button("Open CW log folder"))openPath(cw::host::logFolder().string(),false);
            notebook::hint("Open this host's CW log folder in Finder. Existing logs are kept.");
        }else{
            group("Receiver and decoder");
            ImGui::Text("%s %s | %s",notebook::shortName,CW_VERSION_STRING,cw::host::name);
            ImGui::Text("Glitches filtered: %llu",static_cast<unsigned long long>(s.recoveredDecoder.filteredGlitches));
            ImGui::Text("Quiet relearns: %llu | Reconnects: %llu",static_cast<unsigned long long>(s.recoveredDecoder.recoveries),static_cast<unsigned long long>(reconnects_));
            if(ImGui::Button("Reconnect CW decoder"))handleDebugCommand("reconnect","");
            notebook::hint("Reconnect only Maverick Decoder's private receive channel. Keep the host source and other modules running.");
            if(ImGui::Button("Save 20s signal"))engine_.command("capture");
            notebook::hint("Save a private diagnostic recording for offline analysis. Reception continues.");
            group("Reference data");
            ImGui::TextWrapped("Offline CTY country-prefix snapshot: 23 March 2024. Callsign country is an estimate, not a verified station location. No online lookup occurs until you click a callsign tag.");
        }
    });
}
