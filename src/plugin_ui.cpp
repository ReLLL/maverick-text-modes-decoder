// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "window_presentation.h"
#include "notebook_style.h"
#include "reception_view.h"
#include <core.h>
#include <cstdio>
#include <cmath>

namespace {
const ImVec4 live=notebook::receiving,warning=notebook::brass,muted=notebook::muted;
using notebook::hint;
}
void CWNotebook::setCWMonitoring(bool enabled){
    cwTab_.request(enabled,engine_.snapshot().pauseRevision);cwEnabledView_=enabled;
    if(enabled)selectCW_=true;
    else {accept_=false;detach();}
    engine_.command("pause",enabled?"0":"1");
    metadata();
}
void CWNotebook::sidebar(){
    auto s=engine_.snapshot();
    if(ImGui::Button(window_?"Bring CW Notebook to front":"Open CW Notebook",ImVec2(-1,0))){window_=true;presentWindow_=true;}
    hint("Show and focus the notebook window. Existing decoding and text are kept.");
    if(ImGui::BeginCombo("Receiver",streamName_.c_str())){
        for(const auto& n:sigpath::sinkManager.getStreamNames())if(ImGui::Selectable(n.c_str(),n==streamName_))select(n);
        ImGui::EndCombo();
    }
    hint("Select the Radio receiver followed by the CW decoder. RTTY monitors the whole captured spectrum independently.");
    if(s.paused)ImGui::TextUnformatted("CW off");
    else if(!supportedMode_){
        if(ImGui::Button("Use CW mode",ImVec2(-1,0))){
            cw::host::useCW(streamName_);
        }
        hint("Switch the selected Radio receiver to CW mode so its tuned Morse signal can be decoded.");
    }else if(s.decoder.calibrated){
        if(channel_.amAudio)ImGui::Text("%.1f WPM | AM tone %.0f Hz",s.decoder.wpm,s.decoder.tone);
        else ImGui::Text("%.1f WPM | %+.0f Hz",s.decoder.wpm,channel_.carrierOffset(s.decoder.tone));
    }
    else ImGui::TextUnformatted("Learning dot / dash timing");
    if(!s.error.empty())ImGui::TextColored(warning,"Open notebook: attention needed");
}
void CWNotebook::logControls(const cw::Snapshot& s){
    if(ImGui::Button("Save now"))engine_.command("save");hint("Sync pending Markdown text to disk.");
    ImGui::SameLine();if(ImGui::Button("Clear log")){engine_.command("clear_log");renaming_=false;}
    hint("Archive the current log, then clear the file, transcript and held timing data.");
    ImGui::SameLine();if(ImGui::Button("Copy to clipboard")){
        const auto& d=showRecovery_?s.recoveredDecoder:s.decoder;
        const auto text=cw::transcriptText(s,showRecovery_);
        const auto& rx=showRecovery_?s.recoveredReception:s.reception;
        transcript_.copyReport(frequency_.load(),d.wpm,channel_.carrierOffset(d.tone),s.manualWpm>0,showRecovery_,&text,rx.snr,!rx.currentSnr(playing_&&!s.paused&&d.signal),rx.lastTextUtc);
    }
    hint("Copy the displayed version with date, weekday, frequency, speed and carrier offset.");
    ImGui::SameLine();if(ImGui::Button("Explain with AI..."))ImGui::OpenPopup("notebook_ai_actions");
    hint("Choose ChatGPT, Claude or Gemini. Shares the displayed decode and reception context when you choose a site; also copies the complete prompt.");
    if(ImGui::BeginPopup("notebook_ai_actions")){
        bool hasText=!cw::transcriptText(s,showRecovery_).empty();
        if(!hasText)ImGui::TextDisabled("Receive text first.");
        if(ImGui::MenuItem("ChatGPT",nullptr,false,hasText))explainWithAI(0);
        hint("Open ChatGPT with the displayed CW decode and reception context. The full prompt is also copied for pasting.");
        if(ImGui::MenuItem("Claude",nullptr,false,hasText))explainWithAI(1);
        hint("Open Claude with the displayed CW decode and reception context. The full prompt is also copied for pasting.");
        if(ImGui::MenuItem("Gemini",nullptr,false,hasText))explainWithAI(2);
        hint("Copy the full CW analysis prompt and open Gemini. Paste the prompt there and send it yourself.");
        ImGui::Separator();ImGui::TextDisabled("Full prompt copied for every site.");
        ImGui::TextDisabled("Gemini / long transcripts: paste and send.");
        ImGui::EndPopup();
    }
    ImGui::SameLine();if(ImGui::Button("Log file..."))ImGui::OpenPopup("notebook_log_actions");
    hint("Open, reveal, rename or start a CW Markdown log. RTTY text is copied separately from its tab.");
    controlsPopupOpen_=ImGui::IsPopupOpen("notebook_log_actions")||ImGui::IsPopupOpen("notebook_ai_actions");
    if(ImGui::BeginPopup("notebook_log_actions")){
        if(ImGui::MenuItem("Open Markdown"))openPath(s.path,false);
        hint("Open the current CW log in your default Markdown application.");
        if(ImGui::MenuItem("Show in Finder"))openPath(s.path,true);
        hint("Reveal the current CW Markdown log in Finder.");
        if(ImGui::MenuItem("Rename...")){
            auto name=std::filesystem::path(s.path).filename().string();std::snprintf(filename_,sizeof(filename_),"%s",name.c_str());renaming_=true;
        }
        hint("Change this CW log's filename. An existing file is never overwritten.");
        if(ImGui::MenuItem("New log"))engine_.command("new");
        hint("Save pending CW text and start another log. The previous log is retained.");
        ImGui::EndPopup();
    }
    if(renaming_){
        ImGui::SetNextItemWidth(std::max(120.f,ImGui::GetContentRegionAvail().x-210));
        bool enter=ImGui::InputText("##filename",filename_,sizeof(filename_),ImGuiInputTextFlags_EnterReturnsTrue);
        hint("Enter a new CW log filename. Press Enter or Apply filename to rename safely.");
        ImGui::SameLine();if(ImGui::Button("Apply filename")||enter){engine_.command("rename",filename_);renaming_=false;}
        hint("Rename the current CW log. Refuses a name that would overwrite another file.");
        ImGui::SameLine();if(ImGui::Button("Cancel"))renaming_=false;
        hint("Keep the current log filename and close the rename field.");
    }
    bool save=s.autosave;if(ImGui::Checkbox("Autosave Markdown",&save))engine_.command("autosave",save?"1":"0");
    hint("Save confirmed CW text to the Markdown log automatically. This does not save RTTY tabs; copy those before closing.");
    ImGui::SameLine();ImGui::TextDisabled("%llu characters saved",static_cast<unsigned long long>(s.saved));
    auto name=std::filesystem::path(s.path).filename().string();
    ImGui::TextDisabled("%s",name.empty()?"A log is created when text arrives.":name.c_str());hint(s.path.c_str());
}
void CWNotebook::decoderControls(const cw::Snapshot& s){
    ImGui::TextColored(notebook::brass,"Timing");ImGui::Spacing();
    bool extended=s.extendedSpeed;
    if(ImGui::Checkbox("Extended automatic speed (3-200 WPM)",&extended))engine_.command("extended_speed",extended?"1":"0");
    hint("Allow unusually slow or fast CW speeds. Leave off for the normal 5-45 WPM range and more conservative acquisition.");
    ImGui::TextWrapped("Default: 5-45 WPM. Implausible estimates are rejected, not forced into letters.");
    if(ImGui::Checkbox("Manual speed",&manualSpeed_))engine_.command("wpm",manualSpeed_?std::to_string(speed_):"0");
    hint("Use the WPM value beside this control instead of estimating Morse speed. Turn off to return to automatic speed detection.");
    ImGui::SameLine();ImGui::SetNextItemWidth(180);
    if(ImGui::SliderFloat("WPM",&speed_,3,200,"%.1f")&&manualSpeed_)engine_.command("wpm",std::to_string(speed_));
    hint("Manual speed also decodes held key/gap intervals; it does not discard the buffer.");
    ImGui::Spacing();ImGui::TextColored(notebook::brass,"Tone and tuning");ImGui::Spacing();
    if(ImGui::Checkbox(channel_.amAudio?"Lock AM audio tone":"Lock carrier offset",&manualPitch_)){
        if(manualPitch_)pitch_=s.decoder.tone;
        engine_.command("tone",manualPitch_?std::to_string(pitch_):"0");
    }
    hint("Hold the selected CW tone or carrier offset instead of automatically searching for a signal. Adjust Hz beside this control; turn off to search again.");
    ImGui::SameLine();ImGui::SetNextItemWidth(180);float offset=channel_.amAudio?pitch_:channel_.carrierOffset(pitch_);
    bool centered=mode_=="DSB"||mode_=="AM";
    float low=channel_.amAudio?200:mode_=="CW"?-220:mode_=="LSB"?-bandwidth_:centered?-bandwidth_/2:0;
    float high=channel_.amAudio?std::max(200.,bandwidth_/2):mode_=="CW"?220:mode_=="LSB"?0:centered?bandwidth_/2:bandwidth_;
    if(ImGui::SliderFloat("Hz",&offset,low,high,"%+.0f")&&manualPitch_){pitch_=channel_.amAudio?offset:channel_.zeroTone+channel_.sideband*offset;engine_.command("tone",std::to_string(pitch_));}
    hint("Choose the CW carrier offset inside Radio's passband, or the AM audio pitch. Enable the tone lock checkbox to apply it.");
    bool retune=s.autoRetune;
    if(ImGui::Checkbox("Release manual locks when tuning a new station",&retune))engine_.command("auto_retune",retune?"1":"0");
    hint("Release manual speed and tone locks when tuning to another station, allowing fresh automatic acquisition.");
    if(manualSpeed_||manualPitch_){
        if(ImGui::Button("Return to automatic detection")){engine_.command("wpm","0");engine_.command("tone","0");}
        hint("Release both manual CW locks and let the decoder estimate speed and tone again.");
    }
    ImGui::Spacing();ImGui::TextColored(notebook::brass,"Recovery");ImGui::Spacing();
    bool recovery=s.timingRecovery;
    if(ImGui::Checkbox("Timing and fade repair in separate recovery version",&recovery))engine_.command("timing_recovery",recovery?"1":"0");
    hint("Produce a separate, unverified CW timing/fade recovery version for comparison. The original decode is kept and never replaced with guessed words.");
    ImGui::TextWrapped("Follows sudden fades, filters brief glitches and adapts to uneven gaps. Select Timing + fade recovery in Transcript to compare. Original text is kept; no word replacement or vocabulary guessing.");
    ImGui::TextWrapped("CW, USB, LSB, DSB and AM use Radio's full passband. One keyed signal at a time; narrow the filter to reduce interference. Speaker volume and audio AGC do not affect decoding.");
}
void CWNotebook::cwStatus(const cw::Snapshot& s){
    const auto& d=showRecovery_?s.recoveredDecoder:s.decoder;
    const char* state=s.paused?"CW off":!supportedMode_?"Select CW, USB, LSB, DSB or AM":!playing_?"Receiver stopped":ImGui::GetTime()-lastAudioTime_>2?"Waiting for audio":!d.signal?"Listening":!d.calibrated?"Learning timing":"Receiving CW";
    ImGui::TextColored(d.calibrated?live:warning,"%s",state);
    ImGui::SameLine();ImGui::Text("%.3f kHz",frequency_.load()/1000);
    ImGui::SameLine();ImGui::TextColored(d.keyed?live:muted,d.keyed?"KEY DOWN":"KEY UP");
    ImGui::SameLine();if(ImGui::Button("Relearn"))engine_.command("reset");
    hint("Restart detection. Keeps confirmed text and log, but discards held intervals. Retuning already does this automatically.");
    if(d.calibrated)ImGui::Text("%.1f WPM",d.wpm);else ImGui::TextUnformatted("Learning WPM");
    notebook::flow(280);ImGui::TextDisabled("%s | %s | %.0f Hz filter",manualSpeed_?"Manual":s.extendedSpeed?"Auto 3-200":"Auto 5-45",mode_.c_str(),bandwidth_);
    notebook::flow(130);
    if(channel_.amAudio)ImGui::TextDisabled("AM tone %.0f Hz",d.tone);
    else ImGui::TextDisabled("Carrier %+.0f Hz",channel_.carrierOffset(d.tone));
    notebook::flow(180);
    const auto& rx=showRecovery_?s.recoveredReception:s.reception;
    notebook::rxSnr(rx,playing_&&!s.paused&&d.signal);
    notebook::lastReceived(rx);
    if(mode_=="AM"){
        ImGui::SetNextItemWidth(220);int input=s.amAudio?1:0;
        if(ImGui::Combo("AM input",&input,"Keyed RF carrier\0AM audio tones\0"))engine_.command("am_audio",input?"1":"0");
        hint("Keyed RF carrier: ordinary on/off CW. AM audio tones: Morse tones modulating an AM carrier. Switching relearns timing; confirmed text stays.");
    }
    if(!d.calibrated&&d.bufferedRuns)
        ImGui::TextColored(warning,"%llu intervals held | provisional text updates while learning",static_cast<unsigned long long>(d.bufferedRuns));
}
void CWNotebook::draw(){
    const int cwRequest=pendingCWEnable_.exchange(-1);if(cwRequest>=0)setCWMonitoring(cwRequest==1);
    const auto cwSnapshot=engine_.snapshot();cwTab_.sync(cwSnapshot.paused,cwSnapshot.pauseRevision);cwEnabledView_=cwTab_.enabled();
    const int request=pendingRttyEnable_.exchange(-1);
    const int pskRequest=pendingPskEnable_.exchange(-1);if(pskRequest>=0)setPskMonitoring(pskRequest==1);
    const int js8Request=pendingJs8Enable_.exchange(-1);if(js8Request>=0)setFecMonitoring(true,js8Request==1);
    const int oliviaRequest=pendingOliviaEnable_.exchange(-1);if(oliviaRequest>=0)setFecMonitoring(false,oliviaRequest==1);
    if(request>=0)setRttyMonitoring(request!=0);
    const double listen=pendingRttyListen_.exchange(0);if(listen>0)listenRtty(listen);
    if(pendingRttyCloseAll_.exchange(false)&&rtty_){rtty_->closeAll();selectCW_=true;}
    if(++frame_%5==0)metadata();
    if(rtty_)std::atomic_store(&rttyStatus_,rtty_->snapshot());
    if(!enabled_||!window_){notebook::closePopups();cwSettingsOpen_=helpOpen_=signalsOpen_=false;return;}
    auto s=engine_.snapshot();
    if(s.transcriptReset!=lastTranscriptReset_){transcript_.clear();follow_=true;lastTranscriptReset_=s.transcriptReset;}
    manualSpeed_=s.manualWpm>0;manualPitch_=s.manualTone>0;
    if(manualSpeed_)speed_=s.manualWpm;
    if(manualPitch_)pitch_=s.manualTone;
    if(s.blocks!=lastBlocks_){lastBlocks_=s.blocks;lastAudioTime_=ImGui::GetTime();}
    notebook::Style visualStyle;
    prepareNotebookWindow(presentWindow_);ImGui::SetNextWindowBgAlpha(1);
    if(!ImGui::Begin("CW Notebook###cw_notebook_window",&window_,ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){ImGui::End();return;}
    windowMin_=ImGui::GetWindowPos();auto size=ImGui::GetWindowSize();windowMax_={windowMin_.x+size.x,windowMin_.y+size.y};
    const int modal=pendingModal_.exchange(0);if(modal)openNotebookModal(modal);
    controlsPopupOpen_=false;transcript_.hidden();
    for(auto& pair:rttyViews_)pair.second.transcript.hidden();
    rttyControls();
    ImGui::Separator();
    ImGui::BeginChild("notebook_content",{0,std::max(40.f,ImGui::GetContentRegionAvail().y-notebook::statusHeight()+ImGui::GetStyle().WindowPadding.y)},false);
    const float footer=ImGui::GetFrameHeightWithSpacing()*3+ImGui::GetTextLineHeightWithSpacing()+12+(renaming_?ImGui::GetFrameHeightWithSpacing():0);
    if(ImGui::BeginTabBar("notebook_pages",ImGuiTabBarFlags_Reorderable|ImGuiTabBarFlags_FittingPolicyScroll)){
        bool cwClosed=false;
        bool cwVisible=cwTab_.begin(selectCW_,cwClosed);selectCW_=false;
        if(cwClosed)setCWMonitoring(false);
        if(cwVisible){
            cwStatus(s);ImGui::Separator();
            ImGui::SetNextItemWidth(std::min(290.f,ImGui::GetContentRegionAvail().x*.4f));int version=showRecovery_?1:0;
            if(ImGui::Combo("##transcript_version",&version,"Original decode\0Timing + fade recovery (unverified)\0")){showRecovery_=version==1;transcript_.clear();follow_=true;}
            hint("Both versions are retained. Switching never rewrites the original.");
            notebook::flow(85);ImGui::Checkbox("Follow",&follow_);hint("Scroll to new text. Selection holds scrolling; decoding continues.");
            notebook::flow(160);ImGui::SetNextItemWidth(90);ImGui::SliderFloat("Text size",&textSize_,.9f,1.7f,"%.1fx");
            hint("Change transcript text size. Applies to CW and RTTY text without changing receiver decoding.");
            notebook::flow("Clear view");if(ImGui::Button("Clear view"))engine_.command("clear_view");hint("Clear displayed text only. The log is retained.");
            const auto text=cw::transcriptText(s,showRecovery_);
            callTags(transcript_,text);
            if(text.empty())ImGui::TextDisabled("Tune one Morse signal in CW, USB, LSB, DSB or AM. Text appears as timing becomes clear.");
            transcript_.draw(text,std::max(80.f,ImGui::GetContentRegionAvail().y-footer-24),textSize_,follow_,textOptions_,[this](const auto& code){return reinterpret_cast<ImTextureID>(uintptr_t(flags_.get(code)));});
            ImGui::Separator();logControls(s);
            ImGui::EndTabItem();
        }
        rttyTabs(0);
        ImGui::EndTabBar();
    }
    if(!cwTab_.open()&&rttyStatus_->stations.empty())
        ImGui::TextDisabled("CW is off. Enable CW above to reopen its tab. Digital tabs open when text is received.");
    ImGui::EndChild();
    notebookModals(s);
    receiveStatus(s);
    ImGui::End();
}
