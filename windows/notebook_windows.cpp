// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "file_platform.h"
#include <windows.h>
#include <shellapi.h>
#include <imgui_internal.h>
#include <cstdio>
#include <cmath>
CWNotebook::CWNotebook(std::string name):name_(std::move(name)),engine_(cw::host::logFolder()){
    rtty_=std::make_unique<RttyReceiver>();
    auto value=cw::host::grid(name_);std::snprintf(grid_,sizeof(grid_),"%s",value.c_str());
}
CWNotebook::~CWNotebook(){accept_=false;notebook::closePopups();rtty_.reset();engine_.command("save");}
void CWNotebook::postInit(){
    textOptions_=cw::host::textOptions(name_);rttySelection_=cw::host::rttySelection(name_);
    rttyEnabled_=cw::host::rttyEnabled(name_);pskEnabled_=cw::host::pskEnabled(name_);
    js8Enabled_=cw::host::fecEnabled(name_,true);oliviaEnabled_=cw::host::fecEnabled(name_,false);
    syncDigitalMonitoring();metadata();
}
void CWNotebook::enable(){enabled_=true;window_=true;presentWindow_=true;metadata();}
void CWNotebook::disable(){enabled_=false;accept_=false;if(rtty_)rtty_->enabled(false);engine_.command("save");metadata();}
void CWNotebook::select(const std::string& name){streamName_=name;engine_.command("reset");}
void CWNotebook::detach(){accept_=false;engine_.command("reset");}
void CWNotebook::metadata(){
    const auto host=notebook::sharpHost().state();const auto cw=engine_.snapshot();
    playing_=host.playing;frequency_=host.frequency;
    const bool changed=lastFrequency_>0&&(std::abs(host.frequency-lastFrequency_)>.5||host.center!=lastCenter_||host.rate!=lastRate_);
    if(changed)engine_.command("retune",std::to_string(host.frequency));
    lastFrequency_=host.frequency;lastCenter_=host.center;lastRate_=host.rate;
    auto plan=cw::receiveChannel(host.mode,host.frequency-host.center,host.width,cw.amAudio);
    supportedMode_=plan.supported&&plan.center-plan.width/2>=-host.rate/2&&plan.center+plan.width/2<=host.rate/2;
    if(mode_!=host.mode){engine_.command("input_mode",plan.wide?"1":"0");mode_=host.mode;}
    {std::lock_guard<std::mutex> lock(channelMutex_);channel_=plan;}
    bandwidth_=plan.width;sampleRate_=plan.sampleRate;followsBandwidth_=plan.supported;
    const bool ready=enabled_&&cwTab_.enabled()&&!cw.paused&&supportedMode_&&playing_;
    if(ready!=accept_)engine_.command("reset");accept_=ready;
    if(rtty_){
        rtty_->configure(host.rate,host.center,enabled_&&digitalEnabled()&&playing_);
        rtty_->focus(rtty::radioFocus(host.mode,host.frequency,host.width,host.center,host.rate));
    }
    notebook::Json result={{"cw",ready},{"digital",enabled_&&digitalEnabled()&&host.rate>=8000&&host.rate<=rtty::Skimmer::maxRate},
        {"cw_plan",{{"center",host.center+plan.center},{"dial",host.frequency},{"rate",plan.sampleRate},{"width",plan.width},{"filter_width",host.width},{"mode",host.mode},{"am_audio",cw.amAudio}}}};
    notebook::sharpHost().plan(std::move(result));
}
void CWNotebook::receiveWindows(notebook::SharpBlock block){
    const auto kind=block.header.at("kind").get<std::string>();
    if(kind=="reset"){
        const auto channel=block.header.value("channel",std::string("all"));
        if(channel!="digital"){engine_.command("reset");mixer_.reset();sharpStarted_[0]=false;}
        if(channel!="cw"&&rtty_){rtty_->resetWindows();sharpStarted_[1]=false;}
        return;
    }
    const bool digital=block.header.at("channel")=="digital";const int index=digital?1:0;
    const uint64_t sequence=block.header.at("sequence").get<uint64_t>();
    const bool gap=sharpStarted_[index]&&sequence!=sharpSequences_[index]+1;
    sharpStarted_[index]=true;sharpSequences_[index]=sequence;
    double rate=block.header.at("sample_rate"),center=block.header.at("center"),frequency=block.header.at("frequency");
    if(digital){
        if(rtty_){if(gap)rtty_->resetWindows();rtty_->inputWindows(block.iq.data(),block.iq.size(),rate,center);}
    }else{
        if(!accept_||rate<8000||rate>64000)return;
        auto plan=cw::receiveChannel(block.header.at("mode").get<std::string>(),0,block.header.at("bandwidth"),block.header.value("am_audio",false));
        if(!plan.supported||std::abs(rate-plan.sampleRate)>.01)return;
        if(gap){engine_.command("reset");mixer_.reset();}
        std::vector<float> mono(block.iq.size());
        for(size_t i=0;i<mono.size();++i)mono[i]=mixer_.sample(block.iq[i].real(),block.iq[i].imag(),plan);
        ++inputBlocks_;engine_.audio(std::move(mono),int(rate),frequency);
    }
}
void CWNotebook::openPath(const std::string& text,bool reveal){
    if(text.empty())return;
    auto path=std::filesystem::u8path(text);
    if(reveal){const auto args=L"/select,\""+path.wstring()+L"\"";ShellExecuteW(nullptr,L"open",L"explorer.exe",args.c_str(),nullptr,SW_SHOWNORMAL);}
    else ShellExecuteW(nullptr,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
}
std::string CWNotebook::handleDebugCommand(const std::string& command,const std::string& value){
    if(command=="pause"){pendingCWEnable_=value=="1"?0:1;return "queued";}
    if(command=="reconnect"){engine_.command("reset");return "queued";}
    return "Unsupported command";
}
