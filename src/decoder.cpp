// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include "signal_metrics.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>

namespace cw {
namespace {
constexpr double pi = 3.14159265358979323846;
double median(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    return v[v.size()/2];
}
}
std::string morse(const std::string& s) {
    static const std::unordered_map<std::string, std::string> table = {
        {".-","A"},{"-...","B"},{"-.-.","C"},{"-..","D"},{".","E"},{"..-.","F"},
        {"--.","G"},{"....","H"},{"..","I"},{".---","J"},{"-.-","K"},{".-..","L"},
        {"--","M"},{"-.","N"},{"---","O"},{".--.","P"},{"--.-","Q"},{".-.","R"},
        {"...","S"},{"-","T"},{"..-","U"},{"...-","V"},{".--","W"},{"-..-","X"},
        {"-.--","Y"},{"--..","Z"},{"-----","0"},{".----","1"},{"..---","2"},
        {"...--","3"},{"....-","4"},{".....","5"},{"-....","6"},{"--...","7"},
        {"---..","8"},{"----.","9"},{".-.-.-","."},{"--..--",","},{"..--..","?"},
        {"-..-.","/"},{"-...-","="},{".-.-.","<AR>"},{"...-.-","<SK>"},
        {"-.-.-","<KA>"},{"........","<HH>"},{".-...","<AS>"},{".--.-.","@"},
        {"-....-","-"},{".----.","'"},{"-.--.","("},{"-.--.-",")"},{"---...",":"},
        {"-.-.--","!"},{".-..-.","\""},{"..--.-","_"},{"-.-.-.",";"}
    };
    auto it = table.find(s);
    return it == table.end() ? "[?]" : it->second;
}
void Timing::reset(float manualWpm) {
    key_ = started_ = ready_ = letterSent_ = wordSent_ = false;
    duration_ = 0; manual_ = manualWpm;
    unit_ = manualWpm > 0 ? 1.2 / std::clamp(double(manualWpm),3.0,200.0) : .06;
    cutoff_ = 2 * unit_;
    marks_.clear(); gaps_.clear(); pending_.clear(); symbol_.clear();
    ready_ = manualWpm > 0;
    filteredKey_=false;transition_=0;glitches_=0;gapCutoff_=2;
    acquisitionUnit_=acquisitionCutoff_=0;
    relockUnit_=0;markSerial_=relockSerial_=0;relockVotes_=0;
    learningSeconds_=previewSeconds_=0;provisional_.clear();provisionalWpm_=0;
}
void Timing::estimate() {
    if(recovery_ && ready_ && gaps_.size()>=12){
        std::vector<double> inner,letters;
        double low=unit_*4.5,high=0;
        for(double gap:gaps_)if(gap>unit_*.55 && gap<unit_*4.5){low=std::min(low,gap);high=std::max(high,gap);}
        for(int pass=0;pass<6;pass++){
            inner.clear();letters.clear();
            for(double gap:gaps_)if(gap>unit_*.55 && gap<unit_*4.5)
                (gap<(low+high)/2?inner:letters).push_back(gap);
            if(inner.size()<4 || letters.size()<4)break;
            low=median(inner);high=median(letters);
        }
        if(inner.size()>=4 && letters.size()>=4 && high/low>1.4 && high/low<4)
            gapCutoff_=std::clamp((low+high)/(2*unit_),1.25,2.6);
    }
    if(!ready_)acquisitionUnit_=0;
    if (manual_ > 0 || marks_.size() < (ready_?4:3)) return;
    // Learning evidence is recent; pending_ independently retains the whole
    // reception for replay. Noise before these 16 marks cannot stall learning.
    std::vector<double> v(marks_.end()-std::min(size_t(16),marks_.size()),marks_.end());
    if(ready_&&markSerial_!=relockSerial_){
        relockSerial_=markSerial_;
        double candidate=recentUnit(v,true);
        if(candidate>0&&(candidate<unit_*.7||candidate>unit_*1.4)){
            relockVotes_=relockUnit_>0&&std::abs(candidate/relockUnit_-1)<.12?relockVotes_+1:1;
            relockUnit_=candidate;
        }else{relockUnit_=0;relockVotes_=0;}
    }
    if(double robust=recentUnit(v)){
        if(!ready_){acquisitionUnit_=robust;acquisitionCutoff_=2*robust;return;}
        unit_=robust;cutoff_=2*robust;return;
    }
    // Poor new evidence must not overwrite a supported speed with a fit to
    // fading fragments. Retuning and quiet reacquisition still reset it.
    if(ready_)return;
    std::sort(v.begin(), v.end());
    double a=v.front(), b=v.back();
    for (int n=0;n<8;n++) {
        std::vector<double> lo,hi;
        for (double x:v) (x<(a+b)/2 ? lo:hi).push_back(x);
        if (lo.empty() || hi.empty()) break;
        a=median(lo); b=median(hi);
    }
    if (b/a > 1.8 && b/a < 5.5 && a>=.004 && b<=1.3) {
        double candidate=std::clamp((a+b/3)/2,.006,.4);
        // Reject unsupported estimates; clamping would relabel noise as CW.
        if(!extendedSpeed_&&(candidate<1.2/45-1e-8||candidate>1.2/5+1e-8))return;
        // A clean mixed character is enough, but it must finish with a
        // character gap before committing. Single-class E/T is ambiguous.
        size_t dots=0,dashes=0,inner=0;
        for(double mark:v){
            if(mark>=candidate*.55&&mark<=candidate*1.55)++dots;
            if(mark>=candidate*2.2&&mark<=candidate*3.8)++dashes;
        }
        size_t count=0;
        for(auto it=gaps_.rbegin();it!=gaps_.rend()&&count<16;++it,++count)
            if(*it>=candidate*.55&&*it<=candidate*1.6)++inner;
        bool clean=dots>=1&&dashes>=1&&inner>=2&&dots+dashes>=v.size()*.9&&b/a>2.3&&b/a<3.7;
        bool supported=v.size()>=8&&dots>=3&&dashes>=3&&inner>=3&&dots+dashes>=v.size()*.75;
        if(!clean&&!supported)return;
        double last=marks_.back();
        if(last<candidate*.55||last>candidate*3.8)return;
        acquisitionUnit_=candidate;acquisitionCutoff_=(a+b)/2;
    }
}
void Timing::acquire(){
    if(ready_||key_||acquisitionUnit_<=0||duration_<=acquisitionUnit_*2.2)return;
    unit_=acquisitionUnit_;cutoff_=acquisitionCutoff_;ready_=true;
    replay();
    consume({false,duration_});
}
void Timing::setSpeed(float wpm){
    relockUnit_=0;relockVotes_=0;
    if(wpm<=0){if(ready_)reset();return;}
    manual_=wpm;unit_=1.2/std::clamp(double(wpm),3.0,200.0);cutoff_=unit_*2;
    ready_=true;replay();
    if(!key_)consume({false,duration_});
}
void Timing::extendedSpeed(bool enabled){
    extendedSpeed_=enabled;
    if(!enabled&&manual_==0&&ready_&&(wpm()<5||wpm()>45))reset();
    if(!ready_){estimate();acquire();}
}
void Timing::replay(){
    // Interpret all held durations together only once the timing is supported.
    // Retain the historical startup-noise rejection before the first valid mark.
    bool begin=false;
    pending_.each([&](const Run& run){
        if(!begin&&run.key&&run.seconds>=unit_*.55&&run.seconds<=unit_*3.8)begin=true;
        if(begin)consume(run);
    });
    pending_.clear();
    provisional_.clear();provisionalWpm_=0;
}
void Timing::letter() {
    if (!symbol_.empty() && output) output(morse(symbol_));
    symbol_.clear(); letterSent_=true;
}
void Timing::consume(const Run& r) {
    if (r.key) {
        if (r.seconds < std::max(.002,unit_*.18)) return;
        if (r.seconds > unit_*7 || symbol_.size()>=9) {
            if (output) output("[?]");
            symbol_.clear(); return;
        }
        symbol_ += r.seconds < cutoff_ ? '.' : '-';
        letterSent_=wordSent_=false;
    } else {
        if (r.seconds>unit_*gapCutoff_ && !letterSent_) letter();
        if (r.seconds>unit_*5 && !wordSent_) {
            if (output) output(" ");
            wordSent_=true;
        }
    }
}
void Timing::tick(bool key, double seconds) {
    if(!std::isfinite(seconds)||seconds<=0||seconds>.1)return;
    if(!recovery_){advance(key,seconds);return;}
    // Symmetric debounce delays both edges equally. Short key dropouts and
    // impulses disappear without inserting or guessing Morse elements.
    if(key!=filteredKey_){
        transition_+=seconds;
        double debounce=ready_?std::clamp(unit_*.12,.001,.012):.002;
        if(transition_>=debounce){filteredKey_=key;transition_=0;}
    }else if(transition_>0){++glitches_;transition_=0;}
    advance(filteredKey_,seconds);
}
void Timing::advance(bool key,double seconds) {
    if (!started_) { if(!key)return; started_=true; key_=key; }
    if (key != key_) {
        Run r{key_,duration_};
        if (key_ && duration_>=.003 && duration_<=1.3) {
            marks_.push_back(duration_); if(marks_.size()>64)marks_.pop_front();
            ++markSerial_;
        } else if (!key_ && duration_>=.003 && duration_<1.5) {
            gaps_.push_back(duration_); if(gaps_.size()>64)gaps_.pop_front();
        }
        estimate();
        if (!ready_) {
            pending_.push_back(r);
        } else consume(r);
        key_=key; duration_=0;
    }
    duration_+=seconds;
    acquire();
    preview(seconds);
    if (ready_ && !key_) consume({false,duration_});
    // Finish the current character with its existing interpretation. Only
    // sustained, high-support evidence may escape a mistaken speed lock.
    if(ready_&&!key_&&relockVotes_>=4&&duration_>std::max(unit_,relockUnit_)*2.2){
        unit_=relockUnit_;cutoff_=2*unit_;gapCutoff_=2;
        relockUnit_=0;relockVotes_=0;
    }
}
Decoder::Decoder() { reset(); }
void Decoder::setSpeed(float wpm){
    manualWpm_=wpm;timing_.setSpeed(wpm);
    stats_.wpm=timing_.calibrated()?timing_.wpm():0;stats_.calibrated=timing_.calibrated();
    stats_.bufferedRuns=timing_.bufferedRuns();
    stats_.provisional=timing_.provisional();stats_.provisionalWpm=timing_.provisionalWpm();
}
void Decoder::reset(float wpm, float tone) {
    manualWpm_=wpm;quietTicks_=0;quietReset_=false;
    timing_.reset(wpm);
    timing_.output=[this](const std::string& s){
        stats_.calibrated=timing_.calibrated();stats_.wpm=stats_.calibrated?timing_.wpm():0;
        if(output)output(s);
    };
    stats_={}; if(wpm>0)stats_.wpm=wpm; manualTone_=tone; if(tone>0)stats_.tone=tone;
    audio_.fill(0); levels_.fill(0);
    audioPos_=levelPos_=samples_=ticks_=0;
    noise_=peak_=low1_=low2_=iqI_=iqQ_=i2_=q2_=sum_=0;
    phase_=resample_=0; key_=false; toneLocked_=tone>0; debounce_=rate_=0;
    candidateTone_=0;stableTones_=0;lastToneTick_=0;
}
void Decoder::process(const float* mono,size_t count,int rate) {
    if(rate<4000 || rate>384000)return;
    rate_=rate;
    const int processingRate=wideInput_?rate:4000;
    if(wideInput_&&rate!=16000&&rate!=32000&&rate!=64000)return;
    analysisRate_=processingRate;
    if(rate<processingRate)return;
    const float alpha=1-std::exp(-2*pi*(wideInput_?processingRate/2-1000:1700)/rate);
    for(size_t i=0;i<count;i++) {
        float x=std::isfinite(mono[i])?mono[i]:0;
        low1_+=alpha*(x-low1_); low2_+=alpha*(low1_-low2_);
        resample_+=processingRate;
        if(resample_>=rate) { resample_-=rate; sample(low2_); }
    }
}
void Decoder::findTone() {
    const size_t length=analysisRate_/4000*1024;
    const int processingRate=analysisRate_;
    std::array<std::complex<float>,16384> fft;
    for(size_t i=0;i<length;i++)fft[i]={audio_[(audioPos_+i)%length]*float(.5-.5*std::cos(2*pi*i/(length-1))),0};
    for(size_t i=1,j=0;i<length;i++) {
        size_t bit=length/2;for(;j&bit;bit>>=1)j^=bit;j^=bit;
        if(i<j)std::swap(fft[i],fft[j]);
    }
    for(size_t len=2;len<=length;len*=2) {
        auto step=std::polar(1.f,float(-2*pi/len));
        for(size_t i=0;i<length;i+=len) {
            std::complex<float>w=1;
            for(size_t j=0;j<len/2;j++) {auto u=fft[i+j],v=fft[i+j+len/2]*w;fft[i+j]=u+v;fft[i+j+len/2]=u-v;w*=step;}
        }
    }
    size_t best=51;
    std::vector<float> powers;
    const size_t end=wideInput_?size_t((processingRate/2-1000)*length/processingRate):461;
    for(size_t i=51;i<end;i++) {powers.push_back(std::norm(fft[i]));if(std::norm(fft[i])>std::norm(fft[best]))best=i;}
    std::sort(powers.begin(),powers.end());
    float peak=std::norm(fft[best]), floor=powers[powers.size()/2];
    std::vector<float> local;
    for(int i=std::max(1,int(best)-16);i<std::min(int(length/2)-1,int(best)+17);i++)
        if(std::abs(i-int(best))>3)local.push_back(std::norm(fft[i]));
    std::sort(local.begin(),local.end());
    floor=std::max(floor,local[local.size()/2]);
    const double measuredTone=manualTone_>0?manualTone_:double(best)*processingRate/length;
    stats_.snrDb=radio::spectralSnr(int(length/2),double(processingRate)/length,measuredTone*length/processingRate,100,[&](int b){return std::norm(fft[b]);},false);
    if(manualTone_>0){stats_.tone=manualTone_;stats_.signal=true;stats_.reception.observeSnr(stats_.snrDb);return;}
    stats_.snr=10*std::log10((peak+1e-15f)/(floor+1e-15f));
    stats_.signal=stats_.snr>13 && peak>1e-10;
    if(stats_.signal)stats_.reception.observeSnr(stats_.snrDb);
    if(stats_.signal) {
        float a=std::abs(fft[best-1]),b=std::abs(fft[best]),c=std::abs(fft[best+1]);
        float offset=.5f*(a-c)/(a-2*b+c-1e-20f);
        float f=(best+std::clamp(offset,-.5f,.5f))*processingRate/length;
        stableTones_=std::abs(f-candidateTone_)<12?stableTones_+1:1;
        candidateTone_=f;
        if(stableTones_>=3){
            toneLocked_=true;lastToneTick_=ticks_;
            stats_.tone=std::abs(f-stats_.tone)>40?f:stats_.tone*.6f+f*.4f;
        }
    }else stableTones_=0;
    if(ticks_-lastToneTick_>3000)toneLocked_=false;
}
void Decoder::sample(float x) {
    const size_t length=analysisRate_/4000*1024;
    const int processingRate=analysisRate_;
    audio_[audioPos_]=x;audioPos_=(audioPos_+1)%length;
    if(++samples_%(length/2)==0 && samples_>=length)findTone();
    // Ordinary CW acquisition needs less noise bandwidth than the extended
    // high-speed range. Keep this tone-envelope filter separate from Radio's
    // channel width, which still bounds the input signal.
    float cutoff=timing_.calibrated()?std::clamp(timing_.wpm()*4.f,90.f,500.f):(extendedSpeed_?200.f:90.f);
    float alpha=1-std::exp(-2*pi*cutoff/processingRate);
    phase_+=2*pi*stats_.tone/processingRate;if(phase_>2*pi)phase_-=2*pi;
    iqI_+=alpha*(x*std::cos(phase_)-iqI_);iqQ_+=alpha*(x*std::sin(phase_)-iqQ_);
    i2_+=alpha*(iqI_-i2_);q2_+=alpha*(iqQ_-q2_);
    sum_+=std::hypot(i2_,q2_);
    const int perTick=processingRate/1000;
    if(samples_%perTick==0){envelope(sum_/perTick);sum_=0;}
}
void Decoder::envelope(float x) {
    levels_[levelPos_]=x;levelPos_=(levelPos_+1)%levels_.size();
    if(++ticks_%32==0) {
        size_t count=std::min(ticks_,levels_.size());
        std::vector<float> sorted(levels_.begin(),levels_.begin()+count);
        std::sort(sorted.begin(),sorted.end());
        noise_=sorted[count/5];
    }
    // A fixed 333 ms peak memory can hide the next quiet dot after a loud
    // character. In the optional recovery path, follow amplitude on the
    // current element timescale. Keep the same noise floor and tone gates.
    float decay=.997f;
    if(recovery_&&timing_.calibrated()){
        float memoryMs=std::clamp(1800.f/timing_.wpm(),30.f,300.f);
        decay=std::exp(-1.f/memoryMs);
    }
    peak_=std::max(x,peak_*decay);
    float range=std::max(peak_-noise_,1e-9f);
    float threshold=noise_+range*(key_?.22f:.38f);
    // Requiring five times the noise envelope chopped audible weak dashes
    // into short pulses. Tone lock, hysteresis and debounce reject noise
    // without requiring that excessive instantaneous amplitude margin.
    bool ordinary=!extendedSpeed_&&(manualWpm_==0||(manualWpm_>=5&&manualWpm_<=45));
    bool candidate=x>std::max(threshold,noise_*(ordinary?3.f:5.f)) &&
        peak_>noise_*(ordinary?4.f:6.f) && peak_>1e-7f && toneLocked_;
    int debounceTicks=timing_.calibrated()?std::clamp(int(144/timing_.wpm()),2,12):(extendedSpeed_?2:6);
    if(candidate!=key_) {if(++debounce_>=debounceTicks){key_=candidate;debounce_=0;}} else debounce_=0;
    // Delay acquisition until the first tone estimate, avoiding startup filter clicks.
    if(ticks_>=300)timing_.tick(key_);
    if(recovery_){
        if(key_){quietTicks_=0;quietReset_=false;}
        else if(++quietTicks_>12000 && !quietReset_ && timing_.calibrated()){
            // No guessed text at loss of signal; retain manual locks, clear
            // learned timing so the next operator can have a different speed.
            timing_.reset(manualWpm_);quietReset_=true;++stats_.recoveries;
        }
    }
    stats_.level=x;stats_.threshold=threshold;stats_.keyed=key_;
    stats_.wpm=timing_.calibrated()?timing_.wpm():0;stats_.calibrated=timing_.calibrated();
    stats_.filteredGlitches=timing_.glitches();
    stats_.bufferedRuns=timing_.bufferedRuns();
    stats_.provisional=timing_.provisional();stats_.provisionalWpm=timing_.provisionalWpm();
    if(trace)trace(key_,x,threshold);
}
}
