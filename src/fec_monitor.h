// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_skimmer.h"
#include "fec_worker.h"
#include "fec_frame_history.h"
namespace fec {
class Monitor {
public:
    Monitor();~Monitor();
    void modes(bool js8,bool olivia);
    void reset();void closeAll();void close(uint64_t id);
    void selection(const rtty::Selection& selection);
    void discover(const std::vector<float>& power,double binHz,double center,double seconds,const rtty::Focus& focus,const rtty::Selection& selection);
    void frame(const std::vector<rtty::IQ>& spectrum,double rate,double center,double seconds,size_t tabBudget);
    void append(rtty::Status& status) const;
    size_t tabs() const{return stations_.size();}
    void waitIdle(size_t budget=32);
    void captureTime(double utc,double seconds){epoch_=utc-seconds;}
private:
    struct Channel;
    void collect(size_t budget);
    void updateKeys();
    std::unique_ptr<Worker> worker_;
    std::vector<std::unique_ptr<Channel>> channels_;
    std::vector<rtty::Station> stations_;
    std::vector<rtty::Status::Signal> signals_;
    rtty::Selection selection_;
    rtty::FFT fft_{256};std::vector<rtty::IQ> band_=std::vector<rtty::IQ>(256);
    std::vector<float> scratch_;
    bool js8_=false,olivia_=false;
    double epoch_=0,captureLow_=0,captureHigh_=0;
    uint64_t nextKey_=1,nextId_=uint64_t(1)<<62;
    size_t limited_=0;
    std::map<uint64_t,FrameHistory> lastFrames_;
    struct Cooldown {double frequency,until;bool js8;};
    std::vector<Cooldown> cooldowns_;
};
}
