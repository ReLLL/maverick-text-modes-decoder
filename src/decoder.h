// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "run_buffer.h"
#include "reception.h"
#include <cstdint>
#include <array>
#include <complex>
#include <deque>
#include <functional>
#include <string>
#include <vector>
#include <limits>

namespace cw {
struct DecoderStats {
    float tone = 800, wpm = 0, snr = 0, level = 0, threshold = 0;
    bool keyed = false, calibrated = false, signal = false;
    uint64_t filteredGlitches = 0, recoveries = 0;
    size_t bufferedRuns = 0;
    std::string provisional;
    double snrDb=std::numeric_limits<double>::quiet_NaN();
    radio::Reception reception;
    float provisionalWpm = 0;
};
class Timing {
public:
    std::function<void(const std::string&)> output;
    void reset(float manualWpm = 0);
    void setSpeed(float manualWpm);
    void extendedSpeed(bool enabled);
    void tick(bool key, double seconds = .001);
    void recovery(bool enabled) { recovery_=enabled; }
    uint64_t glitches() const { return glitches_; }
    float wpm() const { return float(1.2 / unit_); }
    bool calibrated() const { return ready_; }
    size_t bufferedRuns() const { return pending_.size(); }
    const std::string& provisional() const { return provisional_; }
    float provisionalWpm() const { return provisionalWpm_; }
private:
    using Run = KeyRun;
    void estimate();
    double recentUnit(const std::vector<double>& marks, bool relock = false) const;
    void acquire();
    void replay();
    void preview(double seconds);
    void consume(const Run& run);
    void letter();
    void advance(bool key,double seconds);
    bool key_ = false, started_ = false, ready_ = false, letterSent_ = false, wordSent_ = false;
    double duration_ = 0, unit_ = .06, cutoff_ = .12, manual_ = 0;
    std::deque<double> marks_, gaps_;
    RunBuffer pending_;
    std::string symbol_;
    bool recovery_=true, filteredKey_=false;
    double transition_=0, gapCutoff_=2;
    uint64_t glitches_=0;
    double acquisitionUnit_=0, acquisitionCutoff_=0;
    double relockUnit_=0;
    uint64_t markSerial_=0,relockSerial_=0;
    unsigned relockVotes_=0;
    bool extendedSpeed_=false;
    double learningSeconds_=0,previewSeconds_=0;
    float provisionalWpm_=0;
    std::string provisional_;
};
class Decoder {
public:
    std::function<void(const std::string&)> output;
    std::function<void(bool,float,float)> trace;
    Decoder();
    void reset(float manualWpm = 0, float manualTone = 0);
    void setSpeed(float manualWpm);
    void extendedSpeed(bool enabled) { extendedSpeed_=enabled;timing_.extendedSpeed(enabled); }
    void recovery(bool enabled) { recovery_=enabled;timing_.recovery(enabled); }
    void wideInput(bool enabled) { wideInput_=enabled; }
    void process(const float* mono, size_t count, int sampleRate);
    DecoderStats stats() const { return stats_; }
private:
    void sample(float x);
    void findTone();
    void envelope(float x);
    Timing timing_;
    DecoderStats stats_;
    std::array<float, 16384> audio_{};
    std::array<float, 4096> levels_{};
    size_t audioPos_ = 0, levelPos_ = 0, samples_ = 0, ticks_ = 0;
    float manualTone_ = 0, noise_ = 0, peak_ = 0, low1_ = 0, low2_ = 0;
    float iqI_ = 0, iqQ_ = 0, i2_ = 0, q2_ = 0, sum_ = 0;
    double phase_ = 0, resample_ = 0;
    bool key_ = false, toneLocked_ = false;
    int debounce_ = 0, rate_ = 0;
    float candidateTone_ = 0;
    int stableTones_ = 0;
    size_t lastToneTick_ = 0;
    bool recovery_=true, quietReset_=false, extendedSpeed_=false;
    float manualWpm_=0;
    bool wideInput_=false;
    int analysisRate_=4000;
    size_t quietTicks_=0;
};
std::string morse(const std::string& symbol);
}
