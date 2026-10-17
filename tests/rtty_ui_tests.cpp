// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "rtty_signal_list.h"
#include "rtty_presentation.h"
#include "rtty_report.h"
#include <iostream>
#include <stdexcept>

void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    rtty::Selection narrow;narrow.pinned={{7100000,200},{7100000,170},{7100000,201}};
    narrow.ignored={{7105000,200}};narrow.onlyPinned=true;
    const auto valid=rtty::validated(narrow);
    require(valid.pinned.size()==2&&valid.ignored.size()==1&&valid.onlyPinned,"Selection discarded the exact 200 Hz preset or accepted an invalid shift");
    require(rtty::contains(valid.pinned,7100000,200)&&!rtty::matches({7100000,170},7100000,200),"170/200 Hz selections were merged");
    rtty::SignalList list;rtty::Selection selection;rtty::Status status;
    status.signals={{10100800,450,true,true,true},{10105000,170,true,false,false}};
    list.update(status,selection);
    const auto chosen=list.rows()[1];
    // Same press/release location, but the live discovery snapshot changes
    // between the two frames. The second checkbox must still target 10105 kHz.
    status.signals={{10098000,170,true,false,false}};
    list.update(status,selection);
    require(list.rows().size()==2,"Live discovery removed rows during selection");
    require(list.rows()[1].center==chosen.center,"Live discovery moved checkbox targets");
    status.signals={{10100830,425,true,false,false},{10112000,850,true,false,false}};
    list.update(status,selection);
    require(list.rows()[0].center==10100800&&list.rows()[0].shift==450,"Frequency drift moved a held target");
    list.refresh();list.update(status,selection);
    require(list.rows().size()==2&&list.rows()[1].center==10112000,"Explicit refresh failed to discover new signals");
    selection.pinned={{10100830,425},{11039000,450}};selection.ignored={{4583000,450}};
    list.refresh();list.update(status,selection);
    require(list.rows().size()==4,"Refresh lost absent pinned/ignored frequencies or duplicated a pair");
    require(list.rows().front().center==4583000,"Refreshed table is not in frequency order");
    require(!rtty::recentText(0,10),"Empty reception appeared active");
    require(rtty::recentText(10,10)&&rtty::recentText(10,69.99),"Fresh letters did not keep a green indicator for one minute");
    require(!rtty::recentText(10,70.01),"Indicator did not turn yellow after one minute");
    require(rtty::recentText(80,80)&&rtty::recentText(80,139.99)&&!rtty::recentText(80,140.01),"Returning RTTY did not restart the full green minute");
    for(const auto& mode:{"USB","LSB","AM","CW","DSB","NFM"}){
        const auto f=rtty::radioFocus(mode,7045000,3000,7045000,96000);
        const double low=std::string(mode)=="USB"?7045000:std::string(mode)=="LSB"?7042000:7043500;
        require(f.valid()&&f.low==low&&f.high==low+3000,"Tuned passband has the wrong sideband or bounds");
    }
    require(!rtty::radioFocus("USB",7045000,NAN,7045000,96000).valid(),"Invalid passband accepted");
    require(!rtty::radioFocus("USB",7100000,3000,7045000,96000).valid(),"Out-of-capture passband accepted");
    require(!rtty::Focus{7044000,7045000}.contains(7044950,170),"Pair with one tone outside Radio received priority");
    const std::string raw="CQ\nRYRYRYRYRYRYRYRYRYRY\nFREQUENCIES 123\nYRYRYRYRYRYR\nRYRYYRY";
    require(rtty::compactRY(raw)=="CQ\n[RY test pattern]\nFREQUENCIES 123\n[RY test pattern]\nRYRYYRY","RY display folding lost message text or hid imperfect short sequences");
    require(raw.find("RYRYRYRYRYRYRYRYRYRY")!=std::string::npos,"RY display changed received text");
    require(rtty::compactRY("RYRYRYRYRYR")=="RYRYRYRYRYR","Short ordinary letters were folded");
    require(rtty::compactRY("RYRYRYRYRYRY")==rtty::compactRY("RYRYRYRYRYRYRY"),"Growing test pattern kept changing the display label");
    rtty::Station station;station.frequency=10100580;station.center=10100805;station.shift=450;station.baud=50;station.reverse=true;station.active=true;station.text=raw;
    rtty::Status capture;capture.center=10114000;capture.rate=3200000;capture.dropped=7;
    const auto report=rtty::receptionReport(station,capture,true,1792108800);
    for(const char* expected:{"RTTY reception","Date/time:","(copied)","Day:","Time zone:","30 m (Region 1 range)","10100.580 kHz (estimated mark)","10100.805 kHz","50.00 baud","450 Hz","Reverse","Receiving","Capture now:","3.200 MS/s","RTTY gaps now:  7","[RY test pattern]"})
        require(report.find(expected)!=std::string::npos,"RTTY report omitted reception metadata or displayed text");
    const auto original=rtty::receptionReport(station,capture,false,1792108800);
    require(original.find(raw)!=std::string::npos&&original.find("Original received text")!=std::string::npos,"Original report did not preserve received text");
    station.frequency=7645780;station.active=false;station.snr=-14.2;
    const auto outside=rtty::receptionReport(station,capture,false,1792108800);
    require(outside.find("HF (outside listed Region 1 amateur ranges)")!=std::string::npos&&outside.find("Inactive")!=std::string::npos,"Report mislabeled an out-of-band/inactive reception");
    require(outside.find("-14.2 dB")!=std::string::npos,"Inactive tab discarded its last valid RX SNR");
    std::cout<<"Stable signal selection checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
