// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "decoder.h"
#include "journal.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <fstream>

namespace cw {
struct Snapshot {
    DecoderStats decoder;
    std::string text, recoveredText, path, error, notice;
    DecoderStats recoveredDecoder;
    radio::Reception reception,recoveredReception;
    bool timingRecovery=true, autoRetune=true, extendedSpeed=false,amAudio=false;
    double frequency=0;
    uint64_t blocks=0, dropped=0, saved=0, transcriptReset=0, pauseRevision=0;
    bool paused=false, autosave=true;
    float manualWpm=0, manualTone=0;
};
std::string transcriptText(const Snapshot& snapshot,bool recovered);
class Engine {
public:
    explicit Engine(const std::filesystem::path& folder);
    ~Engine();
    void audio(std::vector<float> samples,int rate,double frequency);
    void command(std::string name,std::string value="");
    Snapshot snapshot();
private:
    struct Packet {std::vector<float> samples;int rate;double frequency;uint64_t generation;};
    struct Command {std::string name,value;};
    void work();
    void apply(const Command& cmd);
    void decoded(const std::string& text,bool recovered=false);
    void resetDecoders();
    void stationChanged(double frequency);
    void flush();
    void persist();
    void preferences();
    std::mutex queueMutex_,viewMutex_;
    std::condition_variable cv_;
    std::deque<Packet> queue_;
    std::deque<Command> commands_;
    size_t queuedSamples_=0;
    std::atomic<uint64_t> dropped_{0}, generation_{0};
    std::atomic<bool> stop_{false};
    std::thread thread_;
    Decoder decoder_, recoveredDecoder_;
    Journal journal_;
    Snapshot view_;
    std::filesystem::path folder_;
    struct Segment {std::string text;double frequency;float wpm;std::string recovered;};
    std::deque<Segment> pending_;
    size_t unsavedBytes_=0;
    bool paused_=false,autoSave_=true,transcriptBreakPending_=false;
    uint64_t pauseRevision_=0;
    bool discardCurrentPacket_=false;
    bool preferencesDirty_=false;
    bool timingRecovery_=true,autoRetune_=true,recoveredBreakPending_=false;
    bool extendedSpeed_=false;
    bool wideInput_=false;
    bool amAudio_=false;
    float manualWpm_=0,manualTone_=0;
    double frequency_=0;
    radio::Reception reception_,recoveredReception_;
    int rate_=0;
    uint64_t lastGeneration_=0,blocks_=0,saved_=0;
    std::ofstream diagnostic_;
    size_t diagnosticRemaining_=0;
};
}
