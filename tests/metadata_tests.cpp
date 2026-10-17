// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "reception.h"
#include "country_lookup.h"
#include "rtty_report.h"
#include "fec_frame_history.h"
#include <iostream>
#include <stdexcept>
#include <tuple>
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    radio::Reception r;require(!std::isfinite(r.snr)&&radio::receptionAge(r,100)=="--","Unknown reception invented metadata");
    r.observeSnr(-14.2,10);r.observeSnr(NAN,11);r.observeSnr(INFINITY,12);
    require(r.snr==-14.2&&r.snrAt==10,"Missing SNR erased the last measurement or refreshed its age");
    require(r.currentSnr(true,11)&&!r.currentSnr(false,11)&&!r.currentSnr(true,12),"Held SNR presented as a live reading");
    r.observeSnr(0,20);require(r.snr==0&&r.snrAt==20,"Zero dB was treated as missing");
    r.received(100,1792108800);
    require(radio::receptionAge(r,100)=="0 seconds ago"&&radio::receptionAge(r,101)=="1 second ago"&&radio::receptionAge(r,219.99)=="119 seconds ago","Recent reception age has wrong boundaries");
    require(radio::receptionAge(r,220)==radio::receptionTimestamp(1792108800),"120-second reception did not switch to a saved timestamp");
    require(radio::receptionAge(r,99)=="0 seconds ago","Negative relative age underflowed");
    for(auto [call,name,flag]:std::vector<std::tuple<std::string,std::string,std::string>>{
        {"K1ABC","United States","US"},{"ta2abc","Asiatic Turkey","TR"},{"DL1ABC","Germany","DE"},
        {"JA1ABC","Japan","JP"},{"G4ABC","England","GB"},{"EA8/K1ABC/P","Canary Islands","ES"},
        {"W1AW/VE3","Canada","CA"},{"W1AW/P","United States","US"},{"W1AW/4","United States","US"},
        {"3D2ABC","Fiji","FJ"},{"3D2CR","Conway Reef","FJ"},{"TA1C/2","Asiatic Turkey","TR"},
        {"W1AW/KH6","Hawaii","US"}}){
        const auto c=radio::callsignCountry(call);require(c.name==name&&c.flag==flag,("Wrong country/flag for "+call+": "+c.name+" / "+c.flag).c_str());
    }
    for(const auto* call:{"","CQ","DE","<K1ABC>","K1ABC/MM","W1AW/AM","K1ABC/QQ9","/K1ABC","K1ABC/","K1ABC//P","K1ABC;DROP","12345"})
        require(radio::callsignCountry(call).name.empty(),("Unknown/maritime/invalid call invented a land country: "+std::string(call)).c_str());
    rtty::Station station;station.mode="JS8";station.call="K1ABC";station.grid="";station.active=false;station.snr=-14.2;station.lastTextUtc=1792108800;
    auto report=rtty::receptionReport(station,{},false,1792109000);
    for(const char* text:{"-14.2 dB","last valid measurement","United States (callsign prefix estimate)","Last received:"})
        require(report.find(text)!=std::string::npos,"Report lost retained SNR, received time or callsign country");
    require(report.find("Header grid:")==std::string::npos,"Country inference fabricated a grid");
    fec::FrameHistory frames;
    require(frames.accept("slot1|JS8|HELLO")&&frames.accept("slot2|JS8|HELLO")&&!frames.accept("slot1|JS8|HELLO"),"Interleaved duplicate frames would refresh last reception");
    for(int i=0;i<128;i++)require(frames.accept(std::to_string(i)),"Distinct frame discarded");
    require(frames.accept("slot1|JS8|HELLO"),"Frame history grew without eviction");
    std::cout<<"Retained SNR, timestamps, callsign countries, portable calls and report provenance passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
