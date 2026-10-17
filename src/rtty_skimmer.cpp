// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "rtty_skimmer.h"
#include "rtty_presentation.h"
#include "psk_monitor.h"
#include "signal_metrics.h"
#include "fec_monitor.h"
#include <algorithm>
#include <cmath>

namespace rtty {
struct Skimmer::Channel {
    int bin=0;
    double frequency=0,shift=0,lastSeen=0;
    float strength=0;
    uint64_t id=0;
    double created=0;
    double lastDecoded=0;
    unsigned lastFrames=0;
    std::string prefix;
    Decoder decoder;
    std::vector<IQ> overlap=std::vector<IQ>(channelSize/2);
    Channel(int b,double f,double s,float power,double seen,double rate,double residual):
        bin(b),frequency(f),shift(s),lastSeen(seen),strength(power),created(seen),decoder(rate,residual,s){}
};
Skimmer::Skimmer():band_(channelSize),psk_(std::make_unique<psk::Monitor>()),fec_(std::make_unique<fec::Monitor>()){}
Skimmer::~Skimmer()=default;
void Skimmer::configure(double rate,double center){
    discontinuity();rate_=rate;center_=center;size_=256;
    while(size_<rate/31.25)size_*=2;
    binHz_=rate/size_;fft_=std::make_unique<FFT>(size_);
    input_.assign(size_,{});spectrum_.resize(size_);window_.resize(size_);
    power_.assign(size_,0);noiseScratch_.resize(size_);
    for(size_t i=0;i<size_;++i)window_[i]=.5f-.5f*std::cos(2*3.141592653589793*i/size_);
}
void Skimmer::discontinuity(){
    psk_->reset();
    fec_->reset();
    channels_.clear();cooldowns_.clear();signals_.clear();filled_=0;lastDiscovery_=-1;candidates_=limited_=0;
    std::fill(power_.begin(),power_.end(),0);
    for(auto& s:stations_)s.active=false;
}
void Skimmer::process(const IQ* samples,size_t count,double rate,double center){
    if(!std::isfinite(rate)||!std::isfinite(center)||rate<8000||rate>maxRate||center<=0)return;
    if(rate!=rate_||center!=center_)configure(rate,center);
    ++blocks_;
    for(size_t i=0;i<count;){
        size_t n=std::min(count-i,size_-filled_);
        std::copy_n(samples+i,n,input_.begin()+filled_);filled_+=n;i+=n;
        if(filled_==size_){
            seconds_+=double(size_/2)/rate_;frame();
            std::copy(input_.begin()+size_/2,input_.end(),input_.begin());filled_=size_/2;
        }
    }
}
void Skimmer::frame(){
    for(size_t i=0;i<size_;++i)spectrum_[i]=input_[i]*window_[i];
    fft_->transform(spectrum_);
    const float alpha=1-std::exp(-double(size_/2)/rate_/.18);
    const float scale=1.f/(float(size_)*size_);
    for(size_t i=0;i<size_;++i){float p=std::norm(spectrum_[i])*scale;if(!std::isfinite(p))p=0;power_[i]+=alpha*(p-power_[i]);}
    if(seconds_-lastDiscovery_>=.125){
        if(rttyEnabled_)discover();
        if(pskEnabled_)psk_->discover(power_,binHz_,center_,seconds_,focus_,selection_);
        if(js8Enabled_||oliviaEnabled_)fec_->discover(power_,binHz_,center_,seconds_,focus_,selection_);
        lastDiscovery_=seconds_;
    }
    for(auto& ptr:channels_){
        auto& c=*ptr;
        // Integer-bin translation leaves sample phase continuous across hops
        // after correcting the (-1)^bin phase of each half-window advance.
        const double phase=2*3.141592653589793*c.bin*(seconds_*rate_-size_/2)/size_;
        const IQ rotation=std::polar(1.f,float(std::remainder(phase,2*3.141592653589793)));
        for(size_t j=0;j<channelSize;++j){
            int k=j<channelSize/2?int(j):int(j)-int(channelSize);
            int signedBin=c.bin+k;
            size_t source=(signedBin+int(size_))%size_;
            band_[j]=signedBin<-int(size_/2)||signedBin>=int(size_/2)?IQ{}:
                spectrum_[source]*float(channelSize)/float(size_)*rotation;
        }
        channelFFT_.transform(band_,true);
        for(size_t j=0;j<channelSize/2;++j){
            IQ next=band_[j]+c.overlap[j];c.overlap[j]=band_[j+channelSize/2];band_[j]=next;
        }
        c.decoder.process(band_.data(),channelSize/2);
    }
    if(seconds_-lastDiscovery_<double(size_/2)/rate_)updateStations();
    if(pskEnabled_)psk_->frame(spectrum_,rate_,center_,seconds_,maxTabs-stations_.size()-fec_->tabs());
    if(js8Enabled_||oliviaEnabled_)fec_->frame(spectrum_,rate_,center_,seconds_,maxTabs-stations_.size()-psk_->tabs());
}
void Skimmer::discover(){
    noiseScratch_=power_;
    auto mid=noiseScratch_.begin()+size_/2;std::nth_element(noiseScratch_.begin(),mid,noiseScratch_.end());
    const float floor=std::max(1e-16f,*mid)*12;
    float focusFloor=floor;
    if(focus_.valid()){
        noiseScratch_.clear();
        for(int b=-int(size_/2)+2;b<int(size_/2)-2;++b)
            if(focus_.contains(center_+b*binHz_))noiseScratch_.push_back(power_[(b+int(size_))%size_]);
        if(noiseScratch_.size()>=8){
            auto middle=noiseScratch_.begin()+noiseScratch_.size()/2;
            std::nth_element(noiseScratch_.begin(),middle,noiseScratch_.end());focusFloor=std::max(1e-16f,*middle)*12;
        }
    }
    struct Peak {int bin;double offset;float power;};
    std::vector<Peak> peaks;
    for(int b=-int(size_/2)+2;b<int(size_/2)-2;++b){
        size_t i=(b+int(size_))%size_;
        float p=power_[i],left=power_[(i+size_-1)%size_],right=power_[(i+1)%size_];
        if(p<(focus_.contains(center_+b*binHz_)?focusFloor:floor)||p<=left||p<right)continue;
        // Reject broad humps; FSK needs narrow peaks above a local baseline.
        float local=std::min(power_[(i+size_-8)%size_],power_[(i+8)%size_]);
        if(p<std::max(1e-16f,local)*6)continue;
        double a=std::log(std::max(1e-20f,left)),z=std::log(std::max(1e-20f,p)),c=std::log(std::max(1e-20f,right));
        double denominator=a-2*z+c;
        double fraction=std::abs(denominator)>1e-12?std::clamp(.5*(a-c)/denominator,-.5,.5):0;
        peaks.push_back({b,(b+fraction)*binHz_,p});
    }
    // Bound pairing work even in heavily occupied or adversarial spectra.
    if(peaks.size()>256){
        auto localEnd=std::partition(peaks.begin(),peaks.end(),[&](const auto& p){return focus_.contains(center_+p.offset);});
        const auto reserved=std::min(size_t(64),size_t(localEnd-peaks.begin()));
        auto strongest=[](const Peak& a,const Peak& b){return a.power>b.power;};
        std::partial_sort(peaks.begin(),peaks.begin()+reserved,localEnd,strongest);
        std::partial_sort(peaks.begin()+reserved,peaks.begin()+256,peaks.end(),strongest);peaks.resize(256);
    }
    std::sort(peaks.begin(),peaks.end(),[](const Peak& a,const Peak& b){return a.offset<b.offset;});
    struct Pair {double center,shift;float strength;};std::vector<Pair> pairs;
    for(size_t i=0;i<peaks.size();++i)for(size_t j=i+1;j<peaks.size();++j){
        double separation=peaks[j].offset-peaks[i].offset;if(separation>900)break;
        double selected=supportedShifts.front();
        for(double shift:supportedShifts)if(std::abs(separation-shift)<std::abs(separation-selected))selected=shift;
        if(std::abs(separation-selected)>std::max(12.,binHz_*.8))continue;
        if(std::max(peaks[i].power,peaks[j].power)>std::min(peaks[i].power,peaks[j].power)*20)continue;
        pairs.push_back({(peaks[i].offset+peaks[j].offset)/2,selected,std::min(peaks[i].power,peaks[j].power)});
    }
    auto priority=[&](double frequency,double shift){return focus_.contains(frequency,shift)?2:contains(selection_.pinned,frequency,shift)?1:0;};
    std::sort(pairs.begin(),pairs.end(),[&](const Pair& a,const Pair& b){
        const bool af=focus_.contains(center_+a.center,a.shift),bf=focus_.contains(center_+b.center,b.shift);
        return af!=bf?af>bf:a.strength>b.strength;
    });
    // Greedy occupied-band suppression rejects the modulation sidebands of a
    // stronger pair before they can consume decoders or create duplicate tabs.
    std::vector<Pair> distinct;
    for(const auto& p:pairs){
        bool overlap=std::any_of(distinct.begin(),distinct.end(),[&](const Pair& other){return std::abs(p.center-other.center)<(p.shift+other.shift)/2+60;});
        if(!overlap)distinct.push_back(p);
    }
    candidates_=distinct.size();limited_=0;
    signals_.clear();
    for(const auto& p:distinct)signals_.push_back({center_+p.center,p.shift,true,false,false});
    std::stable_sort(distinct.begin(),distinct.end(),[&](const Pair& a,const Pair& b){
        return priority(center_+a.center,a.shift)>priority(center_+b.center,b.shift);
    });
    // Unframed candidates rotate after four seconds. Confirmed tabs stay readable.
    cooldowns_.erase(std::remove_if(cooldowns_.begin(),cooldowns_.end(),[&](const auto& c){return c.until<=seconds_;}),cooldowns_.end());
    channels_.erase(std::remove_if(channels_.begin(),channels_.end(),[&](const auto& c){
        bool stalled=c->id?seconds_-c->lastDecoded>6:seconds_-c->created>4;
        if(stalled&&!priority(c->frequency,c->shift)&&cooldowns_.size()<256)cooldowns_.push_back({c->frequency,c->shift,seconds_+4});
        return seconds_-c->lastSeen>2||stalled;
    }),channels_.end());
    for(const auto& p:distinct){
        const bool pinned=contains(selection_.pinned,center_+p.center,p.shift);
        const int rank=priority(center_+p.center,p.shift);
        if(contains(selection_.ignored,center_+p.center,p.shift)||(selection_.onlyPinned&&!pinned))continue;
        auto found=std::find_if(channels_.begin(),channels_.end(),[&](const auto& c){return std::abs(c->frequency-center_-p.center)<std::max(45.,binHz_*2)&&std::abs(c->shift-p.shift)<30;});
        if(found!=channels_.end()){(*found)->lastSeen=seconds_;(*found)->strength=p.strength;continue;}
        if(!rank&&std::any_of(cooldowns_.begin(),cooldowns_.end(),[&](const auto& c){return std::abs(c.frequency-center_-p.center)<45&&std::abs(c.shift-p.shift)<30;}))continue;
        if(channels_.size()>=maxDecoders&&rank){
            auto victim=channels_.end();
            for(auto it=channels_.begin();it!=channels_.end();++it){
                const int current=priority((*it)->frequency,(*it)->shift);
                if(current>=rank)continue;
                if(victim==channels_.end()||current<priority((*victim)->frequency,(*victim)->shift)||
                   (current==priority((*victim)->frequency,(*victim)->shift)&&(*it)->strength<(*victim)->strength))victim=it;
            }
            if(victim!=channels_.end())channels_.erase(victim);
        }
        if(channels_.size()>=maxDecoders&&!rank){
            // Early modulation sidebands can fill the budget before both real
            // tones appear. Let a substantially stronger pair replace only an
            // unconfirmed, unpinned candidate; retain confirmed reception.
            auto victim=channels_.end();
            for(auto it=channels_.begin();it!=channels_.end();++it)
                if(!(*it)->id&&!priority((*it)->frequency,(*it)->shift)&&
                   (victim==channels_.end()||(*it)->strength<(*victim)->strength))victim=it;
            if(victim!=channels_.end()&&p.strength>(*victim)->strength*4)channels_.erase(victim);
        }
        if(channels_.size()>=maxDecoders){++limited_;continue;}
        int bin=int(std::lround(p.center/binHz_));
        channels_.push_back(std::make_unique<Channel>(bin,center_+p.center,p.shift,p.strength,seconds_,binHz_*channelSize,p.center-bin*binHz_));
    }
}
void Skimmer::updateStations(){
    for(auto& s:stations_)s.active=false;
    for(auto& c:channels_){
        auto result=c->decoder.result();if(!result.locked)continue;
        const bool received=result.frames>c->lastFrames;
        if(received){c->lastFrames=result.frames;c->lastDecoded=seconds_;}
        if(!c->id){
            // Reuse only an inactive tab at the same measured pair frequency.
            auto old=std::find_if(stations_.begin(),stations_.end(),[&](const auto& s){return !s.active&&matches({s.center,s.shift},c->frequency,c->shift);});
            if(old!=stations_.end()){c->id=old->id;c->prefix=old->text+"\n";}
            else{
                if(stations_.size()+psk_->tabs()+fec_->tabs()>=maxTabs){++limited_;continue;}
                c->id=nextId_++;stations_.push_back({});stations_.back().id=c->id;
            }
        }
        auto s=std::find_if(stations_.begin(),stations_.end(),[&](const auto& v){return v.id==c->id;});
        if(s==stations_.end())continue;
        s->center=c->frequency;s->frequency=c->frequency+(result.reverse?-1:1)*c->shift/2;
        s->shift=c->shift;s->baud=result.baud;s->reverse=result.reverse;s->active=seconds_-c->lastDecoded<2;
        s->observeSnr(radio::spectralSnr(int(size_),binHz_,(s->center-center_)/binHz_,s->shift+2*s->baud,[&](int b){return power_[b];}));
        auto text=c->prefix+result.text;
        if(text.size()>40000)text.erase(0,text.size()-32000);
        if(received||(!text.empty()&&text!=s->text))s->received();
        s->text=std::move(text);
        if(s->text.size()>40000)s->text.erase(0,s->text.size()-32000);
    }
}
Status Skimmer::snapshot() const{
    Status s;s.stations=stations_;s.center=center_;s.rate=rate_;s.seconds=seconds_;s.candidates=candidates_;
    s.focus=focus_;
    for(const auto& c:channels_)if(focus_.contains(c->frequency,c->shift))++s.focusedDecoders;
    s.decoders=channels_.size();s.limited=limited_;s.blocks=blocks_;s.signals=signals_;
    psk_->append(s);
    fec_->append(s);
    for(const auto& target:selection_.pinned){
        if(!std::any_of(s.signals.begin(),s.signals.end(),[&](const auto& v){return matches(target,v.center,v.shift);}))s.signals.push_back({target.center,target.shift,false,false,false});
    }
    for(auto& signal:s.signals){
        for(const auto& c:channels_)if(matches({c->frequency,c->shift},signal.center,signal.shift)){
            signal.decoding=true;signal.confirmed=c->id!=0;break;
        }
    }
    std::sort(s.signals.begin(),s.signals.end(),[](const auto& a,const auto& b){return a.center<b.center;});
    return s;
}
void Skimmer::close(uint64_t id){
    psk_->close(id);
    fec_->close(id);
    channels_.erase(std::remove_if(channels_.begin(),channels_.end(),[&](const auto& c){return c->id==id;}),channels_.end());
    stations_.erase(std::remove_if(stations_.begin(),stations_.end(),[&](const auto& s){return s.id==id;}),stations_.end());
}
void Skimmer::closeAll(){discontinuity();stations_.clear();psk_->closeAll();fec_->closeAll();}
void Skimmer::selection(Selection selection){
    selection_=validated(std::move(selection));
    psk_->selection(selection_);
    fec_->selection(selection_);
    channels_.erase(std::remove_if(channels_.begin(),channels_.end(),[&](const auto& c){
        return contains(selection_.ignored,c->frequency,c->shift)||
            (selection_.onlyPinned&&!contains(selection_.pinned,c->frequency,c->shift));
    }),channels_.end());
    stations_.erase(std::remove_if(stations_.begin(),stations_.end(),[&](const auto& s){return contains(selection_.ignored,s.center,s.shift);}),stations_.end());
    for(auto& s:stations_)s.active=false;
    cooldowns_.clear();lastDiscovery_=-1;
}
void Skimmer::focus(Focus focus){
    focus=focus.valid()?focus:Focus{};if(focus==focus_)return;
    focus_=focus;lastDiscovery_=-1;
    cooldowns_.erase(std::remove_if(cooldowns_.begin(),cooldowns_.end(),[&](const auto& c){return focus_.contains(c.frequency,c.shift);}),cooldowns_.end());
}
void Skimmer::modes(bool rtty,bool psk){
    if(rttyEnabled_!=rtty){channels_.clear();signals_.clear();cooldowns_.clear();candidates_=limited_=0;for(auto& s:stations_)s.active=false;}
    if(pskEnabled_!=psk)psk_->reset();
    rttyEnabled_=rtty;pskEnabled_=psk;lastDiscovery_=-1;
}
void Skimmer::fecModes(bool js8,bool olivia){js8Enabled_=js8;oliviaEnabled_=olivia;fec_->modes(js8,olivia);lastDiscovery_=-1;}
void Skimmer::captureTime(double utc){fec_->captureTime(utc,seconds_);}
void Skimmer::waitForDecoders(){fec_->waitIdle(maxTabs-stations_.size()-psk_->tabs());}
}
