// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "rtty_engine.h"
#include <chrono>
#include <cmath>
#include <cstring>

namespace rtty {
Engine::Engine(){
    for(auto& p:pool_)p.data.resize(packetSize);
    thread_=std::thread([this]{work();});
}
Engine::~Engine(){
    {std::lock_guard<std::mutex> lock(mutex_);stop_=true;}
    wake_.notify_one();thread_.join();
}
void Engine::configure(double rate,double center,bool receiving){
    std::lock_guard<std::mutex> lock(mutex_);
    if(!receiving||rate!=rate_||center!=center_)failed_=false;
    receiving=receiving&&!failed_;
    receiving=receiving&&std::isfinite(rate)&&std::isfinite(center)&&rate>=8000&&rate<=Skimmer::maxRate&&center>0;
    if(rate==rate_&&center==center_&&receiving==receiving_)return;
    rate_=rate;center_=center;receiving_=receiving;
    read_=write_=pending_=samples_=0;++generation_;wake_.notify_one();
}
bool Engine::input(const IQ* data,size_t count){
    return inputBytes(data,count);
}
bool Engine::inputBytes(const void* data,size_t count){
    std::unique_lock<std::mutex> lock(mutex_,std::try_to_lock);
    if(!lock.owns_lock()){++dropped_;++gaps_;return false;}
    if(!receiving_)return false;
    if(count==0)return true;
    if(count>packetSize||pending_==poolSize||samples_+count>rate_*.25){++dropped_;++gaps_;return false;}
    auto& p=pool_[write_];std::memcpy(p.data.data(),data,count*sizeof(IQ));p.count=count;p.generation=generation_;p.gaps=gaps_;
    write_=(write_+1)%poolSize;++pending_;samples_+=count;wake_.notify_one();return true;
}
std::shared_ptr<const Status> Engine::snapshot() const{return std::atomic_load(&view_);}
void Engine::close(uint64_t id){std::lock_guard<std::mutex> lock(mutex_);if(closes_.size()<Skimmer::maxTabs)closes_.push_back(id);wake_.notify_one();}
void Engine::closeAll(){
    std::lock_guard<std::mutex> lock(mutex_);clear_=true;
    read_=write_=pending_=samples_=0;++generation_;wake_.notify_one();
}
void Engine::selection(Selection selection){
    std::lock_guard<std::mutex> lock(mutex_);selection_=validated(std::move(selection));wake_.notify_one();
}
void Engine::focus(Focus focus){std::lock_guard<std::mutex> lock(mutex_);focus_=focus.valid()?focus:Focus{};wake_.notify_one();}
void Engine::modes(bool rtty,bool psk){std::lock_guard<std::mutex> lock(mutex_);modes_={{rtty,psk}};wake_.notify_one();}
void Engine::fecModes(bool js8,bool olivia){std::lock_guard<std::mutex> lock(mutex_);fecModes_={{js8,olivia}};wake_.notify_one();}
void Engine::publish(const std::string& error,bool receiving){
    auto status=skimmer_.snapshot();status.dropped=dropped_;status.processingMs=processingMs_;if(!error.empty())status.error=error;
    status.receiving=receiving;
    std::atomic_store(&view_,std::shared_ptr<const Status>(std::make_shared<Status>(std::move(status))));
}
void Engine::work(){
    Packet packet;packet.data.resize(packetSize);
    uint64_t generation=0,gaps=0;
    std::string failure;
    auto published=std::chrono::steady_clock::now();
    while(true){
        double rate,center;bool receiving,reset,clear;std::vector<uint64_t> closes;std::optional<Selection> selection;std::optional<Focus> focus;std::optional<std::pair<bool,bool>> modes,fecModes;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            wake_.wait(lock,[&]{return stop_||pending_||generation_!=generation||!closes_.empty()||clear_||selection_.has_value()||focus_.has_value()||modes_.has_value()||fecModes_.has_value();});
            if(stop_)break;
            rate=rate_;center=center_;receiving=receiving_;closes.swap(closes_);clear=clear_;clear_=false;
            reset=generation_!=generation;generation=generation_;
            selection.swap(selection_);
            focus.swap(focus_);
            modes.swap(modes_);
            fecModes.swap(fecModes_);
            packet.count=0;
            if(pending_){std::swap(packet,pool_[read_]);read_=(read_+1)%poolSize;--pending_;samples_-=packet.count;}
        }
        try{
            if(reset){skimmer_.discontinuity();failure.clear();}
            if(clear)skimmer_.closeAll();
            if(selection)skimmer_.selection(std::move(*selection));
            if(focus)skimmer_.focus(*focus);
            if(modes)skimmer_.modes(modes->first,modes->second);
            if(fecModes)skimmer_.fecModes(fecModes->first,fecModes->second);
            for(auto id:closes)skimmer_.close(id);
            if(packet.count&&gaps!=packet.gaps){gaps=packet.gaps;skimmer_.discontinuity();}
            if(receiving&&packet.count&&packet.generation==generation){
                auto start=std::chrono::steady_clock::now();skimmer_.process(packet.data.data(),packet.count,rate,center);
                processingMs_+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
            }
            auto now=std::chrono::steady_clock::now();
            if(now-published>=std::chrono::milliseconds(250)||!closes.empty()||!receiving||reset||clear||selection||focus||modes||fecModes){publish(failure,receiving);published=now;}
        }catch(const std::exception& e){
            failure=e.what();skimmer_.discontinuity();publish(failure);
            std::lock_guard<std::mutex> lock(mutex_);failed_=true;receiving_=false;pending_=samples_=read_=write_=0;
        }
    }
}
}
