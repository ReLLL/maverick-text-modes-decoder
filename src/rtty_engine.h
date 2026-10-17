// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_skimmer.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <optional>

namespace rtty {
// One optional worker, fixed pool, immutable four-Hz UI snapshots. The host
// callback never waits for DSP or allocates input buffers.
class Engine {
public:
    static constexpr size_t packetSize=262144,poolSize=4;
    Engine();
    ~Engine();
    void configure(double rate,double center,bool receiving);
    bool input(const IQ* data,size_t count);
    bool inputBytes(const void* data,size_t count);
    std::shared_ptr<const Status> snapshot() const;
    void close(uint64_t id);
    void closeAll();
    void selection(Selection selection);
    void focus(Focus focus);
    void modes(bool rtty,bool psk);
    void fecModes(bool js8,bool olivia);
private:
    struct Packet {std::vector<IQ> data;size_t count=0;uint64_t generation=0,gaps=0;};
    void work();
    void publish(const std::string& error="",bool receiving=false);
    mutable std::mutex mutex_;
    std::condition_variable wake_;
    std::array<Packet,poolSize> pool_;
    size_t read_=0,write_=0,pending_=0,samples_=0;
    double rate_=0,center_=0;
    bool receiving_=false,stop_=false,clear_=false,failed_=false;
    uint64_t generation_=0;
    std::vector<uint64_t> closes_;
    std::optional<Selection> selection_;
    std::optional<Focus> focus_;
    std::optional<std::pair<bool,bool>> modes_;
    std::optional<std::pair<bool,bool>> fecModes_;
    std::atomic<uint64_t> dropped_{0},gaps_{0};
    std::shared_ptr<const Status> view_=std::make_shared<const Status>();
    Skimmer skimmer_;
    double processingMs_=0;
    std::thread thread_;
};
}
