// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "notebook_style.h"

void CWNotebook::openNotebookModal(int modal){
    if(modal==1){cwSettingsOpen_=true;ImGui::OpenPopup("CW Settings");}
    if(modal==2){helpOpen_=true;ImGui::OpenPopup("How to use");}
    if(modal==3){signalList_.refresh();signalsOpen_=true;ImGui::OpenPopup("Digital signals");}
    if(modal==4)cwSettingsOpen_=helpOpen_=signalsOpen_=false;
}
void CWNotebook::operatingGuide(){
    ImGui::TextColored(notebook::brass,"JS8Call and Olivia with error correction");
    ImGui::TextWrapped("Enable JS8 or Olivia in the toolbar. Both default on and remember an explicit off. The whole capture stays surveyed. Four JS8 receive windows can each decode multiple streams; two Olivia candidate frequencies check seven common formats. Radio's passband and pins get priority. Unconfirmed probes rotate so false candidates cannot hold every slot forever. All digital modes share 32 retained frequency tabs; Close all tabs preserves CW's current on/off state.");
    ImGui::Spacing();ImGui::TextWrapped("JS8 supports Normal, Fast, Turbo, Slow and Ultra. It needs a complete timed frame and an accurate computer clock. Olivia supports 4/125, 4/250, 8/250, 8/500, 16/500, 16/1000 and 32/1000. Acquisition can take several FEC blocks. Signals lists JS8 receive windows; pinning one accepts every stream inside it. Pin signal in a decoded tab prioritizes that frequency. Text is held in memory; copy it before closing or quitting.");
    ImGui::Spacing();ImGui::TextWrapped("RX SNR is estimated here, referenced to 2500 Hz of noise. JS8 uses its last decoded frame; CW, RTTY, PSK and Olivia use spectral estimates. These are not calibrated signal-strength readings. Header callsigns, Maidenhead grids and remote signal reports are shown when present. A remote report is separate from local RX SNR.");
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    ImGui::TextColored(notebook::brass,"PSK31 and PSK63 across the capture");
    ImGui::TextWrapped("Enable PSK31/63 to check both BPSK rates together. Confirmed streams open frequency tabs with Follow PSK, Copy to clipboard, Explain with AI and Pin signal. Green activity lasts one minute and returning traffic restarts it. The capture, wide FFT and worker are shared with RTTY; eight PSK decoders run alongside up to 16 RTTY decoders. All digital modes share 32 retained tabs. Close all tabs preserves CW's current on/off state.");
    ImGui::Spacing();ImGui::TextWrapped("PSK inside Radio's filter gets first priority, followed by pins, common PSK windows and other captured signals. The entire capture stays monitored. Common-window preference cannot interrupt confirmed streams or take every unconfirmed slot. Signals lists PSK candidates separately from RTTY shifts; pins and ignores apply to the indicated mode. Phase, timing, Varicode and varied text must qualify a stream. Noise, idle-only carriers and short repetitive messages may stay unconfirmed. QPSK, PSK63F and extended character sets are not supported. Weak, overlapping or drifting signals can be missed or corrupted. PSK text is memory-only; copy it before closing or quitting.");
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    ImGui::TextColored(notebook::brass,"RTTY across the capture");
    ImGui::TextWrapped("Start the receiver and enable RTTY in the toolbar. The choice is remembered. CW has its own checkbox for independent reception control. Confirmed streams open frequency tabs without changing your tuning or focus. A dot marks unread text. Selecting a tab only reads it; Tune Radio is an explicit listening action.");
    ImGui::Spacing();ImGui::TextWrapped("Crowded band? Open Signals, pin the frequencies you want, or choose Decode only pinned signals. Rows stay fixed until Refresh list; their states update live. Up to 16 RTTY and eight PSK candidates decode together. Ignore & close prevents an unwanted frequency reopening; clear ignores in Signals. Pins and ignores are remembered per host. Close all tabs clears digital tabs and pending text, keeps your choices, and returns to CW if it is enabled. Fresh traffic can open new tabs.");
    ImGui::Spacing();ImGui::TextWrapped("Every eligible RTTY pair inside Radio's tuned filter is checked first, following frequency, mode and bandwidth changes automatically. Other captured frequencies keep scanning. Pins come next; ignored signals and Decode only pinned signals are still respected. Both RTTY tones must fit inside the filter for tuned priority.");
    ImGui::Spacing();ImGui::TextWrapped("The filled circle before each RTTY frequency stays green for one minute after confirmed activity, then turns yellow. Each returning burst restarts the minute. Follow RTTY scrolls to new text; turning it off does not stop decoding. Compact RY folds exact long RYRY test patterns for reading. Copy to clipboard includes the displayed view plus band, frequency, time and decoding metadata. Turn Compact RY off to copy every test letter. Hover controls and the fixed status bar for explanations.");
    ImGui::Spacing();ImGui::TextWrapped("Repeated single-letter patterns alone do not confirm RTTY. A new stream needs consistent framing and varied payload, or an alternating RY test. Short, repetitive or weak real signals may stay Checking longer. Confirmed reception is not corrected or filtered.");
    ImGui::Spacing();ImGui::TextWrapped("Tabs show estimated mark frequency rounded to 1 Hz. Signals lists pair centers before polarity is known. Receiver calibration, noise and FFT resolution limit accuracy. RTTY text stays in memory; copy it before closing or quitting. Weak, overlapping or unsupported signals may be missed; two tones alone do not prove RTTY.");
    ImGui::Spacing();ImGui::TextDisabled("45.45 / 50 / 75 / 100 baud | 170 / 200 / 425 / 450 / 850 Hz | five-bit ITA2, both polarities");
    ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
    ImGui::TextColored(notebook::brass,"CW notebook");
    ImGui::TextWrapped("Turn CW on to open its tab and start fresh timing learning. Closing the tab's X or turning CW off stops its private receiver and learning while retaining confirmed text and logs. Other digital modes continue. Select CW, USB, LSB, DSB or AM and place one Morse signal inside Radio's filter. Narrow the filter to exclude nearby signals. USB receives above the dial; LSB below; DSB and AM on either side. AM offers keyed RF carrier or AM audio tone input.");
    ImGui::Spacing();ImGui::TextWrapped("Mixed dot/dash evidence establishes timing. Provisional text can appear after about two seconds and may change; only confirmed text is autosaved. Manual WPM can interpret held intervals. Default automatic speed is 5-45 WPM; CW Settings offers manual controls and an extended range.");
    ImGui::Spacing();ImGui::TextWrapped("Changing frequency starts fresh timing and a new line. Relearn restarts detection and discards held intervals while keeping confirmed text. Original and Timing + fade recovery remain separate; changing views does not rewrite the original.");
    ImGui::Spacing();ImGui::TextWrapped("Select text and press Cmd+C, or right-click for copy options. Follow changes scrolling only. Clear view keeps the log. Clear log archives the previous file before clearing. Markdown autosave applies to CW; RTTY copying is separate.");
}
void CWNotebook::notebookModals(const cw::Snapshot& s){
    const auto* viewport=ImGui::GetMainViewport();
    const ImVec2 size{std::max(300.f,std::min(740.f,viewport->WorkSize.x-32)),std::max(280.f,std::min(600.f,viewport->WorkSize.y-32))};
    auto modal=[&](const char* title,bool& open,auto content){
        ImGui::SetNextWindowSize(size,ImGuiCond_Appearing);
        ImGui::SetNextWindowPos({viewport->WorkPos.x+viewport->WorkSize.x/2,viewport->WorkPos.y+viewport->WorkSize.y/2},ImGuiCond_Appearing,{.5f,.5f});
        if(ImGui::BeginPopupModal(title,&open,ImGuiWindowFlags_NoSavedSettings)){
            controlsPopupOpen_=true;
            ImGui::BeginChild("modal_content",{0,-ImGui::GetFrameHeightWithSpacing()-8},false);content();ImGui::EndChild();
            ImGui::Separator();if(ImGui::Button("Close",{100,0})){open=false;ImGui::CloseCurrentPopup();}
            notebook::hint("Return to the notebook. Reception continues; your choices are kept.");
            ImGui::EndPopup();
        }
    };
    modal("CW Settings",cwSettingsOpen_,[&]{decoderControls(s);});
    modal("How to use",helpOpen_,[&]{operatingGuide();});
    modal("Digital signals",signalsOpen_,[&]{signalPicker();});
}
