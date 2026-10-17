// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "notebook_style.h"
#include <core.h>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <imgui_internal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;
SDRPP_MOD_INFO{"cw_notebook","CW notebook with parallel RTTY and PSK monitoring","ReLLL",CW_VERSION_MAJOR,CW_VERSION_MINOR,CW_VERSION_PATCH,1};
CWNotebook::CWNotebook(std::string name):name_(std::move(name)),engine_(cw::host::logFolder()){
    auto grid=cw::host::grid(name_);std::snprintf(grid_,sizeof(grid_),"%s",grid.c_str());
    gui::menu.registerEntry(name_,[](void* ctx){static_cast<CWNotebook*>(ctx)->sidebar();},this,this);
#if CW_HOST_BROWN
    drawHandler_={[](ImGuiContext*,void* ctx){static_cast<CWNotebook*>(ctx)->draw();},this};
    gui::mainWindow.onWaterfallDrawn.bindHandler(&drawHandler_);
#else
    // SDR++ has no Brown draw event. A context hook runs even with its sidebar hidden.
    ImGuiContextHook hook;hook.Type=ImGuiContextHookType_EndFramePre;hook.UserData=this;
    hook.Callback=[](ImGuiContext*,ImGuiContextHook* h){static_cast<CWNotebook*>(h->UserData)->draw();};
    drawHook_=ImGui::AddContextHook(ImGui::GetCurrentContext(),&hook);
#endif
    inputHandler_={[](ImGui::WaterFall::InputHandlerArgs,void* ctx){
        auto* self=static_cast<CWNotebook*>(ctx);
        auto p=ImGui::GetMousePos();
        if(!self->enabled_ || !self->window_){self->ownsMouseGesture_=false;return;}
        const bool inside=p.x>=self->windowMin_.x && p.x<=self->windowMax_.x && p.y>=self->windowMin_.y && p.y<=self->windowMax_.y;
        const bool clicked=ImGui::IsMouseClicked(0)||ImGui::IsMouseClicked(1)||ImGui::IsMouseClicked(2);
        const bool down=ImGui::IsMouseDown(0)||ImGui::IsMouseDown(1)||ImGui::IsMouseDown(2);
        auto* window=ImGui::FindWindowByName("CW Notebook###cw_notebook_window");
        auto* active=GImGui->ActiveIdWindow;
        const bool focusedControl=active && window && active->RootWindow==window && ImGui::GetActiveID()!=0;
        if(clicked && (inside || self->transcript_.popupOpen() || self->controlsPopupOpen_))self->ownsMouseGesture_=true;
        // Keep the complete gesture, even after leaving the window. A popup's
        // dismissal click and text-selection keys must not reach the waterfall.
        if(inside || self->ownsMouseGesture_ || self->transcript_.popupOpen() || self->controlsPopupOpen_ || (focusedControl && !clicked)){
            gui::waterfall.inputHandled=true;
            gui::mainWindow.lockWaterfallControls=true;
        }
        if(!down)self->ownsMouseGesture_=false;
    },this};
    gui::waterfall.onInputProcess.bindHandler(&inputHandler_);
    registered_={[](std::string name,void* ctx){auto* self=static_cast<CWNotebook*>(ctx);if(self->enabled_ && !self->audioStream_ && name==self->streamName_)self->select(name);},this};
    unregistering_={[](std::string name,void* ctx){auto* self=static_cast<CWNotebook*>(ctx);if(name==self->streamName_)self->detach();},this};
    sigpath::sinkManager.onStreamRegistered.bindHandler(&registered_);
    sigpath::sinkManager.onStreamUnregister.bindHandler(&unregistering_);
    sink_.init(nullptr,audio,this);
}
CWNotebook::~CWNotebook(){
    accept_=false;
    notebook::closePopups();
    rtty_.reset();
#if CW_HOST_BROWN
    gui::mainWindow.onWaterfallDrawn.unbindHandler(&drawHandler_);
#else
    ImGui::RemoveContextHook(ImGui::GetCurrentContext(),drawHook_);
#endif
    gui::waterfall.onInputProcess.unbindHandler(&inputHandler_);
    sigpath::sinkManager.onStreamRegistered.unbindHandler(&registered_);
    sigpath::sinkManager.onStreamUnregister.unbindHandler(&unregistering_);
    detach();gui::menu.removeEntry(name_);
}
void CWNotebook::postInit(){
    rttySelection_=cw::host::rttySelection(name_);
    if(enabled_){select(streamName_);metadata();}
    rttyEnabled_=cw::host::rttyEnabled(name_);pskEnabled_=cw::host::pskEnabled(name_);
    js8Enabled_=cw::host::fecEnabled(name_,true);oliviaEnabled_=cw::host::fecEnabled(name_,false);syncDigitalMonitoring();
}
void CWNotebook::enable(){enabled_=true;window_=true;presentWindow_=true;select(streamName_);if(rtty_&&digitalEnabled())rtty_->enabled(true);metadata();}
void CWNotebook::disable(){enabled_=false;accept_=false;pendingModal_=4;if(rtty_)rtty_->enabled(false);detach();engine_.command("save");}
void CWNotebook::detach(){
    if(!audioStream_)return;
    sink_.stop();sigpath::iqFrontEnd.removeVFO("cw_notebook_channel");audioStream_=nullptr;mixer_.reset();
    engine_.command("reset");
}
void CWNotebook::select(const std::string& name){
    detach();streamName_=name;
    if(!enabled_||!cwTab_.enabled()||engine_.snapshot().paused)return;
    auto names=sigpath::sinkManager.getStreamNames();
    if(std::find(names.begin(),names.end(),name)==names.end()||!sigpath::vfoManager.vfoExists(name))return;
    channelOffset_=channel_.center;
    audioStream_=sigpath::iqFrontEnd.addVFO("cw_notebook_channel",sampleRate_,bandwidth_,channelOffset_);
    if(audioStream_){sink_.setInput(&audioStream_->out);sink_.start();}
}
void CWNotebook::metadata(){
    const auto cw=engine_.snapshot();
    playing_=gui::mainWindow.isPlaying();
    double frequency=gui::waterfall.getCenterFrequency();
    const bool hasVfo=sigpath::vfoManager.vfoExists(streamName_);
    double offset=hasVfo?sigpath::vfoManager.getOffset(streamName_):0;
    if(hasVfo)frequency+=offset;
    else frequency=double(gui::freqSelect.frequency);
    double center=gui::waterfall.getCenterFrequency(),rate=sigpath::iqFrontEnd.getSampleRate();
    if(rtty_)rtty_->configure(rate,center,enabled_&&digitalEnabled()&&playing_);
    bool changed=lastFrequency_!=0&&(std::abs(frequency-lastFrequency_)>.5||center!=lastCenter_||rate!=lastRate_);
    if(changed){engine_.command("retune",std::to_string(frequency));watchdog_.grace(ImGui::GetTime());}
    lastFrequency_=frequency;lastCenter_=center;lastRate_=rate;frequency_=frequency;
    std::string mode=cw::host::mode(streamName_);
    const double width=hasVfo?sigpath::vfoManager.getBandwidth(streamName_):500;
    if(rtty_)rtty_->focus(hasVfo?rtty::radioFocus(mode,frequency,width,center,rate):rtty::Focus{});
    auto plan=cw::receiveChannel(mode,offset,width,cw.amAudio);
    supportedMode_=plan.supported;followsBandwidth_=plan.supported;
    bool modeChanged=mode!=mode_;
    bool layoutChanged=modeChanged||std::abs(plan.width-bandwidth_)>.1||plan.amAudio!=channel_.amAudio;
    if(layoutChanged){
        accept_=false;detach();
        {std::lock_guard<std::mutex> lock(channelMutex_);channel_=plan;}
        bandwidth_=plan.width;sampleRate_=plan.sampleRate;mode_=mode;
        if(modeChanged)engine_.command("input_mode",plan.wide?"1":"0");
        if(enabled_)select(streamName_);
        watchdog_.grace(ImGui::GetTime());
    }
    {std::lock_guard<std::mutex> lock(channelMutex_);channel_.center=plan.center;}
    if(audioStream_&&std::abs(plan.center-channelOffset_)>.1){audioStream_->setOffset(plan.center);channelOffset_=plan.center;}
    const bool cwEnabled=enabled_&&cwTab_.enabled()&&!cw.paused;
    if(!cwEnabled&&audioStream_){accept_=false;detach();}
    else if(cwEnabled&&!audioStream_)select(streamName_);
    bool ready=cwEnabled && supportedMode_ && playing_ && hasVfo;
    if(ready!=accept_)engine_.command("reset");
    accept_=ready;
    if(watchdog_.check(ImGui::GetTime(),ready,cw.paused,inputBlocks_.load())){
        // Reconnect only this receive channel, never the source or another module.
        accept_=false;select(streamName_);accept_=ready;++reconnects_;
    }
}
void CWNotebook::audio(dsp::complex_t* samples,int count,void* ctx){
    auto* self=static_cast<CWNotebook*>(ctx);
    ++self->inputBlocks_;
    if(!self->accept_ || count<=0 || count>1000000)return;
    cw::ReceiveChannel plan;
    {std::lock_guard<std::mutex> lock(self->channelMutex_);plan=self->channel_;}
    std::vector<float> mono(size_t(count),0.f);
    for(int i=0;i<count;i++){
        mono[i]=self->mixer_.sample(samples[i].re,samples[i].im,plan);
    }
    self->engine_.audio(std::move(mono),self->sampleRate_,self->frequency_);
}
void CWNotebook::openPath(const std::string& path,bool reveal){
    if(path.empty())return;
    pid_t pid;std::vector<char*> args{const_cast<char*>("/usr/bin/open")};
    if(reveal)args.push_back(const_cast<char*>("-R"));
    args.push_back(const_cast<char*>(path.c_str()));args.push_back(nullptr);
    if(posix_spawn(&pid,"/usr/bin/open",nullptr,nullptr,args.data(),environ)==0){
        // open exits quickly; reap without blocking the UI.
        std::thread([pid]{int status;while(waitpid(pid,&status,0)<0 && errno==EINTR){}}).detach();
    }
}
std::string CWNotebook::handleDebugCommand(const std::string& cmd,const std::string& args){
    if(cmd=="pause"){pendingCWEnable_=args=="1"?0:1;return "{\"status\":\"queued\"}";}
    if(cmd=="rtty_listen"){
        try{
            size_t end=0;double mark=args.size()<=64?std::stod(args,&end):0;
            if(end!=args.size()||!std::isfinite(mark)||mark<=2125||mark>=1e12)return "{\"error\":\"Expected a positive mark frequency in Hz\"}";
            pendingRttyListen_=mark;return "{\"status\":\"queued\"}";
        }catch(const std::exception&){return "{\"error\":\"Expected a positive mark frequency in Hz\"}";}
    }
    if(cmd=="show_settings"||cmd=="show_help"||cmd=="show_signals"||cmd=="close_modal"){
        pendingModal_=cmd=="show_settings"?1:cmd=="show_help"?2:cmd=="show_signals"?3:4;
        return "{\"status\":\"queued\"}";
    }
    if(cmd=="rtty_enable"){
        pendingRttyEnable_=args=="1"?1:0;return "{\"status\":\"queued\"}";
    }
    if(cmd=="psk_enable"){pendingPskEnable_=args=="1"?1:0;return "{\"status\":\"queued\"}";}
    if(cmd=="js8_enable"){pendingJs8Enable_=args=="1"?1:0;return "{\"status\":\"queued\"}";}
    if(cmd=="olivia_enable"){pendingOliviaEnable_=args=="1"?1:0;return "{\"status\":\"queued\"}";}
    if(cmd=="rtty_close_all"){pendingRttyCloseAll_=true;return "{\"status\":\"queued\"}";}
    if(cmd=="rtty_status"||cmd=="digital_status"){
        auto s=std::atomic_load(&rttyStatus_);
        json stations=json::array(),signals=json::array();
        for(const auto& v:s->signals)signals.push_back({{"center",v.center},{"shift",v.shift},{"available",v.available},{"decoding",v.decoding},{"confirmed",v.confirmed}});
        for(const auto& v:s->stations){
            const auto country=v.mode=="JS8"?radio::callsignCountry(v.call):radio::Country{};
            stations.push_back({{"id",v.id},{"frequency",v.frequency},{"center",v.center},{"mode",rtty::modeName(v)},{"format",v.format},{"header_call",v.call},{"header_grid",v.grid},{"remote_report",v.remoteReport},{"baud",v.baud},{"shift",v.shift},{"reverse",v.reverse},{"active",v.active},{"rx_snr_db",v.snr},{"snr_current",v.currentSnr(v.active)},{"snr_measured_at",v.snrAt},{"country",country.name},{"country_flag",country.flag},{"snr_reference_hz",2500},{"text",v.text},{"last_text_at",v.lastTextAt},{"last_text_utc",v.lastTextUtc}});
        }
        return json{{"enabled",rttyEnabledView_.load()},{"psk_enabled",pskEnabledView_.load()},{"js8_enabled",js8EnabledView_.load()},{"olivia_enabled",oliviaEnabledView_.load()},{"receiving",s->receiving},{"priority_low",s->focus.low},{"priority_high",s->focus.high},{"priority_decoders",s->focusedDecoders},{"rate",s->rate},{"center",s->center},{"decoders",s->decoders},{"psk_decoders",s->pskDecoders},{"js8_windows",s->js8Decoders},{"olivia_decoders",s->oliviaDecoders},{"fec_dropped",s->fecDropped},{"fec_processing_ms",s->fecProcessingMs},{"candidates",s->candidates},{"limited",s->limited},{"dropped",s->dropped},{"processing_ms",s->processingMs},{"error",s->error},{"stations",stations},{"signals",signals}}.dump();
    }
    if(cmd=="status"){
        auto s=engine_.snapshot();return json{{"text",s.text},{"recovered_text",s.recoveredText},
            {"timing_recovery",s.timingRecovery},{"auto_retune",s.autoRetune},{"reconnects",reconnects_},
            {"input_blocks",inputBlocks_.load()},{"recovery_resets",s.recoveredDecoder.recoveries},
            {"filtered_glitches",s.recoveredDecoder.filteredGlitches},{"path",s.path},{"error",s.error},{"notice",s.notice},{"frequency",s.frequency},
            {"wpm",s.decoder.wpm},{"tone",s.decoder.tone},{"calibrated",s.decoder.calibrated},{"signal",s.decoder.signal},{"rx_snr_db",s.reception.snr},{"snr_current",s.reception.currentSnr(playing_&&!s.paused&&s.decoder.signal)},{"snr_measured_at",s.reception.snrAt},{"last_text_at",s.reception.lastTextAt},{"last_text_utc",s.reception.lastTextUtc},{"snr_reference_hz",2500},
            {"buffered_runs",s.decoder.bufferedRuns},{"recovery_buffered_runs",s.recoveredDecoder.bufferedRuns},
            {"extended_speed",s.extendedSpeed},
            {"provisional",s.decoder.provisional},{"provisional_wpm",s.decoder.provisionalWpm},
            {"bandwidth_hz",bandwidth_},{"follows_radio_bandwidth",followsBandwidth_},{"mode",mode_},
            {"channel_offset_hz",channelOffset_},{"sample_rate",sampleRate_.load()},
            {"carrier_offset_hz",channel_.carrierOffset(s.decoder.tone)},{"supported_mode",supportedMode_},
            {"blocks",s.blocks},{"dropped",s.dropped},{"saved",s.saved},{"paused",s.paused},{"pause_revision",s.pauseRevision},{"cw_enabled",cwEnabledView_.load()},{"autosave",s.autosave},
            {"level",s.decoder.level},{"threshold",s.decoder.threshold},{"contrast_db",s.decoder.snr},{"input","IQ"},{"version",CW_VERSION_STRING},{"host",cw::host::name},
            {"manual_wpm",s.manualWpm},{"manual_tone",s.manualTone},{"am_audio",s.amAudio}}.dump();
    }
    if(cmd=="reconnect"){accept_=false;select(streamName_);watchdog_.grace(ImGui::GetTime());++reconnects_;metadata();return "{\"status\":\"reconnected\"}";}
    if(cmd=="capture"||cmd=="save"||cmd=="new"||cmd=="rename"||cmd=="clear_log"||cmd=="clear_view"||cmd=="pause"||cmd=="autosave"||cmd=="reset"||cmd=="wpm"||cmd=="tone"||cmd=="timing_recovery"||cmd=="auto_retune"||cmd=="extended_speed"||cmd=="am_audio"){
        engine_.command(cmd,args);return "{\"status\":\"queued\"}";
    }
    return "{\"error\":\"Unknown command\"}";
}
MOD_EXPORT void _INIT_(){}
MOD_EXPORT ModuleManager::Instance* _CREATE_INSTANCE_(std::string name){return new CWNotebook(std::move(name));}
MOD_EXPORT void _DELETE_INSTANCE_(ModuleManager::Instance* instance){delete instance;}
MOD_EXPORT void _END_(){}
