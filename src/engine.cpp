// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>

namespace cw {
std::string transcriptText(const Snapshot& s,bool recovered){
    auto text=recovered?s.recoveredText:s.text;
    const auto& d=recovered?s.recoveredDecoder:s.decoder;
    if(!d.provisional.empty()){
        if(!text.empty())text+="\n\n";
        text+="[PROVISIONAL - learning, about "+std::to_string(int(std::lround(d.provisionalWpm)))+" WPM; may change]\n";
        text+=d.provisional;
    }
    return text;
}
Engine::Engine(const std::filesystem::path& folder):journal_(folder),folder_(folder){
    decoder_.recovery(false);
    decoder_.output=[this](const std::string& s){decoded(s);};
    recoveredDecoder_.output=[this](const std::string& s){decoded(s,true);};
    thread_=std::thread([this]{work();});
}
Engine::~Engine(){stop_=true;cv_.notify_one();if(thread_.joinable())thread_.join();}
void Engine::audio(std::vector<float> samples,int rate,double frequency){
    if(rate<4000||rate>384000||!std::isfinite(frequency)||frequency<=0||samples.empty())return;
    std::lock_guard<std::mutex> lock(queueMutex_);
    if(samples.size()>size_t(rate)*2){dropped_++;generation_++;return;}
    if(!queue_.empty() && (std::abs(queue_.back().frequency-frequency)>.5||queue_.back().rate!=rate)){
        queue_.clear();queuedSamples_=0;generation_++;
    }
    if(queuedSamples_+samples.size()>size_t(rate)*2){queue_.clear();queuedSamples_=0;dropped_++;generation_++;}
    queuedSamples_+=samples.size();
    queue_.push_back({std::move(samples),rate,frequency,generation_.load()});cv_.notify_one();
}
void Engine::command(std::string name,std::string value){
    std::lock_guard<std::mutex> lock(queueMutex_);
    if(commands_.size()<64)commands_.push_back({std::move(name),std::move(value)});
    cv_.notify_one();
}
Snapshot Engine::snapshot(){std::lock_guard<std::mutex> lock(viewMutex_);return view_;}
void Engine::resetDecoders(){
    decoder_.wideInput(wideInput_);recoveredDecoder_.wideInput(wideInput_);
    decoder_.reset(manualWpm_,manualTone_);
    recoveredDecoder_.recovery(timingRecovery_);
    recoveredDecoder_.reset(manualWpm_,manualTone_);
    decoder_.extendedSpeed(extendedSpeed_);recoveredDecoder_.extendedSpeed(extendedSpeed_);
}
void Engine::stationChanged(double frequency){
    if(frequency_>0&&std::abs(frequency-frequency_)>.5){
        reception_={};recoveredReception_={};
        transcriptBreakPending_=recoveredBreakPending_=true;
        if(autoRetune_){manualWpm_=manualTone_=0;preferencesDirty_=true;}
    }
}
void Engine::decoded(const std::string& text,bool recovered){
    if(text.find_first_not_of(" \r\n\t")!=std::string::npos)(recovered?recoveredReception_:reception_).received();
    bool& breakPending=recovered?recoveredBreakPending_:transcriptBreakPending_;
    if(text==" "){
        std::lock_guard<std::mutex> lock(viewMutex_);
        auto& target=recovered?view_.recoveredText:view_.text;
        if(breakPending || target.empty() || target.back()==' ' || target.back()=='\n')return;
    }
    std::string formatted=text;
    {
        std::lock_guard<std::mutex> lock(viewMutex_);
        auto& target=recovered?view_.recoveredText:view_.text;
        if(breakPending){
            // A reset is not received text. Separate runs only once new text arrives.
            auto end=target.find_last_not_of(" \n");
            if(end!=std::string::npos){target.resize(end+1);target+='\n';}
            breakPending=false;
        }
        auto end=target.find_last_not_of(" \n");
        if(text=="<AR>" || text=="<SK>" || (text=="=" && end!=std::string::npos && target[end]=='='))formatted+="\n\n";
    }
    if(pending_.empty() || pending_.back().frequency!=frequency_)pending_.push_back({"",frequency_,decoder_.stats().wpm,""});
    auto& segment=pending_.back();
    (recovered?segment.recovered:segment.text)+=formatted;unsavedBytes_+=formatted.size();
    // Retain unsaved data on I/O failure, with a visible bounded-memory limit.
    if(unsavedBytes_>2*1024*1024){paused_=true;std::lock_guard<std::mutex> l(viewMutex_);view_.error="Unsaved text reached 2 MB. Decoding paused; save to recover.";}
    std::lock_guard<std::mutex> lock(viewMutex_);
    auto& target=recovered?view_.recoveredText:view_.text;
    target+=formatted;
    if(target.size()>100000)target.erase(0,target.size()-80000);
}
void Engine::flush(){
    if(pending_.empty())return;
    if(journal_.path().empty())journal_.create(frequency_);
    while(!pending_.empty()){
        const auto& segment=pending_.front();
        std::string record="Original decode: "+segment.text;
        if(!segment.recovered.empty()&&segment.recovered!=segment.text)
            record+="\nTiming + fade recovery (unverified): "+segment.recovered;
        record+='\n';
        journal_.append(record,segment.frequency,segment.wpm);
        saved_+=segment.text.size();unsavedBytes_-=segment.text.size()+segment.recovered.size();pending_.pop_front();
    }
    persist();
}
void Engine::persist(){
    if(journal_.path().empty())return;
    auto tmp=folder_/".last-log.tmp";
    {std::ofstream out(tmp);out.exceptions(std::ios::badbit|std::ios::failbit);out<<journal_.path().filename().string();}
    std::filesystem::rename(tmp,folder_/".last-log");
}
void Engine::preferences(){
    std::filesystem::create_directories(folder_);
    auto temp=folder_/".decoder-settings.tmp";
    // Keep the retired vocabulary slot as zero for old preference compatibility.
    {std::ofstream out(temp);out.exceptions(std::ios::badbit|std::ios::failbit);out<<autoSave_<<' '<<manualWpm_<<' '<<manualTone_<<' '<<timingRecovery_<<" 0 "<<autoRetune_<<' '<<extendedSpeed_<<' '<<amAudio_<<'\n';}
    std::filesystem::rename(temp,folder_/".decoder-settings");
}
void Engine::apply(const Command& c){
    if(c.name=="capture"){
        if(diagnostic_.is_open())diagnostic_.close();
        diagnostic_.clear();std::filesystem::create_directories(folder_);
        std::filesystem::create_directories(folder_/"Diagnostics");
        auto path=folder_/"Diagnostics"/("diagnostic-"+utc("%Y%m%d-%H%M%SZ")+".f32");
        if(std::filesystem::exists(path))throw std::runtime_error("Diagnostic file already exists");
        diagnostic_.open(path,std::ios::binary);diagnostic_.exceptions(std::ios::badbit|std::ios::failbit);
        diagnosticRemaining_=size_t(rate_>0?rate_:8000)*20;
        std::lock_guard<std::mutex> lock(viewMutex_);view_.notice="20-second diagnostic capture: "+path.string();
    }
    else if(c.name=="input_mode"||c.name=="am_audio"){
        reception_={};recoveredReception_={};
        if(c.name=="am_audio")amAudio_=c.value=="1";
        else wideInput_=c.value=="1";
        if(autoRetune_){manualWpm_=manualTone_=0;preferencesDirty_=true;}
        resetDecoders();transcriptBreakPending_=recoveredBreakPending_=true;
        {std::lock_guard<std::mutex> lock(queueMutex_);queue_.clear();queuedSamples_=0;}
        discardCurrentPacket_=true;
    }
    else if(c.name=="pause") {paused_=c.value=="1";++pauseRevision_;resetDecoders();if(autoSave_)flush();}
    else if(c.name=="autosave") {autoSave_=c.value=="1";if(autoSave_)flush();}
    else if(c.name=="save") {flush();if(journal_.path().empty())journal_.create(frequency_);persist();}
    else if(c.name=="reset"||c.name=="retune"){
        if(c.name=="retune"){
            double frequency=std::stod(c.value);
            if(!std::isfinite(frequency)||frequency<=0)throw std::runtime_error("Invalid receive frequency");
            stationChanged(frequency);frequency_=frequency;transcriptBreakPending_=recoveredBreakPending_=true;
        }
        resetDecoders();
        {std::lock_guard<std::mutex> lock(queueMutex_);queue_.clear();queuedSamples_=0;}
        discardCurrentPacket_=true;
    }
    else if(c.name=="clear_view"){reception_.lastTextAt=reception_.lastTextUtc=recoveredReception_.lastTextAt=recoveredReception_.lastTextUtc=0;std::lock_guard<std::mutex> lock(viewMutex_);view_.text.clear();view_.recoveredText.clear();++view_.transcriptReset;}
    else if(c.name=="rename"){flush();if(journal_.path().empty())journal_.create(frequency_);journal_.rename(c.value);persist();}
    else if(c.name=="new"){flush();journal_.create(frequency_);persist();saved_=0;}
    else if(c.name=="clear_log"){
        flush();if(journal_.path().empty())journal_.create(frequency_);
        auto backup=journal_.clear();saved_=0;
        reception_.lastTextAt=reception_.lastTextUtc=recoveredReception_.lastTextAt=recoveredReception_.lastTextUtc=0;
        resetDecoders();transcriptBreakPending_=recoveredBreakPending_=false;
        {std::lock_guard<std::mutex> lock(queueMutex_);queue_.clear();queuedSamples_=0;}
        discardCurrentPacket_=true;
        std::lock_guard<std::mutex> lock(viewMutex_);
        view_.text.clear();view_.recoveredText.clear();++view_.transcriptReset;
        view_.notice="Log and decoded text cleared. Previous log archived as "+backup.filename().string();
    }
    else if(c.name=="wpm" || c.name=="tone"){
        float v=std::stof(c.value);
        if(!std::isfinite(v) || (v!=0 && (c.name=="wpm"?(v<3||v>200):(v<200||v>(wideInput_?31000:1800)))))throw std::runtime_error("Setting outside supported range");
        if(c.name=="wpm"){
            manualWpm_=v;decoder_.setSpeed(v);recoveredDecoder_.setSpeed(v);
        }else{manualTone_=v;reception_={};recoveredReception_={};resetDecoders();}
    }
    else if(c.name=="extended_speed"){
        extendedSpeed_=c.value=="1";
        decoder_.extendedSpeed(extendedSpeed_);recoveredDecoder_.extendedSpeed(extendedSpeed_);
    }
    else if(c.name=="timing_recovery"||c.name=="auto_retune"){
        if(c.name=="timing_recovery"){
            timingRecovery_=c.value=="1";recoveredDecoder_.recovery(timingRecovery_);recoveredDecoder_.reset(manualWpm_,manualTone_);recoveredBreakPending_=true;
        }
        else autoRetune_=c.value=="1";
    }
    if(c.name=="autosave"||c.name=="wpm"||c.name=="tone"||c.name=="timing_recovery"||c.name=="auto_retune"||c.name=="extended_speed"||c.name=="am_audio")preferences();
    {std::lock_guard<std::mutex> lock(viewMutex_);view_.error.clear();}
}
void Engine::work(){
    auto lastFlush=std::chrono::steady_clock::now();
    try{
        std::ifstream prefs(folder_/".decoder-settings");float wpm=0,tone=0;bool save=true;
        if(prefs>>save>>wpm>>tone && std::isfinite(wpm)&&std::isfinite(tone) &&
           (wpm==0||(wpm>=3&&wpm<=200)) && (tone==0||(tone>=200&&tone<=31000))){
            manualWpm_=wpm;manualTone_=tone;autoSave_=save;
            bool recovery=true,retired=false;if(prefs>>recovery>>retired)timingRecovery_=recovery;
            bool retune=true;if(prefs>>retune)autoRetune_=retune;
            bool extended=false;if(prefs>>extended)extendedSpeed_=extended;
            bool amAudio=false;if(prefs>>amAudio)amAudio_=amAudio;
            resetDecoders();
        }
        std::ifstream in(folder_/".last-log");std::string name;
        if(std::getline(in,name) && !name.empty())journal_.resume(name);
    }catch(const std::exception& e){std::lock_guard<std::mutex> l(viewMutex_);view_.error=e.what();}
    while(!stop_){
        Packet p{};std::deque<Command> commands;
        {std::unique_lock<std::mutex> lock(queueMutex_);cv_.wait_for(lock,std::chrono::milliseconds(100),[this]{return stop_||!queue_.empty()||!commands_.empty();});
            commands.swap(commands_);
            if(!queue_.empty()){p=std::move(queue_.front());queue_.pop_front();queuedSamples_-=p.samples.size();}
        }
        try{
            for(const auto& c:commands)apply(c);
            if(discardCurrentPacket_){p.samples.clear();discardCurrentPacket_=false;}
            if(!p.samples.empty()){
                if(diagnosticRemaining_){
                    try{
                        auto n=std::min(diagnosticRemaining_,p.samples.size());
                        diagnostic_.write(reinterpret_cast<const char*>(p.samples.data()),n*sizeof(float));diagnosticRemaining_-=n;
                        if(!diagnosticRemaining_)diagnostic_.close();
                    }catch(const std::exception& e){
                        diagnosticRemaining_=0;diagnostic_.exceptions(std::ios::goodbit);diagnostic_.close();
                        std::lock_guard<std::mutex> lock(viewMutex_);view_.error=std::string("Signal capture stopped: ")+e.what();
                    }
                }
                if(p.rate!=rate_ || std::abs(p.frequency-frequency_)>.5 || p.generation!=lastGeneration_){
                    if(frequency_!=0)transcriptBreakPending_=recoveredBreakPending_=true;
                    stationChanged(p.frequency);
                    resetDecoders();frequency_=p.frequency;rate_=p.rate;lastGeneration_=p.generation;
                }
                if(!paused_){decoder_.process(p.samples.data(),p.samples.size(),p.rate);recoveredDecoder_.process(p.samples.data(),p.samples.size(),p.rate);}
                blocks_++;
            }
            auto now=std::chrono::steady_clock::now();
            if(now-lastFlush>std::chrono::seconds(1)){
                lastFlush=now;
                if(preferencesDirty_){preferences();preferencesDirty_=false;}
                if(autoSave_)flush();
            }
        }catch(const std::exception& e){std::lock_guard<std::mutex> lock(viewMutex_);view_.error=e.what();}
        const auto original=decoder_.stats(),recovered=recoveredDecoder_.stats();
        if(original.reception.snrAt>reception_.snrAt)reception_.observeSnr(original.reception.snr,original.reception.snrAt);
        if(recovered.reception.snrAt>recoveredReception_.snrAt)recoveredReception_.observeSnr(recovered.reception.snr,recovered.reception.snrAt);
        {std::lock_guard<std::mutex> lock(viewMutex_);view_.decoder=original;view_.frequency=frequency_;view_.blocks=blocks_;view_.dropped=dropped_;
            view_.reception=reception_;view_.recoveredReception=recoveredReception_;
            view_.path=journal_.path().string();view_.saved=saved_;view_.paused=paused_;view_.pauseRevision=pauseRevision_;view_.autosave=autoSave_;
            view_.manualWpm=manualWpm_;view_.manualTone=manualTone_;}
        {std::lock_guard<std::mutex> lock(viewMutex_);view_.recoveredDecoder=recovered;view_.timingRecovery=timingRecovery_;view_.autoRetune=autoRetune_;view_.extendedSpeed=extendedSpeed_;view_.amAudio=amAudio_;}
    }
    try{if(preferencesDirty_)preferences();flush();}catch(...){ /* Previous successfully synced rows remain recoverable. */ }
}
}
