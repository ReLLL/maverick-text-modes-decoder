// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_fft.h"
#include <array>
#include <memory>
#include <string>
namespace psk {
struct Result {std::string text;double baud=0,offset=0,quality=0;unsigned characters=0;bool locked=false;unsigned symbols=0;};
class Decoder {
public:
    struct Diagnostic {double baud,frequency,quality;unsigned characters,invalid,goodFrames,frames;std::string text;};
    Decoder(double rate,double residual);
    ~Decoder();
    void process(const rtty::IQ* samples,size_t count);
    Result result() const;
    std::array<Diagnostic,2> diagnostics() const;
private:
    struct Trial;
    double rate_,clock_=0;
    rtty::IQ phase_{1,0},step_;
    std::vector<rtty::IQ> history_;
    std::vector<float> taps_;
    size_t index_=0;
    std::array<std::unique_ptr<Trial>,2> trials_;
    mutable int selected_=-1;
};
}
