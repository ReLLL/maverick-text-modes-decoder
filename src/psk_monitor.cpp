// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "psk_monitor.h"
#include "psk_priority.h"
#include "signal_metrics.h"
#include "rtty_presentation.h"
#include <algorithm>
#include <cmath>
namespace psk {
struct Monitor::Channel {
    int bin;
    double frequency,observed,strength,created,lastSeen,lastDecoded=0,lastReport=-1;
    unsigned characters=0;
    uint64_t id=0;
    bool locked=false;
    double snr=std::numeric_limits<double>::quiet_NaN();
    std::string prefix;
    Decoder decoder;
    std::vector<rtty::IQ> overlap=std::vector<rtty::IQ>(64);
    Channel(int b,double f,double p,double seconds,double rate,double residual):
        bin(b),frequency(f),observed(f),strength(p),created(seconds),lastSeen(seconds),decoder(rate,residual){}
};
Monitor::Monitor()=default;
Monitor::~Monitor()=default;
void Monitor::reset(){channels_.clear();signals_.clear();cooldowns_.clear();candidates_=limited_=0;for(auto& s:stations_)s.active=false;}
void Monitor::closeAll(){reset();stations_.clear();}
void Monitor::close(uint64_t id){
    channels_.erase(std::remove_if(channels_.begin(),channels_.end(),[&](const auto& c){return c->id==id;}),channels_.end());
    stations_.erase(std::remove_if(stations_.begin(),stations_.end(),[&](const auto& s){return s.id==id;}),stations_.end());
}
void Monitor::selection(const rtty::Selection& selection){
    auto excluded=[&](double f){return rtty::contains(selection.ignored,f,0)||(selection.onlyPinned&&!rtty::contains(selection.pinned,f,0));};
    channels_.erase(std::remove_if(channels_.begin(),channels_.end(),[&](const auto& c){return excluded(c->frequency);}),channels_.end());
    stations_.erase(std::remove_if(stations_.begin(),stations_.end(),[&](const auto& s){return rtty::contains(selection.ignored,s.center,0);}),stations_.end());
    cooldowns_.clear();
}
void Monitor::discover(const std::vector<float>& power,double binHz,double center,double seconds,const rtty::Focus& focus,const rtty::Selection& selection){
    const int size=int(power.size()),half=size/2;
    auto at=[&](int b){return power[(b+size)%size];};
    scratch_=power;auto mid=scratch_.begin()+half;std::nth_element(scratch_.begin(),mid,scratch_.end());
    float floor=std::max(1e-16f,*mid)*15,localFloor=floor;
    if(focus.valid()){
        scratch_.clear();for(int b=-half+4;b<half-4;++b)if(focus.contains(center+b*binHz))scratch_.push_back(at(b));
        if(scratch_.size()>=8){auto m=scratch_.begin()+scratch_.size()/2;std::nth_element(scratch_.begin(),m,scratch_.end());localFloor=std::max(1e-16f,*m)*15;}
    }
    struct Candidate {double frequency,power;};std::vector<Candidate> candidates;
    // Local energy includes both idle sidebands. Relative shoulders keep clean
    // captures' tiny spectral sidelobes from merging into one broad island.
    for(int b=-half+7;b<half-7;++b){
        const float threshold=focus.contains(center+b*binHz)?localFloor:floor;
        if(at(b)<threshold||at(b)<at(b-1)||at(b)<=at(b+1))continue;
        double total=0,weighted=0;
        for(int j=b-4;j<=b+4;++j){total+=at(j);weighted+=at(j)*j;}
        if(total>0){
            double bin=weighted/total,f=center+bin*binHz;
            if(at(b-6)+at(b+6)<total*.08)candidates.push_back({f,total});
        }
    }
    auto priority=[&](double f){return psk::priority(f,focus,selection);};
    auto strongest=[&](const auto& a,const auto& b){return priority(a.frequency)!=priority(b.frequency)?priority(a.frequency)>priority(b.frequency):a.power>b.power;};
    if(candidates.size()>256){std::partial_sort(candidates.begin(),candidates.begin()+256,candidates.end(),strongest);candidates.resize(256);}
    else std::sort(candidates.begin(),candidates.end(),strongest);
    std::vector<Candidate> distinct;
    for(const auto& c:candidates)if(!std::any_of(distinct.begin(),distinct.end(),[&](const auto& d){return std::abs(c.frequency-d.frequency)<75;})){
        distinct.push_back(c);if(distinct.size()==256)break;
    }
    candidates=std::move(distinct);
    candidates_=candidates.size();limited_=0;if(candidates.size()>256)candidates.resize(256);
    signals_.clear();for(const auto& c:candidates)signals_.push_back({c.frequency,0,true,false,false});
    cooldowns_.erase(std::remove_if(cooldowns_.begin(),cooldowns_.end(),[&](const auto& c){return c.until<=seconds;}),cooldowns_.end());
    channels_.erase(std::remove_if(channels_.begin(),channels_.end(),[&](const auto& c){
        bool expired=seconds-c->lastSeen>2||seconds-(c->id?c->lastDecoded:c->created)>(c->id?6:10);
        if(expired&&priority(c->observed)<2&&cooldowns_.size()<256)cooldowns_.push_back({c->observed,seconds+3});return expired;
    }),channels_.end());
    for(const auto& p:candidates){
        if(rtty::contains(selection.ignored,p.frequency,0)||(selection.onlyPinned&&!rtty::contains(selection.pinned,p.frequency,0)))continue;
        auto old=std::find_if(channels_.begin(),channels_.end(),[&](const auto& c){return std::abs(c->frequency-p.frequency)<55;});
        if(old!=channels_.end()){
            if(!(*old)->id&&std::abs((*old)->frequency-p.frequency)>3){
                int bin=int(std::lround((p.frequency-center)/binHz));double created=(*old)->created;
                *old=std::make_unique<Channel>(bin,p.frequency,p.power,seconds,binHz*128,p.frequency-center-bin*binHz);(*old)->created=created;
            }
            (*old)->observed=p.frequency;(*old)->lastSeen=seconds;(*old)->strength=p.power;
            (*old)->snr=radio::spectralSnr(size,binHz,(p.frequency-center)/binHz,100,[&](int b){return power[b];});continue;
        }
        const int rank=priority(p.frequency);
        if(rank<2&&std::any_of(cooldowns_.begin(),cooldowns_.end(),[&](const auto& c){return std::abs(c.frequency-p.frequency)<40;}))continue;
        if(channels_.size()>=maxDecoders){
            const size_t ordinary=std::count_if(channels_.begin(),channels_.end(),[&](const auto& c){return priority(c->observed)==0;});
            auto victim=channels_.end();
            for(auto it=channels_.begin();it!=channels_.end();++it)
                if(mayReplace(rank,priority((*it)->observed),(*it)->id!=0,ordinary,p.power,(*it)->strength))
                    if(victim==channels_.end()||priority((*it)->observed)<priority((*victim)->observed)||
                        (priority((*it)->observed)==priority((*victim)->observed)&&(*it)->strength<(*victim)->strength))victim=it;
            if(victim!=channels_.end())channels_.erase(victim);
        }
        if(channels_.size()>=maxDecoders){++limited_;continue;}
        int bin=int(std::lround((p.frequency-center)/binHz));
        channels_.push_back(std::make_unique<Channel>(bin,p.frequency,p.power,seconds,binHz*128,p.frequency-center-bin*binHz));
        channels_.back()->snr=radio::spectralSnr(size,binHz,(p.frequency-center)/binHz,100,[&](int b){return power[b];});
    }
}
void Monitor::frame(const std::vector<rtty::IQ>& spectrum,double rate,double center,double seconds,size_t tabBudget){
    const int size=int(spectrum.size());
    for(auto& s:stations_)s.active=false;
    for(auto& ptr:channels_){auto& c=*ptr;
        double phase=2*3.141592653589793*c.bin*(seconds*rate-size/2)/size;
        auto rotation=std::polar(1.f,float(std::remainder(phase,2*3.141592653589793)));
        for(int j=0;j<128;++j){int k=j<64?j:j-128,b=c.bin+k;
            band_[j]=b<-size/2||b>=size/2?rtty::IQ{}:spectrum[(b+size)%size]*float(128)/float(size)*rotation;}
        fft_.transform(band_,true);
        for(size_t j=0;j<64;++j){auto next=band_[j]+c.overlap[j];c.overlap[j]=band_[j+64];band_[j]=next;}
        c.decoder.process(band_.data(),64);
        if(c.id){auto old=std::find_if(stations_.begin(),stations_.end(),[&](const auto& s){return s.id==c.id;});
            if(old!=stations_.end())old->active=c.locked&&seconds-c.lastDecoded<2;}
        if(seconds-c.lastReport<.125)continue;c.lastReport=seconds;
        auto result=c.decoder.result();c.locked=result.locked;if(!result.locked)continue;
        bool fresh=result.symbols>c.characters;
        if(fresh){c.characters=result.symbols;c.lastDecoded=seconds;}
        if(!c.id){
            auto old=std::find_if(stations_.begin(),stations_.end(),[&](const auto& s){return !s.active&&std::abs(s.center-c.frequency)<35&&s.baud==result.baud;});
            if(old!=stations_.end()){c.id=old->id;c.prefix=old->text+"\n";}
            else {if(stations_.size()>=tabBudget){++limited_;continue;}c.id=nextId_++;stations_.push_back({});stations_.back().id=c.id;}
        }
        auto s=std::find_if(stations_.begin(),stations_.end(),[&](const auto& v){return v.id==c.id;});
        if(s==stations_.end())continue;
        s->frequency=s->center=c.frequency+result.offset;s->baud=result.baud;s->psk=true;s->shift=0;
        s->observeSnr(c.snr);
        s->active=seconds-c.lastDecoded<2;auto text=c.prefix+result.text;
        if(text.size()>40000)text.erase(0,text.size()-32000);
        if(fresh)s->received();s->text=std::move(text);
    }
    (void)center;
}
void Monitor::append(rtty::Status& status) const{
    status.stations.insert(status.stations.end(),stations_.begin(),stations_.end());
    status.pskDecoders=channels_.size();status.candidates+=candidates_;status.limited+=limited_;
    for(auto signal:signals_){
        for(const auto& c:channels_)if(std::abs(c->frequency-signal.center)<35){signal.decoding=true;signal.confirmed=c->id!=0;break;}
        status.signals.push_back(signal);
    }
    for(const auto& c:channels_)if(status.focus.contains(c->observed,80))++status.focusedDecoders;
}
}
