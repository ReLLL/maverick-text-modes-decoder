// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fec_worker.h"
#include "fec_resampler.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <limits>
#ifdef __APPLE__
#include <pthread/qos.h>
#endif
namespace fec {
struct Worker::Channel {
    uint64_t sequence=0;
    double rate=0,phase=0,energy=1e-8,lastReport=0;
    Resampler resampler;
    bool js8;
    std::unique_ptr<Olivia> olivia;
    std::vector<int16_t> ring;
    size_t write=0,filled=0;
    std::array<int64_t,5> periods{};
    std::string previousText;
    Channel(bool j,double r):rate(r),resampler(r,j?12000:8000),js8(j){
        if(j){ring.resize(720000);periods.fill(-1);}
        else olivia=std::make_unique<Olivia>();
    }
    std::vector<Frame> process(const Packet& packet,Js8* decoder){
        std::vector<double> audio;audio.reserve(packet.samples.size()*4);
        const double outputRate=js8?12000:8000,step=2*3.141592653589793*1500/outputRate;
        for(auto value:packet.samples){
            if(!std::isfinite(value.real())||!std::isfinite(value.imag()))value={};
            resampler.sample(value,[&](rtty::IQ interpolated){
                double x=(interpolated*std::polar(1.f,float(phase))).real();
                phase=std::remainder(phase+step,2*3.141592653589793);
                energy+=.0002*(x*x-energy);
                if(js8){
                    double scaled=x*std::min(1e8,3000/std::sqrt(std::max(1e-20,energy)));
                    ring[write]=int16_t(std::clamp(scaled,-30000.,30000.));write=(write+1)%ring.size();filled=std::min(filled+1,ring.size());
                }else audio.push_back(x);
            });
        }
        if(olivia){
            olivia->process(audio.data(),audio.size());
            if(packet.utc-lastReport<.25||!olivia->locked())return {};
            lastReport=packet.utc;auto text=olivia->text();if(text==previousText)return {};
            Frame f;f.text=text;f.format=olivia->format();f.frequency=olivia->offset();f.snr=packet.snr;f.time=packet.utc;
            previousText=text;return {std::move(f)};
        }
        const std::array<int,5> durations{15,10,6,30,4};int mask=0;
        for(size_t i=0;i<durations.size();++i){
            int64_t period=int64_t(packet.utc)/durations[i];
            if(period!=periods[i]){periods[i]=period;if(filled>=size_t(durations[i]*12000))mask|=1<<i;}
        }
        if(!mask)return {};
        std::vector<int16_t> chronological(ring.size());
        size_t begin=filled<ring.size()?0:write,leading=ring.size()-filled;
        for(size_t i=0;i<filled;++i)chronological[leading+i]=ring[(begin+i)%ring.size()];
        return decoder->decode(chronological,packet.utc,mask);
    }
};
Worker::Worker():thread_([this]{work();}){}
Worker::~Worker(){ {std::lock_guard<std::mutex> lock(mutex_);stop_=true;pending_=0;}wake_.notify_one();thread_.join();}
void Worker::reset(){std::lock_guard<std::mutex> lock(mutex_);++generation_;reset_=true;received_.clear();pending_=read_=write_=0;stats_.error.clear();wake_.notify_one();}
void Worker::keys(std::vector<uint64_t> keys){std::lock_guard<std::mutex> lock(mutex_);keys_=std::move(keys);keysDirty_=true;wake_.notify_one();}
void Worker::input(Packet packet){
    std::lock_guard<std::mutex> lock(mutex_);
    if(!stats_.error.empty())return;
    if(pending_==queue_.size()){++stats_.dropped;return;}
    packet.generation=generation_;queue_[write_]=packet;write_=(write_+1)%queue_.size();++pending_;wake_.notify_one();
}
std::vector<Received> Worker::take(){std::lock_guard<std::mutex> lock(mutex_);std::vector<Received> out;out.swap(received_);return out;}
Worker::Stats Worker::stats(){std::lock_guard<std::mutex> lock(mutex_);return stats_;}
void Worker::waitIdle(){std::unique_lock<std::mutex> lock(mutex_);idle_.wait(lock,[&]{return !busy_&&!pending_&&!reset_&&!keysDirty_;});}
void Worker::work(){
#ifdef __APPLE__
    pthread_set_qos_class_self_np(QOS_CLASS_UTILITY,0);
#endif
    std::map<uint64_t,std::unique_ptr<Channel>> channels;
    std::unique_ptr<Js8> js8;
    for(;;){
        Packet packet;uint64_t generation;bool hasPacket=false;
        {
            std::unique_lock<std::mutex> lock(mutex_);wake_.wait(lock,[&]{return stop_||reset_||keysDirty_||pending_;});
            if(stop_)return;
            if(reset_){channels.clear();js8.reset();reset_=false;}
            if(keysDirty_){for(auto it=channels.begin();it!=channels.end();)if(std::find(keys_.begin(),keys_.end(),it->first)==keys_.end())it=channels.erase(it);else ++it;keysDirty_=false;}
            if(pending_){packet=queue_[read_];read_=(read_+1)%queue_.size();--pending_;hasPacket=true;}
            generation=generation_;busy_=hasPacket;
            if(!hasPacket){idle_.notify_all();continue;}
            if(packet.generation!=generation||std::find(keys_.begin(),keys_.end(),packet.key)==keys_.end()){busy_=false;idle_.notify_all();continue;}
        }
        try {
            timespec started{};clock_gettime(CLOCK_THREAD_CPUTIME_ID,&started);auto& channel=channels[packet.key];
            if(!channel||packet.sequence!=channel->sequence+1||packet.rate!=channel->rate)channel=std::make_unique<Channel>(packet.js8,packet.rate);
            if(packet.js8&&!js8)js8=std::make_unique<Js8>();
            channel->sequence=packet.sequence;auto frames=channel->process(packet,js8.get());
            std::lock_guard<std::mutex> lock(mutex_);
            timespec ended{};clock_gettime(CLOCK_THREAD_CPUTIME_ID,&ended);
            stats_.cpuMs+=1000*(ended.tv_sec-started.tv_sec)+double(ended.tv_nsec-started.tv_nsec)/1e6;
            if(packet.generation==generation_)for(auto& f:frames)if(received_.size()<128)received_.push_back({packet.key,packet.js8,packet.frequency,std::move(f)});
            busy_=false;idle_.notify_all();
        }catch(const std::exception& e){
            std::lock_guard<std::mutex> lock(mutex_);stats_.error=e.what();pending_=0;busy_=false;idle_.notify_all();
        }
    }
}
}
