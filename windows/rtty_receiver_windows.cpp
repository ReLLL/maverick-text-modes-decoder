// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "rtty_receiver.h"
struct RttyReceiver::Stream {};
RttyReceiver::RttyReceiver()=default;
RttyReceiver::~RttyReceiver(){engine_.configure(0,0,false);}
void RttyReceiver::enabled(bool value){requested_=value;if(!value)engine_.configure(rate_,center_,false);}
void RttyReceiver::configure(double rate,double center,bool playing){
    rate_=rate;center_=center;playing_=playing;
    if(!playing||!requested_)engine_.configure(rate,center,false);
}
void RttyReceiver::sync(){}
