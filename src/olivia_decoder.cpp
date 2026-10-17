// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fec_decoder.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include "pj_mfsk.h"
namespace fec {
struct Olivia::Impl {
    struct Trial {
        MFSK_Receiver<double> decoder;
        std::string text,recent;
        double quality=0;
        int tones,width;
        Trial(int t,int w):tones(t),width(w){
            decoder.Default();
            decoder.Tones=t;decoder.Bandwidth=w;decoder.SyncMargin=8;
            decoder.SyncIntegLen=4;decoder.SyncThreshold=5;decoder.SampleRate=decoder.InputSampleRate=8000;
            decoder.FirstCarrierMultiplier=float((1500-w*(1.-.5/t)/2)/500);decoder.Reverse=0;
            if(decoder.Preset()<0)throw std::runtime_error("Olivia allocation failed");
        }
        bool qualified() {
            if(text.size()<16||!std::isfinite(quality)||quality<5)return false;
            std::array<int,128> counts{};int n=0,unique=0,maximum=0;
            int bad=0,nonIdle=0;for(unsigned char c:recent)if(c){++nonIdle;if(c<32&&c!=8&&c!=9&&c!=10&&c!=13)++bad;}
            if(bad*10>nonIdle)return false;
            for(unsigned char c:text.substr(text.size()>64?text.size()-64:0))if(c>32&&c<127){++counts[c];++n;}
            for(int c:counts)if(c){++unique;maximum=std::max(maximum,c);}
            return n>=12&&unique>=6&&maximum*2<n;
        }
    };
    std::vector<std::unique_ptr<Trial>> trials;
    int selected=-1,pending=-1;double seconds=0,pendingSince=0;
    Impl(){for(auto p:std::array<std::pair<int,int>,7>{{{4,125},{4,250},{8,250},{8,500},{16,500},{16,1000},{32,1000}}})trials.push_back(std::make_unique<Trial>(p.first,p.second));}
};
Olivia::Olivia():impl_(std::make_unique<Impl>()){}
Olivia::~Olivia()=default;
void Olivia::process(const double* samples,size_t count){
    impl_->seconds+=double(count)/8000;
    for(size_t i=0;i<impl_->trials.size();++i){
        if(impl_->selected>=0&&int(i)!=impl_->selected)continue;
        auto& t=*impl_->trials[i];t.decoder.Process(samples,count);unsigned char ch;
        double quality=t.decoder.SignalToNoiseRatio();if(!std::isfinite(quality))quality=0;
        t.quality+=(1-std::exp(-double(count)/16000))*(quality-t.quality);
        while(t.decoder.GetChar(ch)>0){
            t.recent+=char(ch);if(t.recent.size()>64)t.recent.erase(0,t.recent.size()-64);
            if(ch>=32&&ch<127)t.text+=char(ch);
            else if(ch==10||ch==13){if(t.text.empty()||t.text.back()!='\n')t.text+='\n';}
            else if(ch==9)t.text+='\t';
            if(t.text.size()>40000)t.text.erase(0,t.text.size()-32000);
        }
    }
    if(impl_->selected>=0||impl_->seconds<24)return;
    int best=-1;double score=0,second=0;
    for(size_t i=0;i<impl_->trials.size();++i){auto& t=*impl_->trials[i];if(!t.qualified())continue;
        if(t.quality>score){second=score;score=t.quality;best=int(i);}else second=std::max(second,t.quality);}
    if(best<0||score<second*1.5){impl_->pending=-1;return;}
    if(best!=impl_->pending){impl_->pending=best;impl_->pendingSince=impl_->seconds;}
    else if(impl_->seconds-impl_->pendingSince>=4)impl_->selected=best;
}
bool Olivia::locked() const{return impl_->selected>=0&&quality()>=3;}
std::string Olivia::text() const{return impl_->selected<0?"":impl_->trials[impl_->selected]->text;}
std::string Olivia::format() const{if(impl_->selected<0)return "Olivia";auto& t=*impl_->trials[impl_->selected];return "Olivia "+std::to_string(t.tones)+"/"+std::to_string(t.width);}
double Olivia::offset() const{return impl_->selected<0?0:impl_->trials[impl_->selected]->decoder.FrequencyOffset();}
double Olivia::quality() const{return impl_->selected<0?0:impl_->trials[impl_->selected]->decoder.SignalToNoiseRatio();}
}
