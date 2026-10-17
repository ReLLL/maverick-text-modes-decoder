// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fec_decoder.h"
#include "gfsk8modem.h"
#include "signal_metrics.h"
#include "rtty_skimmer.h"
#include "fec_monitor.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <random>
#include <stdexcept>
#include <ctime>
#include "pj_mfsk.h"
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){try{
    {
        // Narrow strong carriers with broad low-level skirts must not displace
        // a weaker Olivia candidate from the two bounded probe slots.
        fec::Monitor monitor;monitor.modes(false,true);
        std::vector<float> power(8192,1.f);constexpr double binHz=12,center=14070000;
        for(int b:{-300,-100,100}){for(int j=-5;j<=5;++j)power[(b+j+8192)%8192]=3;power[(b+8192)%8192]=100000;}
        for(int j=-10;j<=10;++j)power[275+j]=100;
        monitor.discover(power,binHz,center,1,{},{});rtty::Status status;monitor.append(status);
        require(std::any_of(status.signals.begin(),status.signals.end(),[](auto& s){return s.shift==rtty::oliviaTarget&&s.decoding&&std::abs(s.center-14073300)<30;}),"Narrow carriers monopolized Olivia probes");
    }
    // Frozen tones from upstream JS8Call's independent Fortran genjs8/encode174
    // and Boost CRC-12, source a7ff1be0b389d287fdc56e2ea0d06962aa68127d.
    const char* oracle[]{"4256130557167227312130552171520106314256130674270247777777777777777346624256130","0623541557167227312130552171520106311502364674270247777777777777777346622506413"};
    for(int i=0;i<2;++i){std::vector<int> tones;require(gfsk8::encode(i?gfsk8::Submode::Fast:gfsk8::Submode::Normal,3,"tYuK++++++++",tones),"JS8 oracle encode failed");
        require(tones.size()==79,"JS8 symbol count changed");for(size_t j=0;j<tones.size();++j)require(tones[j]==oracle[i][j]-'0',"JS8 differs from upstream Fortran/CRC oracle");}
    std::vector<float> spectrum(1024,1.f);
    spectrum[100]=101;
    double a=radio::spectralSnr(1024,10,100,40,[&](int b){return spectrum[b];},false);
    require(std::abs(a-10*std::log10(100./250))<1e-6,"SNR reference bandwidth incorrect");
    spectrum[100]=1001;
    double b=radio::spectralSnr(1024,10,100,40,[&](int b){return spectrum[b];},false);
    require(std::abs(b-a-10)<1e-6,"SNR did not track tenfold signal power");
    require(!std::isfinite(radio::spectralSnr(1024,10,1,100,[&](int b){return spectrum[b];},false)),"SNR invented a capture-edge measurement");
    for(auto mode:{gfsk8::Submode::Normal,gfsk8::Submode::Fast,gfsk8::Submode::Turbo,gfsk8::Submode::Slow,gfsk8::Submode::Ultra}){
        auto frames=gfsk8::pack("K1ABC","FN42","HELLO WORLD",mode);require(!frames.empty(),"JS8 packing failed");
        auto pcm=gfsk8::modulate(mode,frames[0].frameType,frames[0].payload,1500);
        auto parms=gfsk8::submodeParms(mode);std::vector<int16_t> capture(720000);
        size_t start=720000-size_t(parms.periodSeconds)*12000;
        std::mt19937 random(731);std::normal_distribution<float> noise(0,50);
        for(auto& v:capture)v=int16_t(noise(random));
        for(size_t i=0;i<pcm.size();++i)capture[start+i]+=int16_t(pcm[i]*12000);
        fec::Js8 decoder;auto work=std::clock();auto out=decoder.decode(capture,86400);
        require(std::any_of(out.begin(),out.end(),[](const auto& f){return f.text.find("HELLO")!=std::string::npos;}),"JS8 decoded payload not unpacked to text");
        std::cout<<"JS8 mode "<<int(mode)<<": "<<double(std::clock()-work)/CLOCKS_PER_SEC<<" CPU seconds\n";
    }
    {
        fec::Js8 decoder;std::vector<int16_t> capture(720000);std::mt19937 random(6007);std::normal_distribution<float> noise(0,3000);
        for(auto& v:capture)v=int16_t(noise(random));
        require(decoder.decode(capture,86400).empty(),"Noise qualified as JS8");
        auto frames=gfsk8::pack("K1ABC","FN42","HEARTBEAT FN42",gfsk8::Submode::Normal);
        require(!frames.empty(),"JS8 header reference packing failed");
        auto wave=gfsk8::modulate(gfsk8::Submode::Normal,frames.front().frameType,frames.front().payload,1500);
        for(size_t i=0;i<wave.size();++i)capture[540000+i]+=int16_t(wave[i]*500);
        auto out=decoder.decode(capture,86400,1);
        require(std::any_of(out.begin(),out.end(),[](const auto& f){return f.call=="K1ABC"&&f.grid=="FN42";}),"Weak JS8 header call/grid missing");
        for(const auto& f:out)std::cout<<"Weak JS8: "<<f.call<<' '<<f.grid<<' '<<f.snr<<" dB\n";
    }
    for(auto preset:std::array<std::pair<int,int>,7>{{{4,125},{4,250},{8,250},{8,500},{16,500},{16,1000},{32,1000}}}){
        MFSK_Transmitter<double> tx;tx.Default();tx.Tones=preset.first;tx.Bandwidth=preset.second;
        tx.FirstCarrierMultiplier=float((1500-preset.second*(1.-.5/preset.first)/2)/500);tx.Reverse=0;
        require(tx.Preset()==0,"Olivia transmitter preset failed");
        const std::string text="CQ CQ DE K1ABC HELLO WORLD 123 TESTING OLIVIA ";
        for(int i=0;i<12;++i)for(unsigned char c:text)tx.PutChar(c);tx.Start();
        fec::Olivia rx;std::vector<double> audio(tx.MaxOutputLen);size_t count=0;auto work=std::clock();
        while(count<8000*100){int n=tx.Output(audio.data());require(n>0,"No Olivia waveform");rx.process(audio.data(),n);count+=n;}
        if(!rx.locked()||rx.text().find("HELLO WORLD")==std::string::npos){std::cerr<<preset.first<<'/'<<preset.second<<' '<<rx.format()<<' '<<rx.quality()<<' '<<rx.text()<<'\n';throw std::runtime_error("Olivia reference decode failed");}
        require(rx.format()=="Olivia "+std::to_string(preset.first)+"/"+std::to_string(preset.second),"Wrong Olivia format");
        std::cout<<rx.format()<<": "<<double(std::clock()-work)/CLOCKS_PER_SEC<<" CPU seconds / 100 seconds input\n";
    }
    {
        constexpr int rate=8000,seconds=90;constexpr double center=14070000;
        MFSK_Transmitter<double> tx;tx.Default();tx.Tones=8;tx.Bandwidth=250;tx.FirstCarrierMultiplier=float((1500-250*(1.-.5/8)/2)/500);tx.Reverse=0;
        require(tx.Preset()==0,"Olivia integration waveform failed");
        const std::string phrase="CQ CQ DE K1ABC HELLO WORLD 123 TESTING OLIVIA ";
        for(int i=0;i<12;++i)for(unsigned char c:phrase)tx.PutChar(c);tx.Start();
        size_t length=1;while(length<size_t(rate*seconds))length*=2;
        std::vector<rtty::IQ> wave(length);std::vector<double> buffer(tx.MaxOutputLen);size_t written=0;
        while(written<size_t(rate*seconds)){int n=tx.Output(buffer.data());for(int i=0;i<n&&written<size_t(rate*seconds);++i)wave[written++]={float(buffer[i]),0};}
        // Analytic RF fixture from the reference audio, removing its mirror.
        rtty::FFT fft(length);fft.transform(wave);for(size_t i=1;i<length/2;++i)wave[i]*=2.f;for(size_t i=length/2;i<length;++i)wave[i]={};fft.transform(wave,true);
        auto frames=gfsk8::pack("K1ABC","FN42","HELLO WORLD",gfsk8::Submode::Normal);std::vector<int> tones;
        require(gfsk8::encode(gfsk8::Submode::Normal,frames.front().frameType,frames.front().payload,tones),"JS8 integration waveform failed");
        rtty::Skimmer monitor;monitor.modes(false,false);monitor.fecModes(true,true);monitor.focus({center-1900,center-1300});
        monitor.process(nullptr,0,rate,center);monitor.captureTime(86400);
        std::vector<rtty::IQ> packet(137);size_t sample=0;double phase=0;int batch=0;auto work=std::clock();
        for(size_t n=0;n<size_t(rate*seconds);n+=packet.size()){
            for(auto& v:packet){
                double t=double(sample)/rate,within=std::fmod(t,15)-.5;int symbol=int(std::floor(within/.16));
                v=sample<wave.size()?wave[sample]*rtty::IQ(std::polar(.7,2*3.141592653589793*-200*t)):rtty::IQ{};
                if(within>=0&&symbol>=0&&symbol<79){phase=std::remainder(phase+2*3.141592653589793*(-1700+tones[symbol]*6.25)/rate,2*3.141592653589793);v+=std::polar(.5f,float(phase));}
                ++sample;
            }
            monitor.process(packet.data(),packet.size(),rate,center);if(++batch%8==0)monitor.waitForDecoders();
        }
        monitor.waitForDecoders();auto s=monitor.snapshot();
        for(const auto& station:s.stations)std::cout<<station.mode<<' '<<station.frequency-center<<' '<<station.format<<' '<<station.text.substr(0,100)<<'\n';
        require(std::count_if(s.stations.begin(),s.stations.end(),[](auto& v){return v.mode=="JS8"&&v.text.find("HELLO")!=std::string::npos;})==1,"Full-capture JS8 integration missed payload or created an aliased copy");
        require(std::any_of(s.stations.begin(),s.stations.end(),[](auto& v){return v.mode=="Olivia"&&v.text.find("HELLO WORLD")!=std::string::npos;}),"Full-capture Olivia integration missed payload");
        require(s.fecDropped==0,"Paced FEC fixture overflowed");
        std::cout<<"Mixed JS8/Olivia: "<<double(std::clock()-work)/CLOCKS_PER_SEC<<" total CPU seconds / 90 seconds IQ; FEC worker "<<s.fecProcessingMs/1000<<" seconds\n";
        auto count=s.stations.size();monitor.fecModes(false,false);monitor.waitForDecoders();s=monitor.snapshot();
        require(s.js8Decoders==0&&s.oliviaDecoders==0&&s.stations.size()==count,"FEC off lost tabs or kept processing");
        monitor.closeAll();require(monitor.snapshot().stations.empty(),"FEC close all retained text");
    }
    std::cout<<"JS8, Olivia and SNR reference checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
