// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_skimmer.h"
#include "psk_decoder.h"
namespace psk {
// Shares the host packet queue and wide FFT. Only channel inverse FFTs are added.
class Monitor {
public:
    static constexpr size_t maxDecoders=8;
    Monitor();
    ~Monitor();
    void reset();
    void discover(const std::vector<float>& power,double binHz,double center,double seconds,const rtty::Focus& focus,const rtty::Selection& selection);
    void frame(const std::vector<rtty::IQ>& spectrum,double rate,double center,double seconds,size_t tabBudget);
    void append(rtty::Status& status) const;
    void close(uint64_t id);
    void closeAll();
    void selection(const rtty::Selection& selection);
    size_t tabs() const{return stations_.size();}
private:
    struct Channel;
    std::vector<std::unique_ptr<Channel>> channels_;
    std::vector<rtty::Station> stations_;
    std::vector<rtty::Status::Signal> signals_;
    struct Cooldown {double frequency,until;};
    std::vector<Cooldown> cooldowns_;
    std::vector<float> scratch_;
    rtty::FFT fft_{128};
    std::vector<rtty::IQ> band_=std::vector<rtty::IQ>(128);
    uint64_t nextId_=uint64_t(1)<<63;
    size_t candidates_=0,limited_=0;
};
}
