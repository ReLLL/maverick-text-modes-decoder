// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <ctime>
#include <string>
#include <limits>
namespace cw {
struct AnalysisContext {
    double frequency=0,bandwidth=0,carrierOffset=0;
    float wpm=0;
    bool recovered=false,manualSpeed=false;
    std::string mode,grid;
    std::time_t timestamp=0;
    double snr=std::numeric_limits<double>::quiet_NaN();
    bool snrLast=false;
    double lastReceivedUtc=0;
};
struct BrowserHandoff {std::string url;bool prefilled=false;};
std::string analysisPrompt(const std::string& transcript,const AnalysisContext& context);
BrowserHandoff browserHandoff(int provider,const std::string& prompt);
}
