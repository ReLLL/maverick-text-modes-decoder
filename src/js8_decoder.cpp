// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fec_decoder.h"
#include "gfsk8modem.h"
#include "DecodedText.h"
#include <cmath>
#include <mutex>
#include <stdexcept>
namespace fec {
namespace {std::mutex js8Mutex;}
struct Js8::Impl {gfsk8::Decoder decoder{31,350,2800};};
Js8::Js8():impl_(std::make_unique<Impl>()){}
Js8::~Js8()=default;
std::vector<Frame> Js8::decode(const std::vector<int16_t>& samples,double utc,int mask){
    if(samples.size()!=720000||!std::isfinite(utc)||utc<0||mask<=0||mask>31)throw std::invalid_argument("Invalid JS8 capture");
    std::vector<Frame> out;
    // The upstream API snapshots a global audio buffer. Serialize all notebook
    // instances; no host/UI thread enters this path.
    std::lock_guard<std::mutex> lock(js8Mutex);
    impl_->decoder.submodes(mask);
    impl_->decoder.clearHistory();
    impl_->decoder.decode(samples,int(std::floor(utc))%86400,[&](const gfsk8::Decoded& d){
        if(!std::isfinite(d.quality)||d.quality<.17f)return;
        DecodedText decoded(d.message,d.frameType,d.submode);
        if(decoded.frameType()==Varicode::FrameUnknown||decoded.message().empty())return;
        Frame f;f.text=decoded.message();f.frequency=d.frequencyHz;f.snr=d.snrDb;
        f.first=(d.frameType&1)!=0;f.last=(d.frameType&2)!=0;
        auto mode=gfsk8::submodeParms(static_cast<gfsk8::Submode>(d.submode));
        f.time=std::floor(utc/mode.periodSeconds)*mode.periodSeconds-mode.periodSeconds;
        const char* name=d.submode==0?"Normal":d.submode==1?"Fast":d.submode==2?"Turbo":d.submode==4?"Slow":"Ultra";
        f.format=std::string("JS8 ")+name;
        if(decoded.isCompound()){f.call=decoded.compoundCall();f.grid=decoded.extra();}
        if(decoded.isDirectedMessage()){
            const auto parts=decoded.directedMessage();
            f.call=parts[0];
            if(parts.size()>3&&parts[2]==" SNR")f.remoteReport=parts[3];
        }
        // Only accept a standard grid carried in a decoded header field.
        if(f.grid.size()!=4||f.grid[0]<'A'||f.grid[0]>'R'||f.grid[1]<'A'||f.grid[1]>'R'||
            f.grid[2]<'0'||f.grid[2]>'9'||f.grid[3]<'0'||f.grid[3]>'9')f.grid.clear();
        out.push_back(std::move(f));
    });
    return out;
}
}
