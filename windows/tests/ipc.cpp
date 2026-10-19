// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
#include "sharp_host.h"
#include <iostream>
#include <atomic>
int main(){
    notebook::SharpHost host;std::atomic<int> resets{0},cw{0},digital{0};
    host.consume=[&](notebook::SharpBlock block){
        if(block.header.at("kind")=="reset")++resets;
        else if(block.header.at("channel")=="cw")++cw;
        else ++digital;
    };
    host.start();
    while(host.alive)std::this_thread::sleep_for(std::chrono::milliseconds(5));
    host.finish();
    const bool passed=resets==3&&cw==2&&digital==2;
    std::cerr<<"resets="<<resets<<" cw="<<cw<<" digital="<<digital<<'\n';
    return passed?0:1;
}
