// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fec_monitor.h"
#include "rtty_presentation.h"
#include "signal_metrics.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>
namespace fec {
namespace {
double target(bool js8){return js8?rtty::js8Target:rtty::oliviaTarget;}
constexpr double js8Bases[]{3578000,7078000,10130000,14078000,18104000,21078000,24922000,28078000,50318000};
bool commonJs8(double f){return std::any_of(std::begin(js8Bases),std::end(js8Bases),[&](double b){return f>=b&&f<b+3000;});}
}
struct Monitor::Channel {
    uint64_t key,sequence=0,station=0;
    bool js8;
    int bin,rank;
    double frequency,strength,created,lastSeen,lastText=0,snr=0;
    std::string prefix;
    std::array<rtty::IQ,128> overlap{};
};
Monitor::Monitor()=default;Monitor::~Monitor()=default;
void Monitor::updateKeys(){if(worker_){std::vector<uint64_t> keys;for(auto& c:channels_)keys.push_back(c->key);worker_->keys(std::move(keys));}}
void Monitor::reset(){channels_.clear();signals_.clear();cooldowns_.clear();epoch_=0;limited_=0;if(worker_)worker_->reset();for(auto& s:stations_)s.active=false;}
void Monitor::modes(bool js8,bool olivia){
    if(js8==js8_&&olivia==olivia_)return;js8_=js8;olivia_=olivia;
    if(!js8&&!olivia){reset();return;}
    channels_.erase(std::remove_if(channels_.begin(),channels_.end(),[&](const auto& c){return c->js8?!js8:!olivia;}),channels_.end());
    for(auto& s:stations_)if(s.mode=="JS8"?!js8:!olivia)s.active=false;
    if(!worker_)worker_=std::make_unique<Worker>();updateKeys();
}
void Monitor::closeAll(){reset();stations_.clear();lastFrames_.clear();}
void Monitor::close(uint64_t id){
    auto it=std::find_if(stations_.begin(),stations_.end(),[&](auto& s){return s.id==id;});if(it==stations_.end())return;
    double f=it->center,shift=it->shift;
    channels_.erase(std::remove_if(channels_.begin(),channels_.end(),[&](const auto& c){return target(c->js8)==shift&&std::abs(c->frequency-f)<(c->js8?1400:200);}),channels_.end());
    stations_.erase(it);lastFrames_.erase(id);updateKeys();
}
void Monitor::selection(const rtty::Selection& s){
    selection_=s;
    stations_.erase(std::remove_if(stations_.begin(),stations_.end(),[&](const auto& v){if(!rtty::contains(s.ignored,v.center,v.shift))return false;lastFrames_.erase(v.id);return true;}),stations_.end());reset();
}
void Monitor::discover(const std::vector<float>& power,double binHz,double center,double seconds,const rtty::Focus& focus,const rtty::Selection& selection){
    if(!worker_||(!js8_&&!olivia_))return;
    selection_=selection;const int size=int(power.size()),half=size/2;
    auto at=[&](int b){return power[(b+size)%size];};
    scratch_=power;auto median=scratch_.begin()+half;std::nth_element(scratch_.begin(),median,scratch_.end());
    const double floor=std::max(1e-16f,*median);
    struct Candidate {double frequency,power;bool js8;int rank;};std::vector<Candidate> candidates;
    std::unordered_map<int64_t,size_t> buckets;
    auto add=[&](double f,double strength,bool js8){
        const double shift=target(js8);if(rtty::contains(selection.ignored,f,shift))return;
        int rank=focus.contains(f)?3:rtty::contains(selection.pinned,f,shift)?2:(js8&&commonJs8(f))?1:0;
        if(selection.onlyPinned&&!rtty::contains(selection.pinned,f,shift))return;
        double channel=js8?std::floor(f/2000)*2000+1000:f;
        if(std::abs(f-center)+(js8?150:550)>=size*binHz/2)return;
        int64_t cell=int64_t(std::llround(channel/(js8?2000:100))),key=cell*2+(js8?1:0);
        for(int d=js8?0:-1;d<=(js8?0:1);++d){auto found=buckets.find(key+2*d);
            if(found!=buckets.end()){auto& old=candidates[found->second];if(std::abs(old.frequency-channel)<(js8?100:100)){old.rank=std::max(old.rank,rank);old.power=std::max(old.power,strength);return;}}
        }
        buckets[key]=candidates.size();candidates.push_back({channel,strength,js8,rank});
    };
    // Energy proposes probes; FEC and protocol checks identify the mode.
    // Tuned/pinned/common JS8 regions also get probes below the energy gate.
    int first=0,last=0;double sum=0,weighted=0,squared=0;
    auto segment=[&]{
        // Hann skirts around a strong narrow carrier span many bins. Require
        // actual energy spread, so CW/PSK carriers do not consume Olivia slots.
        double variance=sum>0?std::max(0.,squared/sum-std::pow(weighted/sum,2)):0;
        if(olivia_&&sum>0&&(last-first+1)*binHz>=100&&variance*binHz*binHz>=625)
            add(center+weighted/sum*binHz,sum,false);
        sum=weighted=squared=0;
    };
    for(int b=-half+8;b<half-8;++b){
        double p=at(b);
        if(js8_&&p>floor*3&&p>at(b-1)&&p>=at(b+1))add(center+b*binHz,p,true);
        if(p>floor*2.5){if(sum&&b-last>3)segment();if(!sum)first=b;last=b;sum+=p;weighted+=p*b;squared+=p*b*b;}
        else if(sum&&b-last>3)segment();
    }
    segment();
    if(js8_&&!selection.onlyPinned){
        for(double f:js8Bases)if(std::abs(f+1500-center)<size*binHz/2){add(f+700,floor,true);add(f+2200,floor,true);}
        if(focus.valid())for(double f=focus.low+100;f<focus.high&&f<focus.low+8000;f+=1800)add(f,floor,true);
    }
    for(const auto& pin:selection.pinned)if((rtty::isJs8(pin.shift)&&js8_)||(pin.shift==rtty::oliviaTarget&&olivia_))add(pin.center,floor,rtty::isJs8(pin.shift));
    auto strongest=[](const auto& a,const auto& b){return a.rank!=b.rank?a.rank>b.rank:a.power>b.power;};
    limited_=candidates.size()>512?candidates.size()-512:0;
    if(candidates.size()>512){std::partial_sort(candidates.begin(),candidates.begin()+512,candidates.end(),strongest);candidates.resize(512);}else std::sort(candidates.begin(),candidates.end(),strongest);
    signals_.clear();for(auto& c:candidates)signals_.push_back({c.frequency,c.js8?rtty::js8WindowTarget:rtty::oliviaTarget,true,false,false});
    cooldowns_.erase(std::remove_if(cooldowns_.begin(),cooldowns_.end(),[&](auto& c){return c.until<=seconds;}),cooldowns_.end());
    channels_.erase(std::remove_if(channels_.begin(),channels_.end(),[&](const auto& c){
        bool expired=seconds-c->lastSeen>3||(!c->lastText&&seconds-c->created>(c->js8?65:55))||
            (c->lastText&&seconds-c->lastText>(c->js8?70:25));
        if(expired&&cooldowns_.size()<512)cooldowns_.push_back({c->frequency,seconds+5,c->js8});return expired;
    }),channels_.end());
    for(const auto& p:candidates){
        auto found=std::find_if(channels_.begin(),channels_.end(),[&](const auto& c){return c->js8==p.js8&&std::abs(c->frequency-p.frequency)<(p.js8?100:150);});
        if(found!=channels_.end()){(*found)->rank=p.rank;(*found)->lastSeen=seconds;continue;}
        if(std::any_of(cooldowns_.begin(),cooldowns_.end(),[&](auto& c){return c.js8==p.js8&&std::abs(c.frequency-p.frequency)<150;}))continue;
        const size_t maximum=p.js8?4:2;
        auto count=std::count_if(channels_.begin(),channels_.end(),[&](const auto& c){return c->js8==p.js8;});
        if(size_t(count)>=maximum){
            auto victim=channels_.end();
            for(auto it=channels_.begin();it!=channels_.end();++it)if((*it)->js8==p.js8&&p.rank>=2&&(*it)->rank<p.rank)
                if(victim==channels_.end()||(*it)->rank<(*victim)->rank)victim=it;
            if(victim!=channels_.end())channels_.erase(victim);else{++limited_;continue;}
        }
        auto c=std::make_unique<Channel>();c->key=nextKey_++;c->js8=p.js8;c->rank=p.rank;c->frequency=p.frequency;
        c->bin=int(std::lround((p.frequency-center)/binHz));c->strength=p.power;c->created=c->lastSeen=seconds;channels_.push_back(std::move(c));
    }
    for(auto& c:channels_)c->snr=radio::spectralSnr(size,binHz,(c->frequency-center)/binHz,c->js8?250:1000,[&](int b){return power[b];});
    updateKeys();
}
void Monitor::frame(const std::vector<rtty::IQ>& spectrum,double rate,double center,double seconds,size_t budget){
    if(!worker_)return;
    captureLow_=center-rate/2;captureHigh_=center+rate/2;
    if(!epoch_)epoch_=std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count()-seconds;
    const int size=int(spectrum.size());
    for(auto& c:channels_){
        auto rotation=std::polar(1.f,float(std::remainder(2*3.141592653589793*c->bin*(seconds*rate-size/2)/size,2*3.141592653589793)));
        for(int j=0;j<256;++j){int k=j<128?j:j-256,b=c->bin+k;
            double offset=std::abs(k*rate/size),pass=c->js8?1100:600,stop=c->js8?1350:750;
            float taper=offset<=pass?1:offset>=stop?0:float(.5+.5*std::cos(3.141592653589793*(offset-pass)/(stop-pass)));
            band_[j]=b<-size/2||b>=size/2?rtty::IQ{}:spectrum[(b+size)%size]*float(256)/float(size)*rotation*taper;}
        fft_.transform(band_,true);Packet packet;packet.key=c->key;packet.sequence=++c->sequence;packet.js8=c->js8;
        packet.frequency=center+c->bin*rate/size;packet.rate=rate/size*256;packet.utc=epoch_+seconds;packet.snr=c->snr;
        for(int j=0;j<128;++j){packet.samples[j]=band_[j]+c->overlap[j];c->overlap[j]=band_[j+128];}worker_->input(std::move(packet));
    }
    collect(budget);
}
void Monitor::collect(size_t budget){
    if(!worker_)return;
    const double now=rtty::monotonicSeconds();for(auto& s:stations_)s.active=(s.mode=="JS8"?js8_:olivia_)&&now-s.lastTextAt<60;
    for(auto& r:worker_->take()){
        auto channel=std::find_if(channels_.begin(),channels_.end(),[&](const auto& c){return c->key==r.key;});if(channel==channels_.end())continue;
        double frequency=r.center+r.frame.frequency-(r.js8?1500:0),shift=target(r.js8);
        if(frequency<captureLow_||frequency>=captureHigh_)continue;
        if(rtty::contains(selection_.ignored,frequency,shift)||(selection_.onlyPinned&&!rtty::contains(selection_.pinned,frequency,shift)))continue;
        auto old=std::find_if(stations_.begin(),stations_.end(),[&](const auto& s){return s.shift==shift&&std::abs(s.frequency-frequency)<(r.js8?25:150);});
        if(old==stations_.end()){
            if(stations_.size()>=budget){++limited_;continue;}
            stations_.push_back({});old=std::prev(stations_.end());old->id=nextId_++;old->mode=r.js8?"JS8":"Olivia";old->shift=shift;
        }
        if(r.js8&&!lastFrames_[old->id].accept(std::to_string(r.frame.time)+"|"+r.frame.format+"|"+r.frame.text))continue;
        old->frequency=old->center=frequency;old->format=r.frame.format;
        if(!r.frame.call.empty()){if(old->call!=r.frame.call){old->grid.clear();old->remoteReport.clear();}old->call=r.frame.call;}
        if(!r.frame.grid.empty())old->grid=r.frame.grid;if(!r.frame.remoteReport.empty())old->remoteReport=r.frame.remoteReport;
        if(r.js8){
            if(r.frame.first&&!old->text.empty()&&old->text.back()!='\n')old->text+='\n';old->text+=r.frame.text;if(r.frame.last)old->text+='\n';
        }else{
            if((*channel)->station!=old->id){(*channel)->station=old->id;(*channel)->prefix=old->text.empty()?"":old->text+"\n";}
            old->text=(*channel)->prefix+r.frame.text;
        }
        old->observeSnr(r.frame.snr,now);old->received(now);
        if(old->text.size()>40000)old->text.erase(0,old->text.size()-32000);old->active=true;(*channel)->lastText=(*channel)->lastSeen;
    }
}
void Monitor::append(rtty::Status& s) const{
    s.stations.insert(s.stations.end(),stations_.begin(),stations_.end());s.limited+=limited_;s.candidates+=signals_.size();
    for(auto& c:channels_)if(c->js8)++s.js8Decoders;else ++s.oliviaDecoders;
    for(auto signal:signals_){for(auto& c:channels_)if(c->js8==rtty::isJs8(signal.shift)&&std::abs(c->frequency-signal.center)<150){signal.decoding=true;signal.confirmed=c->lastText>0;break;}s.signals.push_back(signal);}
    if(worker_){auto stats=worker_->stats();s.fecDropped=stats.dropped;s.fecProcessingMs=stats.cpuMs;if(s.error.empty())s.error=stats.error;}
}
void Monitor::waitIdle(size_t budget){if(worker_){worker_->waitIdle();collect(budget);}}
}
