// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <windows.h>
#include <shellapi.h>
void CWNotebook::explainWithAI(int provider){
    auto s=engine_.snapshot();auto text=cw::transcriptText(s,showRecovery_);if(text.empty())return;
    const auto& d=showRecovery_?s.recoveredDecoder:s.decoder;const auto& rx=showRecovery_?s.recoveredReception:s.reception;
    cw::AnalysisContext context;context.frequency=frequency_;context.bandwidth=bandwidth_;context.wpm=d.wpm;
    context.carrierOffset=channel_.carrierOffset(d.tone);context.mode=mode_+(channel_.amAudio?" (audio tones)":"");
    context.recovered=showRecovery_;context.manualSpeed=s.manualWpm>0;context.timestamp=std::time(nullptr);context.grid=cw::host::grid(name_);
    context.snr=rx.snr;context.snrLast=!rx.currentSnr(playing_&&!s.paused&&d.signal);context.lastReceivedUtc=rx.lastTextUtc;
    openAnalysisPrompt(provider,cw::analysisPrompt(text,context));
}
void CWNotebook::openAnalysisPrompt(int provider,const std::string& prompt){
    auto handoff=cw::browserHandoff(provider,prompt);ImGui::SetClipboardText(prompt.c_str());
    auto path=std::filesystem::u8path(handoff.url);aiNoticeAt_=ImGui::GetTime();
    if(intptr_t(ShellExecuteW(nullptr,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32){aiNotice_="Could not open the browser. Full analysis prompt copied; paste it into your AI site.";return;}
    const char* names[]={"ChatGPT","Claude","Gemini"};
    aiNotice_=std::string("Opened ")+names[provider]+(handoff.prefilled?". Complete prompt copied too; paste it if the site does not prefill.":". Complete prompt copied: paste it into the browser and send.");
}
