// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <limits>
#include <functional>
#include "radio_text.h"
namespace rtty {struct Station;struct Status;}

// GUI-thread-only view. Selection and scrolling never pause incoming text.
class TranscriptView {
public:
    using FlagProvider=std::function<ImTextureID(const std::string&)>;
    const std::vector<radio::CallTag>& callsigns(const std::string& text,const radio::TextOptions& options);
    void draw(const std::string& text, float height, float scale, bool& follow,const radio::TextOptions& options={},const FlagProvider& flags={});
    void clear();
    void copyAll(const std::string& text);
    void copyReport(double frequency, float wpm, float carrierOffset, bool manualSpeed,bool recovered=false,const std::string* currentText=nullptr,double snr=std::numeric_limits<double>::quiet_NaN(),bool snrLast=false,double lastReceivedUtc=0);
    void copyRTTYReport(const rtty::Station& station,const rtty::Status& capture,bool compact);
    bool popupOpen() const { return popupOpen_; }
    void hidden(){popupOpen_=false;}
private:
    static int selectionCallback(ImGuiInputTextCallbackData* data);
    void update(const std::string& text, float width);
    void paint(ImGuiID id,const FlagProvider& flags);
    struct InkRun {float first,last;int row;radio::Ink ink;std::string flag;size_t begin,end;};
    std::vector<InkRun> inkRuns_;
    radio::TextMarkup markup_;
    radio::TextOptions options_;
    std::string markupSource_;
    bool markupReady_=false,markupChanged_=false;
    std::string source_, selection_;
    std::vector<char> buffer_{'\0'};
    float width_=0,fontSize_=0;
    bool wasFollowing_=true;
    bool resetSelection_=false;
    bool popupOpen_=false;
    std::string copyNotice_;
    double copiedAt_=-10;
    void copied(const std::string& text, const char* kind);
};
