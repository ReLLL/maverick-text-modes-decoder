// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <chrono>
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <ctime>
namespace radio {
inline double monotonicSeconds(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
inline double utcSeconds(){return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();}
// Owned by the decoder worker. Missing observations never erase valid history.
struct Reception {
    double snr=std::numeric_limits<double>::quiet_NaN(),snrAt=0;
    double lastTextAt=0,lastTextUtc=0;
    void observeSnr(double value,double at=monotonicSeconds()){
        if(std::isfinite(value)&&std::isfinite(at)&&at>0){snr=value;snrAt=at;}
    }
    void received(double at=monotonicSeconds(),double utc=utcSeconds()){
        lastTextAt=at;lastTextUtc=utc;
    }
    bool currentSnr(bool receiving,double now=monotonicSeconds())const{
        return receiving&&std::isfinite(snr)&&snrAt>0&&now>=snrAt&&now-snrAt<2;
    }
};
inline std::string receptionTimestamp(double timestamp){
    if(!std::isfinite(timestamp)||timestamp<=0)return "--";
    const auto utc=static_cast<std::time_t>(timestamp);std::tm local{};localtime_r(&utc,&local);
    char label[80]{};std::strftime(label,sizeof(label),"%Y-%m-%d %H:%M:%S %Z",&local);return label;
}
inline std::string receptionAge(const Reception& r,double now=monotonicSeconds()){
    if(r.lastTextAt<=0||r.lastTextUtc<=0)return "--";
    const auto age=std::max(0.,now-r.lastTextAt);
    if(age<120){const auto seconds=static_cast<unsigned>(age);return std::to_string(seconds)+(seconds==1?" second ago":" seconds ago");}
    return receptionTimestamp(r.lastTextUtc);
}
}
