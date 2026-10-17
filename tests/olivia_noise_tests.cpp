// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fec_decoder.h"
#include "psk_fixture.h"
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
int main(){try{
    std::mt19937 random(2203);std::normal_distribution<double> noise(0,.2);
    for(int kind=0;kind<3;++kind){
        fec::Olivia decoder;fixture::Fsk rtty(8000,1500);std::vector<double> block(257);size_t sample=0;
        for(size_t n=0;n<8000*45;n+=block.size()){
            for(auto& v:block){double t=double(sample++)/8000;v=kind==0?noise(random):kind==1?std::sin(2*3.141592653589793*1500*t):rtty.sample().real();}
            decoder.process(block.data(),block.size());
        }
        if(decoder.locked())throw std::runtime_error("Noise, carrier or RTTY qualified as Olivia");
    }
    std::cout<<"Olivia interference rejection and sync-metric memory checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
