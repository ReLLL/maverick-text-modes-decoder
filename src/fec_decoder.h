// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <limits>
namespace fec {
struct Frame {
    std::string text,call,grid,format,remoteReport;
    double frequency=0,snr=std::numeric_limits<double>::quiet_NaN(),time=0;
    bool first=false,last=false;
};
class Js8 {
public:
    Js8();~Js8();
    // Exactly 60 seconds of chronological 12 kHz PCM, ending at utc seconds.
    std::vector<Frame> decode(const std::vector<int16_t>& samples,double utc,int mask=31);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
class Olivia {
public:
    Olivia();~Olivia();
    void process(const double* samples,size_t count);
    bool locked() const;
    std::string text() const;
    std::string format() const;
    double offset() const;
    // FEC synchronization metric, deliberately not labeled as received SNR.
    double quality() const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
