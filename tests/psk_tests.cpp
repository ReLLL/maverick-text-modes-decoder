// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "psk_decoder.h"
#include "psk_fixture.h"
#include "psk_priority.h"
#include "psk_monitor.h"
#include "rtty_skimmer.h"
#include <iostream>
#include <stdexcept>
#include <random>
#include <chrono>
#include <algorithm>
#include <ctime>
void require(bool b,const char* message){if(!b)throw std::runtime_error(message);}
int main(int argc,char**){try{
    if(argc<2){
    for(double base:{3580000.,7040000.,14070000.,21070000.,28120000.}){
        require(!psk::commonWindow(base-1)&&psk::commonWindow(base)&&psk::commonWindow(base+3999)&&!psk::commonWindow(base+4000),"PSK common window boundary failed");
        rtty::Selection selected;selected.pinned.push_back({base+1000,0});
        require(psk::priority(base+1000,{},selected)==2&&psk::priority(base+1000,{base,base+2000},selected)==3&&psk::priority(base+500,{},selected)==1,"PSK priority order failed");
    }
    require(!psk::mayReplace(1,0,true,4,100,1)&&!psk::mayReplace(1,0,false,1,100,1),"Common window interrupted confirmed/last ordinary stream");
    require(psk::mayReplace(1,0,false,2,1,100)&&psk::mayReplace(0,1,false,0,1,100),"Common window fairness failed");
    require(!psk::mayReplace(0,1,false,1,100,1)&&!psk::mayReplace(1,2,false,0,100,1)&&psk::mayReplace(3,2,true,0,1,100),"PSK hard priority protection failed");
    {
        psk::Monitor monitor;std::vector<float> power(8192,1e-10f);
        auto peak=[&](int bin,float strength){power[(bin+8192)%8192]=strength;};
        for(int i=0;i<8;++i)peak(20+i*40,1);
        peak(-100,.001f);peak(-140,.0001f);
        monitor.discover(power,10,14070000,0,{},{});rtty::Status s;monitor.append(s);
        require(s.pskDecoders==8&&std::any_of(s.signals.begin(),s.signals.end(),[](const auto& v){return v.center<14070000&&v.decoding;}),"Common window starved entire-capture candidates");
        monitor.discover(power,10,14070000,.125,{14068500,14069100},{});s={};monitor.append(s);
        require(std::count_if(s.signals.begin(),s.signals.end(),[](const auto& v){return v.center<14070000&&v.decoding;})==2,"Common window displaced tuned streams");
        monitor.reset();std::fill(power.begin(),power.end(),1e-10f);
        for(int i=0;i<8;++i)peak(-100-i*40,1);
        monitor.discover(power,10,14070000,0,{},{});
        peak(100,.001f);monitor.discover(power,10,14070000,.125,{},{});s={};monitor.append(s);
        require(std::any_of(s.signals.begin(),s.signals.end(),[](const auto& v){return v.center>14070000&&v.decoding;}),"Weak common-window candidate not prioritized");
    }
    for(double rate:{2000.,3000.,4000.})for(double baud:{31.25,62.5})for(double offset:{-7.,0.,7.}){
        psk::Decoder decoder(rate,offset);fixture::Psk tx(rate,offset+1.5,baud);
        std::vector<rtty::IQ> packet(137);
        for(size_t n=0;n<size_t(rate*24);n+=packet.size()){
            for(auto& v:packet)v=tx.sample();decoder.process(packet.data(),packet.size());
        }
        auto r=decoder.result();
        if(!r.locked||r.text.find("CQ CQ DE TEST HELLO 123 abcdef K")==std::string::npos){std::cerr<<rate<<' '<<baud<<' '<<offset<<" -> "<<r.text<<'\n';throw std::runtime_error("PSK reference not decoded");}
        require(r.baud==baud,"Wrong PSK baud");
    }
    for(double rate:{3000.,4000.})for(double baud:{31.25,62.5})for(int skip:{0,23,61,91}){
        fixture::Psk tx(rate,0,baud);for(int i=0;i<int(rate*16)+skip;++i)tx.sample();psk::Decoder decoder(rate,0);
        std::vector<rtty::IQ> packet(137);
        for(size_t n=0;n<size_t(rate*16);n+=packet.size()){
            for(auto& v:packet)v=tx.sample();decoder.process(packet.data(),packet.size());decoder.result();
        }
        auto r=decoder.result();require(r.locked&&r.baud==baud&&r.text.find("HELLO")!=std::string::npos,"Mid-transmission PSK timing acquisition failed");
    }
    std::mt19937 random(1947);std::normal_distribution<float> noise(0,.1);
    for(double baud:{31.25,62.5})for(double drift:{-.0008,.0008})for(double residual:{-5.,5.}){
        psk::Decoder decoder(3000,0);fixture::Psk tx(3000,residual,baud*(1+drift));std::vector<rtty::IQ> packet(113);
        for(size_t n=0;n<3000*40;n+=packet.size()){
            for(auto& v:packet){v=std::conj(tx.sample())*float(.65+.35*std::sin(tx.time*2));v+=rtty::IQ{noise(random),noise(random)};}
            decoder.process(packet.data(),packet.size());
        }
        auto r=decoder.result();
        if(!r.locked||r.text.find("HELLO 123 abcdef")==std::string::npos){std::cerr<<"Clock/noise "<<baud<<' '<<drift<<' '<<residual<<" -> "<<r.text<<'\n';for(const auto& d:decoder.diagnostics())std::cerr<<d.baud<<" Hz "<<d.frequency<<" q "<<d.quality<<" chars "<<d.characters<<" errors "<<d.invalid<<" good "<<d.goodFrames<<"/32 "<<d.text<<'\n';throw std::runtime_error("PSK clock/fading/noise fixture failed");}
    }
    for(int kind=0;kind<4;++kind){
        psk::Decoder decoder(4000,0);std::vector<rtty::IQ> packet(127);
        fixture::Psk repeat(4000,0,31.25,"EEEEEEEEEEEEEEEEEEEE ");
        for(size_t n=0;n<4000*30;n+=packet.size()){
            for(auto& v:packet)v=kind==0?rtty::IQ{}:kind==1?rtty::IQ{1,0}:kind==2?rtty::IQ{noise(random),noise(random)}:repeat.sample();
            decoder.process(packet.data(),packet.size());
        }
        require(!decoder.result().locked,"Non-PSK or repetitive interference acquired");
    }
    {
        rtty::Skimmer monitor;monitor.modes(true,true);std::vector<rtty::IQ> packet(127);size_t sample=0;
        for(size_t n=0;n<8000*30;n+=packet.size()){
            for(auto& v:packet){double t=double(sample++)/8000;
                v=rtty::IQ{noise(random),noise(random)}+std::polar(1.f,float(2*3.141592653589793*900*t))+
                    std::polar(float(std::sin(t*17)>0?.8:0),float(2*3.141592653589793*-600*t));}
            monitor.process(packet.data(),packet.size(),8000,14070000);
        }
        require(monitor.snapshot().stations.empty(),"Noise/carrier/keying created digital tabs");
    }
    for(double rate:{8000.,96000.,1024000.,4096000.}){
        rtty::Skimmer monitor;monitor.modes(false,true);
        fixture::Psk a(rate,-1012.3,31.25),b(rate,1521.7,62.5);
        std::vector<rtty::IQ> packet(4093);
        double cpu=0,wall=0;
        for(size_t n=0;n<size_t(rate*16);n+=packet.size()){
            for(auto& v:packet)v=a.sample()+b.sample()*.7f;
            auto start=std::chrono::steady_clock::now();
            auto work=std::clock();
            monitor.process(packet.data(),packet.size(),rate,14070000);
            cpu+=double(std::clock()-work)/CLOCKS_PER_SEC;
            wall+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        }
        auto s=monitor.snapshot();
        if(s.stations.size()!=2){std::cerr<<"Monitor rate "<<rate<<" stations "<<s.stations.size()<<" candidates "<<s.candidates<<" checking "<<s.pskDecoders<<'\n';for(auto signal:s.signals)std::cerr<<signal.center-14070000<<' ';std::cerr<<'\n';throw std::runtime_error("Parallel PSK monitor missed streams");}
        for(auto& station:s.stations){require(station.psk,"PSK tab mode lost");require(station.text.find("CQ CQ DE TEST HELLO")!=std::string::npos,"PSK scanner payload corrupted");}
        std::cout<<"PSK rate "<<rate<<": "<<cpu<<" seconds CPU, "<<wall<<" seconds elapsed DSP / 16 seconds input\n";
        rtty::Selection chosen;chosen.onlyPinned=true;chosen.pinned.push_back({s.stations.front().center,0});monitor.selection(chosen);
        require(monitor.snapshot().pskDecoders==1,"PSK only-pinned kept unwanted decoder");
        chosen.onlyPinned=false;chosen.ignored.push_back({s.stations.back().center,0});monitor.selection(chosen);
        require(monitor.snapshot().stations.size()==1,"PSK ignore did not close its tab");
        monitor.discontinuity();require(monitor.snapshot().stations.size()==1,"Gap lost PSK text");
        monitor.close(s.stations.front().id);require(monitor.snapshot().stations.empty(),"PSK close failed");
        monitor.closeAll();require(monitor.snapshot().stations.empty(),"PSK close all failed");
    }
    }
    {
        rtty::Skimmer monitor;monitor.modes(false,true);
        constexpr double rate=96000,center=14070000;
        std::vector<fixture::Psk> transmitters;for(int i=0;i<10;++i)transmitters.emplace_back(rate,-3000+i*400,i%2?62.5:31.25);
        std::vector<rtty::IQ> packet(4093);
        auto feed=[&](double seconds){for(size_t n=0;n<size_t(rate*seconds);n+=packet.size()){
            for(auto& v:packet){v={};for(size_t i=0;i<transmitters.size();++i)v+=transmitters[i].sample()*float(.3+.1*i);}
            monitor.process(packet.data(),packet.size(),rate,center);
        }};
        auto work=std::clock();feed(16);std::cout<<"Crowded PSK CPU: "<<double(std::clock()-work)/CLOCKS_PER_SEC<<" seconds including generation / 16 seconds input\n";
        auto before=monitor.snapshot();require(before.pskDecoders==8&&before.limited>=2,"PSK capacity not enforced/reported");
        monitor.focus({center-3050,center-2550});feed(16);auto focused=monitor.snapshot();
        if(focused.pskDecoders!=8||focused.focusedDecoders!=2){std::cerr<<"Focused "<<focused.pskDecoders<<" decoders, "<<focused.focusedDecoders<<" priority, "<<focused.stations.size()<<" tabs, "<<focused.limited<<" waiting\n";for(const auto& signal:focused.signals)std::cerr<<signal.center-center<<" checking="<<signal.decoding<<'\n';throw std::runtime_error("PSK tuned priority missed multiple streams");}
        for(double wanted:{center-3000,center-2600})if(!std::any_of(focused.stations.begin(),focused.stations.end(),[&](const auto& s){return std::abs(s.frequency-wanted)<10&&s.text.find("HELLO")!=std::string::npos;})){
            for(const auto& s:focused.stations)std::cerr<<"Station "<<s.frequency-center<<' '<<s.baud<<' '<<s.text<<'\n';throw std::runtime_error("PSK tuned stream payload absent");}
        monitor.modes(false,false);require(monitor.snapshot().pskDecoders==0&&!monitor.snapshot().stations.empty(),"Disabling PSK lost text or kept processing");
    }
    if(argc<2){
    {
        rtty::Skimmer monitor;monitor.modes(true,true);constexpr double rate=96000,center=14070000;
        fixture::Psk a(rate,-700,31.25),b(rate,1400,62.5);fixture::Fsk c(rate,-3200);std::vector<rtty::IQ> packet(733);
        auto feed=[&](double seconds){for(size_t n=0;n<size_t(rate*seconds);n+=packet.size()){
            for(auto& v:packet)v=a.sample()+b.sample()+c.sample();monitor.process(packet.data(),packet.size(),rate,center);}};
        feed(18);auto s=monitor.snapshot();
        require(s.stations.size()==3,"Mixed PSK/RTTY missed streams or created false tabs");
        require(std::count_if(s.stations.begin(),s.stations.end(),[](const auto& v){return v.psk;})==2,"Mixed protocols mislabeled tabs");
        for(const auto& v:s.stations)require(v.text.find(v.psk?"HELLO":"CQ CQ DE TEST")!=std::string::npos,"Mixed protocol payload missing");
        monitor.discontinuity();feed(18);auto resumed=monitor.snapshot();
        for(const auto& old:s.stations)require(std::any_of(resumed.stations.begin(),resumed.stations.end(),[&](const auto& v){return v.id==old.id&&v.text.find(old.text)==0;}),"Gap/reacquisition did not preserve original text");
        monitor.modes(false,true);feed(1);require(monitor.snapshot().decoders==0&&monitor.snapshot().pskDecoders>0,"RTTY off stopped PSK or kept RTTY running");
    }
    {
        rtty::Skimmer monitor;monitor.modes(false,true);constexpr double rate=8000;
        std::vector<fixture::Psk> tx;for(int i=0;i<8;++i)tx.emplace_back(rate,-2400+i*600,i%2?62.5:31.25);
        std::vector<rtty::IQ> packet(419);
        for(int group=0;group<5;++group){
            for(size_t n=0;n<size_t(rate*18);n+=packet.size()){
                for(auto& v:packet){v={};for(auto& t:tx)v+=t.sample();}
                monitor.process(packet.data(),packet.size(),rate,14070000+group*100000);
            }
            auto s=monitor.snapshot();require(s.stations.size()<=32,"Digital tab budget exceeded");
            if(group==4)require(s.stations.size()==32&&s.limited>0,"Full tabs did not report capacity");
        }
        monitor.closeAll();require(monitor.snapshot().stations.empty(),"Full PSK close-all failed");
    }
    }
    std::cout<<"PSK31/63 references, parallel full-capture monitoring, AFC and rejection passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
