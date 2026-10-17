// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "psk_decoder.h"
#include "psk_varicode.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace psk {
namespace {
constexpr double pi=3.141592653589793;
std::vector<float> lowpass(size_t length,double cutoff,double rate){
    std::vector<float> taps(length);double sum=0;
    for(size_t i=0;i<length;++i){double x=double(i)-double(length-1)/2;
        double v=x==0?2*cutoff/rate:std::sin(2*pi*cutoff/rate*x)/(pi*x);
        taps[i]=float(v*(.42-.5*std::cos(2*pi*i/(length-1))+.08*std::cos(4*pi*i/(length-1))));sum+=taps[i];}
    for(auto& v:taps)v/=float(sum);return taps;
}
rtty::IQ filtered(const std::vector<rtty::IQ>& history,const std::vector<float>& taps,size_t next){
    rtty::IQ sum{};for(size_t i=0;i<taps.size();++i)sum+=history[(next+history.size()-1-i)%history.size()]*taps[i];return sum;
}
}
struct Decoder::Trial {
    double baud,period,time=0,next=0,frequency=0,phase=0,quality=0,power=0;
    std::vector<float> taps;
    std::vector<rtty::IQ> history;
    std::array<double,32> timingEnergy{};
    size_t index=0;
    rtty::IQ last{},previous{},middle{};
    bool center=false,havePrevious=false;
    unsigned code=0,length=0,characters=0,invalid=0,bits=0,transitions=0;
    std::array<bool,32> validHistory{};
    unsigned frameCount=0,goodFrames=0;
    std::string text,evidence;
    explicit Trial(double b):baud(b),period(1000/b),next(period/2),taps(lowpass(size_t(4*period)+1,b*.7,1000)),history(taps.size()){}
    void frame(bool valid){auto& old=validHistory[frameCount%validHistory.size()];if(old)--goodFrames;old=valid;if(valid)++goodFrames;++frameCount;}
    bool framing() const{return goodFrames*10>=std::min(frameCount,32u)*9;}
    void bit(bool one){
        ++bits;if(!one)++transitions;
        code=(code<<1)|unsigned(one);++length;
        if((code&3)==0){
            if(code){
                const unsigned value=code>>2;
                auto found=std::find(varicode.begin(),varicode.end(),value);
                int c=found==varicode.end()?-1:int(found-varicode.begin());
                if(c>=32&&c<127){text+=char(c);evidence+=char(c);++characters;}
                else if(c==10||c==13||c==9){if(c!=13||text.empty()||text.back()!='\n')text+=c==9?'\t':'\n';++characters;}
                else if(c==8){text+="[BS]";++characters;} // Visible control; never erase confirmed text.
                else {++invalid;if(characters<12){text.clear();evidence.clear();}}
                frame((c>=32&&c<127)||c==10||c==13||c==9||c==8);
            }
            code=length=0;
        }else if(length>12){++invalid;frame(false);code=length=0;if(characters<12){text.clear();evidence.clear();}}
        if(text.size()>40000)text.erase(0,text.size()-32000);
        if(evidence.size()>64)evidence.erase(0,evidence.size()-64);
    }
    bool qualified() const{
        if(characters<12||bits<100||transitions<20||quality<.92||invalid*12>characters||!framing())return false;
        std::array<unsigned,128> counts{};unsigned n=0,unique=0,maximum=0;
        for(unsigned char c:evidence)if(c>32&&c<127){++counts[c];++n;}
        if(n<12)return false;
        double entropy=0;for(auto c:counts)if(c){++unique;maximum=std::max(maximum,c);double p=double(c)/n;entropy-=p*std::log2(p);}
        return unique>=5&&maximum*4<n*3&&entropy>=2.2;
    }
    void sample(rtty::IQ input){
        phase=std::remainder(phase+2*pi*frequency/1000,2*pi);
        history[index]=input*std::polar(1.f,float(-phase));index=(index+1)%history.size();
        auto value=filtered(history,taps,index);++time;
        // Acquire the eye's energy maximum before starting the Gardner loop.
        // Starting at a transition null can otherwise produce a stable false
        // timing solution when joining an already-running stream.
        auto& energy=timingEnergy[size_t(time)%size_t(period)];energy=.9*energy+.1*std::norm(value);
        if(time<=1000){
            if(time==1000){
                auto peak=std::max_element(timingEnergy.begin(),timingEnergy.begin()+size_t(period))-timingEnergy.begin();
                next=time+std::fmod(double(peak)+period-std::fmod(time,period),period);
                center=true;havePrevious=false;
            }
            last=value;return;
        }
        if(time>=next){
            const float fraction=float(std::clamp(time-next,0.,1.));
            auto symbol=value*(1-fraction)+last*fraction;
            double correction=0;
            if(center){
                if(havePrevious){
                    auto d=symbol*std::conj(previous);double magnitude=std::abs(d);
                    power=.98*power+.02*std::norm(symbol);
                    if(magnitude>std::max(1e-14,power*.1)){
                        double angle=.5*std::arg(d*d);quality=.98*quality+.02*std::cos(2*angle);
                        frequency=std::clamp(frequency+.025*angle*baud/(2*pi),-baud*.24,baud*.24);
                        if(quality>.3)bit(d.real()>0);
                        correction=std::clamp(double(((previous-symbol)*std::conj(middle)).real())/std::max(1e-14,power)*.12,-.35,.35);
                    }else {quality*=.98;code=length=0;}
                }
                previous=symbol;havePrevious=true;
            }else middle=symbol;
            center=!center;next+=period/2+correction;
        }
        last=value;
    }
};
Decoder::Decoder(double rate,double residual):rate_(rate),step_(std::polar(1.f,float(-2*pi*residual/rate))),history_(97),taps_(lowpass(97,110,rate)){
    if(!std::isfinite(rate)||rate<1000||!std::isfinite(residual))throw std::invalid_argument("Invalid PSK channel");
    trials_[0]=std::make_unique<Trial>(31.25);trials_[1]=std::make_unique<Trial>(62.5);
}
Decoder::~Decoder()=default;
void Decoder::process(const rtty::IQ* samples,size_t count){
    for(size_t i=0;i<count;++i){
        auto value=samples[i];if(!std::isfinite(value.real())||!std::isfinite(value.imag()))value={};
        history_[index_]=value*phase_;index_=(index_+1)%history_.size();phase_*=step_;
        if((i&255)==0){float a=std::abs(phase_);if(a>0)phase_/=a;}
        clock_+=1000;
        if(clock_>=rate_){clock_-=rate_;auto x=filtered(history_,taps_,index_);for(auto& t:trials_)t->sample(x);}
    }
}
Result Decoder::result() const{
    if(selected_<0){int best=-1;double score=0;
        for(int i=0;i<2;++i){const auto& t=*trials_[i];double s=t.quality*double(t.characters)/(t.characters+t.invalid+1);
            if(t.qualified()&&s>score){best=i;score=s;}}
        selected_=best;
    }
    if(selected_<0)return {};
    const auto& t=*trials_[selected_];return {t.text,t.baud,t.frequency,t.quality,t.characters,t.quality>.75&&t.framing(),t.bits};
}
std::array<Decoder::Diagnostic,2> Decoder::diagnostics() const{
    std::array<Diagnostic,2> out;
    for(size_t i=0;i<2;++i){const auto& t=*trials_[i];out[i]={t.baud,t.frequency,t.quality,t.characters,t.invalid,t.goodFrames,t.frameCount,t.text};}
    return out;
}
}
