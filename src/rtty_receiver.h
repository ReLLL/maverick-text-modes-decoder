// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_engine.h"
#include <dsp/sink/handler_sink.h>

// Host lifecycle belongs on the GUI thread; DSP belongs in rtty::Engine.
class RttyReceiver {
public:
    RttyReceiver();
    ~RttyReceiver();
    void enabled(bool enabled);
    void modes(bool rtty,bool psk){engine_.modes(rtty,psk);}
    void fecModes(bool js8,bool olivia){engine_.fecModes(js8,olivia);}
    void configure(double rate,double center,bool playing);
    std::shared_ptr<const rtty::Status> snapshot() const{return engine_.snapshot();}
    void close(uint64_t id){engine_.close(id);}
    void closeAll(){engine_.closeAll();}
    void selection(rtty::Selection selection){engine_.selection(std::move(selection));}
    void focus(rtty::Focus focus){if(!(focus==focus_)){focus_=focus;engine_.focus(focus);}}
private:
    void sync();
    struct Stream;
    static void input(dsp::complex_t* samples,int count,void* context);
    rtty::Engine engine_;
    std::unique_ptr<Stream> stream_;
    double rate_=0,center_=0;
    rtty::Focus focus_;
    bool playing_=false,requested_=false;
};
