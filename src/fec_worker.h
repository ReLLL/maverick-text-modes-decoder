// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "fec_decoder.h"
#include "rtty_fft.h"
#include <array>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <thread>
namespace fec {
struct Packet {
    uint64_t key=0,generation=0,sequence=0;
    bool js8=false;
    double frequency=0,rate=0,utc=0,snr=std::numeric_limits<double>::quiet_NaN();
    std::array<rtty::IQ,128> samples{};
};
struct Received {uint64_t key;bool js8;double center;Frame frame;};
class Worker {
public:
    Worker();~Worker();
    void reset();
    void keys(std::vector<uint64_t> keys);
    void input(Packet packet);
    std::vector<Received> take();
    struct Stats {uint64_t dropped=0;double cpuMs=0;std::string error;};
    Stats stats();
    void waitIdle();
private:
    struct Channel;
    void work();
    std::mutex mutex_;std::condition_variable wake_,idle_;
    std::array<Packet,256> queue_{};size_t read_=0,write_=0,pending_=0;
    std::vector<Received> received_;
    std::vector<uint64_t> keys_;
    bool stop_=false,busy_=false,reset_=false,keysDirty_=false;
    uint64_t generation_=0;
    Stats stats_;
    std::thread thread_;
};
}
