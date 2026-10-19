// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <json.hpp>
#include <complex>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>
#include <filesystem>
namespace notebook {
using Json=nlohmann::json;
struct SharpBlock {Json header;std::vector<std::complex<float>> iq;};
struct SharpState {double center=0,frequency=0,rate=0,width=500;std::string mode="CW";bool playing=false;uint64_t epoch=0;};
class SharpHost {
public:
    std::filesystem::path root,binary;
    std::atomic<bool> alive{true},show{true};
    std::function<void(SharpBlock)> consume;
    void start();void finish();void plan(Json value);void request(Json value);
    SharpState state(){std::lock_guard<std::mutex> lock(mutex_);return state_;}
private:
    void read();void write();
    std::thread reader_,writer_;std::mutex mutex_;std::condition_variable wake_;
    SharpState state_;Json plan_,request_;uint64_t requestId_=0;bool dirty_=false;
    std::atomic<unsigned long> readerId_{0},writerId_{0};
    std::atomic<bool> readerDone_{false},writerDone_{false};
};
SharpHost& sharpHost();
}
