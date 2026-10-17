// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_fft.h"
#include <array>
#include <cmath>
namespace fec {
// Fractional low-pass interpolation. Linear interpolation of IQ alone creates
// spectral images which can masquerade as valid copies of a strong JS8 frame.
class Resampler {
public:
    Resampler(double input,double output):input_(input),output_(output){
        constexpr double pi=3.141592653589793;
        const double cutoff=std::min(1450.,input*.42);
        for(size_t p=0;p<taps_.size();++p){double sum=0;
            for(size_t j=0;j<65;++j){double x=double(j)-32-double(p)/32;
                double tap=(std::abs(x)<1e-12?2*cutoff/input:std::sin(2*pi*cutoff/input*x)/(pi*x))*(.42-.5*std::cos(2*pi*j/64)+.08*std::cos(4*pi*j/64));
                taps_[p][j]=float(tap);sum+=tap;
            }
            for(auto& v:taps_[p])v/=float(sum);
        }
    }
    template<class Output>void sample(rtty::IQ value,Output output){
        history_[index_]=value;index_=(index_+1)&127;clock_+=output_;
        while(clock_>=input_){
            clock_-=input_;double phase=clock_/output_*32;
            size_t p=std::min(size_t(phase),size_t(31));float blend=float(phase-p);rtty::IQ sum{};
            for(size_t j=0;j<65;++j){float tap=taps_[p][j]+blend*(taps_[p+1][j]-taps_[p][j]);sum+=history_[(index_+127-j)&127]*tap;}
            output(sum);
        }
    }
private:
    double input_,output_,clock_=0;
    size_t index_=0;
    std::array<rtty::IQ,128> history_{};
    std::array<std::array<float,65>,33> taps_{};
};
}
