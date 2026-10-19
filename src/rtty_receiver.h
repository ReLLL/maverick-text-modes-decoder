// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_engine.h"
#if !CW_HOST_SDRSHARP
#include <dsp/sink/handler_sink.h>
#endif

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
#if CW_HOST_SDRSHARP
    bool inputWindows(const void* data,size_t count,double rate,double center){
        if(!requested_||!playing_)return false;
        engine_.configure(rate,center,true);return engine_.inputBytes(data,count);
    }
    void resetWindows(){engine_.configure(0,0,false);}
#endif
private:
    void sync();
    struct Stream;
#if !CW_HOST_SDRSHARP
    static void input(dsp::complex_t* samples,int count,void* context);
#endif
    rtty::Engine engine_;
    std::unique_ptr<Stream> stream_;
    double rate_=0,center_=0;
    rtty::Focus focus_;
#if CW_HOST_SDRSHARP
    std::atomic<bool> playing_{false},requested_{false};
#else
    bool playing_=false,requested_=false;
#endif
};
