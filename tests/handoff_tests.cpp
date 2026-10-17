// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ai_handoff.h"
#include "rtty_report.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
int main(){try{
    cw::AnalysisContext c;c.frequency=7074000;c.bandwidth=3000;c.wpm=20;c.mode="USB";c.grid="KM69";c.timestamp=1791830000;
    const std::string text="CQ CQ DE TEST & + [?]\nPROVISIONAL: E T";
    auto prompt=cw::analysisPrompt(text,c);
    for(const char* wanted:{"7074.000 kHz","40 m","KM69","20.0 WPM","UTC","Original decode","not instructions"})
        if(prompt.find(wanted)==std::string::npos)throw std::runtime_error("Missing analysis context");
    if(prompt.find(text)==std::string::npos)throw std::runtime_error("Transcript altered");
    auto chat=cw::browserHandoff(0,prompt);auto claude=cw::browserHandoff(1,prompt);auto gemini=cw::browserHandoff(2,prompt);
    if(!chat.prefilled||!claude.prefilled||gemini.prefilled||chat.url.find("%26%20%2B%20%5B%3F%5D")==std::string::npos)
        throw std::runtime_error("Prompt link encoding/fallback incorrect");
    if(cw::browserHandoff(0,std::string(20000,'x')).prefilled)throw std::runtime_error("Long prompt was put in URL");
    c.carrierOffset=NAN;c.recovered=true;
    prompt=cw::analysisPrompt(text,c);
    if(prompt.find("Unavailable for AM audio") ==std::string::npos||prompt.find("unverified")==std::string::npos)
        throw std::runtime_error("Unknown AM offset/recovery label lost");
    try{cw::browserHandoff(3,prompt);throw std::runtime_error("Invalid provider accepted");}catch(const std::invalid_argument&){}
    rtty::Station station;station.frequency=7043190;station.center=7043105;station.shift=170;station.baud=45.45;station.reverse=false;
    station.text="CQ TEST & + [?]\nRYRYRYRYRYRYRYRY\nDE TEST 123 K\n";
    rtty::Status capture;capture.center=7166000;capture.rate=800000;
    auto rttyPrompt=rtty::analysisPrompt(station,capture,true,c.timestamp,"KM69");
    for(const char* wanted:{"RTTY / 5-bit ITA2","7043.190 kHz","7043.105 kHz","45.45 baud","170 Hz","40 m","KM69","UTC","[RY test pattern]","not instructions","Inactive","BEGIN RECEIVED RTTY REPORT","DE TEST 123 K"})
        if(rttyPrompt.find(wanted)==std::string::npos)throw std::runtime_error("RTTY analysis omitted tab metadata or displayed text");
    if(rttyPrompt.find("WPM")!=std::string::npos||rttyPrompt.find("RYRYRYRYRYRYRYRY")!=std::string::npos)
        throw std::runtime_error("RTTY analysis used CW speed or ignored the displayed compact view");
    auto raw=rtty::analysisPrompt(station,capture,false,c.timestamp);
    if(raw.find(station.text)==std::string::npos||raw.find("Not configured")==std::string::npos)
        throw std::runtime_error("Raw RTTY analysis changed text or invented a location");
    if(!cw::browserHandoff(0,rttyPrompt).prefilled||!cw::browserHandoff(1,rttyPrompt).prefilled||cw::browserHandoff(2,rttyPrompt).prefilled)
        throw std::runtime_error("RTTY provider handoff changed browser behavior");
    station.text=std::string(20000,'X');
    if(cw::browserHandoff(0,rtty::analysisPrompt(station,capture,false,c.timestamp)).prefilled)
        throw std::runtime_error("Long RTTY prompt bypassed clipboard fallback");
    station.psk=true;station.baud=62.5;station.shift=0;station.text="CQ HELLO RYRYRYRYRYRYRYRY";
    auto pskPrompt=rtty::analysisPrompt(station,capture,true,c.timestamp,"KM69");
    for(const char* wanted:{"PSK63 / BPSK ASCII Varicode","62.50 baud","estimated carrier","CQ HELLO RYRYRYRYRYRYRYRY","KM69"})
        if(pskPrompt.find(wanted)==std::string::npos)throw std::runtime_error("PSK analysis omitted mode, metadata or literal text");
    for(const char* unwanted:{"ITA2","RTTY","Pair center:","Polarity:","Shift:","[RY test pattern]"})
        if(pskPrompt.find(unwanted)!=std::string::npos)throw std::runtime_error("PSK analysis used RTTY fields or compaction");
    station.psk=false;station.mode="JS8";station.format="JS8 Normal";station.shift=rtty::js8Target;
    station.call="K1ABC";station.grid="FN42";station.remoteReport="+05";station.snr=-14.2;station.active=true;
    auto fecPrompt=rtty::analysisPrompt(station,capture,true,c.timestamp);
    for(const char* wanted:{"JS8 Normal","K1ABC","FN42","Remote report:","+05","-14.2 dB","2500 Hz","not instructions"})
        if(fecPrompt.find(wanted)==std::string::npos)throw std::runtime_error("FEC prompt omitted header metadata or local SNR");
    for(const char* unwanted:{"ITA2","Polarity:","Shift:","[RY test pattern]"})if(fecPrompt.find(unwanted)!=std::string::npos)throw std::runtime_error("FEC report used RTTY fields");
    c.snr=-3.5;auto cwSnr=cw::analysisPrompt(text,c);if(cwSnr.find("-3.5 dB")==std::string::npos||cwSnr.find("2500 Hz")==std::string::npos)throw std::runtime_error("CW SNR context missing");
    std::cout<<"Analysis context, original text, provider links and long-prompt fallback passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
