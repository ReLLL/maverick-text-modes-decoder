// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "radio_text.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <ctime>
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    const std::string text="CQ CQ DE K1ABC TA2ABC K1ABC 599 73\n";
    auto marked=radio::markText(text,{});
    require(marked.display=="CQ CQ DE K1ABC[US] TA2ABC[TR] K1ABC[US] 599 73\n","Inline flags lost or changed received characters");
    require(marked.calls.size()==2&&marked.calls.back().call=="K1ABC","Callsign tags are duplicated or not ordered by last occurrence");
    require(marked.calls.front().country.flag=="TR","Callsign country/flag missing");
    auto plain=radio::markText(text,{false,false});require(plain.display==text&&plain.spans.empty()&&plain.calls.empty(),"Disabled annotations changed the received text");
    auto terms=radio::markText(text,{false,true});require(terms.display==text&&terms.calls.empty()&&!terms.spans.empty(),"HAM highlighting depends on country detection");
    require(radio::markText("DE K1AB",{}).calls.empty(),"Unfinished callsign was annotated before its delimiter");
    require(radio::markText("DE K1ABC ",{}).calls.size()==1,"Delimited callsign was missed");
    require(radio::markText("<K1ABC> CQ DE 10100.8 KHZ 599 73 RR73 RTTY PSK31 ",{}).calls.empty(),"Hash/prosign, frequency, report or mode name became a callsign");
    require(radio::markText("[PROVISIONAL - learning, about 20 WPM; may change]\nCQ DE K1ABC ",{}).calls.empty(),"Provisional CW draft established a station identity");
    require(radio::qrzUrl("ea8/k1abc/p")=="https://www.qrz.com/db/EA8%2FK1ABC%2FP","Portable QRZ path was not safely encoded");
    require(radio::qrzUrl("K1ABC/../../").empty()&&radio::qrzUrl("K1ABC?token=x").empty(),"Untrusted transcript text escaped the QRZ callsign path");
    require(radio::unicodeFlag("US")=="\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8"&&radio::unicodeFlag("bad").empty(),"Unicode flag encoding is invalid");
    const radio::TextOptions cautious{true,true,true};
    const std::string contest="YL3NU 599 63 YL3NU\nLYL3NU TU SV2AEL CQ\n";
    auto guarded=radio::markText(contest,cautious);
    require(guarded.display.find("YL3NU[LV]")!=std::string::npos&&guarded.display.find("\nLYL3NU TU")!=std::string::npos,
        "One-off extension gained a misleading flag or its received text was changed");
    require(std::none_of(guarded.calls.begin(),guarded.calls.end(),[](const auto& call){return call.call=="LYL3NU";}),
        "Ambiguous extension gained a country/QRZ tag");
    require(radio::markText(contest,{}).display.find("LYL3NU[LT]")!=std::string::npos,
        "RTTY-only guard changed other modes");
    for(const std::string raw:{"LYL3NU ","YL3NU LYL3NU ","YL3NU YL3NU LYL3NU LYL3NU ",
            "<YL3NU> YL3NU LYL3NU ","LYL3NU YL3NU [PROVISIONAL - learning YL3NU "})
        require(radio::markText(raw,cautious).display.find("LYL3NU[LT]")!=std::string::npos,
            "Uncorroborated, provisional or repeated legitimate callsign lost its annotation");
    require(radio::markText("YL3NU YL3NU LYL3NU/P ",cautious).display.find("LYL3NU/P[LT]")!=std::string::npos,
        "Portable callsign was treated as a single-letter extension");
    for(const auto& pair:std::vector<std::pair<std::string,std::string>>{{"IZ5MMT","GIZ5MMT"},{"JN7TAN","BJN7TAN"},{"PA7KY","XPA7KY"}}){
        const auto raw=pair.first+" "+pair.first+" "+pair.second+" ";auto marked=radio::markText(raw,cautious);
        require(marked.display.find(pair.second+" ")!=std::string::npos&&marked.display.find(pair.second+"[")==std::string::npos,
            "Leading X/G/B artifact still acquired a misleading country flag");
    }
    auto trailing=radio::markText("LYL3NU YL3NU YL3NU",cautious);
    require(trailing.display.find("LYL3NU[LT]")!=std::string::npos,"Unfinished token supplied corroboration");
    require(radio::markText("LYL3NU YL3NU YL3NU ",cautious).display.find("LYL3NU[")==std::string::npos,
        "Completed returning evidence did not invalidate an ambiguous earlier annotation");
    require(radio::markText("yl3nu YL3NU lyl3nu ",cautious).display=="yl3nu[LV] YL3NU[LV] lyl3nu ",
        "Case normalization altered raw text or missed repeated evidence");
    require(radio::markText(contest,{false,true,true}).display==contest,
        "Prefix guard inserted annotations with detection off");
    std::string many;for(int i=0;i<40;i++)many+="K1A"+std::string(1,char('A'+i/26))+char('A'+i%26)+' ';
    require(radio::markText(many,{}).calls.size()==24,"Callsign tag history exceeded its fixed limit");
    std::string large;while(large.size()<100000)large+=text;
    const auto start=std::clock();for(int i=0;i<10;i++){auto r=radio::markText(large,{});require(r.calls.size()==2,"Large transcript lost callsign tags");}
    std::cout<<"Callsigns, flags, QSO colors, original text, QRZ encoding and bounded tags passed; "<<1000.*double(std::clock()-start)/CLOCKS_PER_SEC/10<<" CPU ms / 100k-character parse\n";
    const auto guardStart=std::clock();for(int i=0;i<10;i++)radio::markText(large,cautious);
    std::cout<<"RTTY corroboration: "<<1000.*double(std::clock()-guardStart)/CLOCKS_PER_SEC/10<<" CPU ms / 100k-character parse\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
