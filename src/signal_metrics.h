// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
namespace radio {
// Integrated signal power minus noise in its occupied band, referenced to
// 2500 Hz of locally measured noise. Approximate: interference and fading bias
// spectral estimates. Reuses the decoder spectrum; no additional FFT.
template<class Power>
double spectralSnr(int size,double binHz,double bin,double width,Power power,bool wrap=true){
    const double missing=std::numeric_limits<double>::quiet_NaN();
    if(size<8||!std::isfinite(binHz)||binHz<=0||!std::isfinite(bin)||width<=0)return missing;
    const int first=int(std::floor(bin-width/(2*binHz))),last=int(std::ceil(bin+width/(2*binHz)));
    const int low=wrap?-size/2:0,high=wrap?size/2:size;
    if(first<low||last>=high)return missing;
    auto at=[&](int b){return double(power((b+size)%size));};
    std::array<double,512> noise{};size_t count=0;
    const int radius=std::max(12,int(std::ceil((width/2+400)/binHz)));
    for(int b=std::max(low,int(bin)-radius);b<std::min(high,int(bin)+radius+1);++b)
        if((b<first-2||b>last+2)&&count<noise.size()){double v=at(b);if(std::isfinite(v)&&v>1e-25)noise[count++]=v;}
    if(count<8)return missing;
    auto middle=noise.begin()+count/2;std::nth_element(noise.begin(),middle,noise.begin()+count);
    const double floor=*middle;double sum=0;
    for(int b=first;b<=last;++b){double v=at(b);if(!std::isfinite(v))return missing;sum+=v;}
    double signal=sum-floor*(last-first+1);
    return signal>0?10*std::log10(signal/(floor*2500/binHz)):missing;
}
}
