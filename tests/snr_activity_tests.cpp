// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include "rtty_skimmer.h"
#include "psk_fixture.h"
#include <iostream>
#include <random>
#include <stdexcept>

namespace {
constexpr double pi=3.14159265358979323846;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void cwHistory(){
    for(float manual:{0.f,800.f}){
        cw::Decoder decoder;decoder.reset(20,manual);
        std::mt19937 random(948);std::normal_distribution<float> noise(0,.01f);
        std::vector<float> block(800);double time=0;
        auto feed=[&](int blocks,float amplitude){
            for(int b=0;b<blocks;++b){
                for(auto& x:block){const auto slot=int(time/.06)%12;
                    const bool key=slot==0||slot==2||slot==4||(slot>=6&&slot<9);
                    x=noise(random)+(key?amplitude*std::sin(2*pi*800*time):0);time+=1./8000;}
                decoder.process(block.data(),block.size(),8000);
            }
        };
        feed(30,0);require(!std::isfinite(decoder.stats().reception.snr),"CW noise alone created retained signal SNR");
        feed(80,.6f);require(std::isfinite(decoder.stats().reception.snr),"CW signal SNR missing");
        feed(10,0);const auto held=decoder.stats().reception;
        feed(40,0);auto now=decoder.stats().reception;
        require(now.snr==held.snr&&now.snrAt==held.snrAt,"CW noise replaced last signal SNR");
        feed(80,.12f);now=decoder.stats().reception;
        require(now.snrAt>held.snrAt&&now.snr!=held.snr,"Returning CW failed to replace held signal SNR");
    }
}
struct Rtty {
    std::vector<bool> bits;double time=0;
    Rtty(){
        bits.assign(60,true);
        for(int n=0;n<200;++n)for(int code:{31,14,23,4,9,1,4,16,1,5,16,4,27,23,19,1,31,8,2}){
            bits.insert(bits.end(),2,false);
            for(int b=0;b<5;++b)bits.insert(bits.end(),2,(code&(1<<b))!=0);
            bits.insert(bits.end(),3,true);
        }
    }
    rtty::IQ sample(){
        const bool mark=bits[size_t(time*100)%bits.size()];
        // Integrate phase across frequency transitions.
        phase+=2*pi*(1000+(mark?100:-100))/8000;time+=1./8000;
        return std::polar(1.f,float(std::remainder(phase,2*pi)));
    }
    double phase=0;
};
void digitalHistory(bool psk,double baud=31.25){
    rtty::Skimmer monitor;monitor.modes(!psk,psk);
    Rtty rtty;fixture::Psk bpsk(8000,1000,baud);
    std::mt19937 random(582);std::normal_distribution<float> noise(0,.008f);
    std::vector<rtty::IQ> block(800);
    auto feed=[&](int blocks,float amplitude){
        for(int b=0;b<blocks;++b){
            for(auto& x:block)x=(psk?bpsk.sample():rtty.sample())*amplitude+rtty::IQ{noise(random),noise(random)};
            monitor.process(block.data(),block.size(),8000,14070000);
        }
    };
    feed(180,.6f);auto stations=monitor.snapshot().stations;
    require(stations.size()==1,"Signal SNR fixture did not acquire exactly one digital stream");
    require(std::isfinite(stations[0].snr),"Digital signal SNR missing");
    feed(6,0);const auto held=monitor.snapshot().stations[0];
    feed(10,0);const auto now=monitor.snapshot().stations[0];
    if(now.snr!=held.snr||now.snrAt!=held.snrAt){
        std::cerr<<(psk?"PSK":"RTTY")<<" SNR "<<held.snr<<" -> "<<now.snr<<'\n';
        throw std::runtime_error("Noise-only input replaced/refreshed retained digital SNR");
    }
    feed(180,.12f);stations=monitor.snapshot().stations;
    require(stations.size()==1&&stations[0].snrAt>held.snrAt&&stations[0].snr!=held.snr,
        "Returning weaker digital signal failed to replace held SNR");
}
}
int main(int argc,char** argv){try{
    const std::string mode=argc>1?argv[1]:"all";
    if(mode=="cw"||mode=="all")cwHistory();
    if(mode=="rtty"||mode=="all")digitalHistory(false);
    if(mode=="psk"||mode=="all"){digitalHistory(true);digitalHistory(true,62.5);}
    std::cout<<mode<<": signal/noise/returning-signal SNR retention passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
