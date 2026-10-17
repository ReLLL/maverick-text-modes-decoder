// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include <core.h>
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
void CWNotebook::explainWithAI(int provider){
    auto s=engine_.snapshot();auto text=cw::transcriptText(s,showRecovery_);
    if(text.empty())return;
    const auto& d=showRecovery_?s.recoveredDecoder:s.decoder;
    cw::AnalysisContext context;
    context.frequency=frequency_;context.bandwidth=bandwidth_;context.wpm=d.wpm;
    context.carrierOffset=channel_.carrierOffset(d.tone);context.mode=mode_+(channel_.amAudio?" (audio tones)":"");
    context.recovered=showRecovery_;context.manualSpeed=s.manualWpm>0;context.timestamp=std::time(nullptr);
    context.grid=cw::host::grid(name_);
    const auto& rx=showRecovery_?s.recoveredReception:s.reception;
    context.snr=rx.snr;context.snrLast=!rx.currentSnr(playing_&&!s.paused&&d.signal);context.lastReceivedUtc=rx.lastTextUtc;
    openAnalysisPrompt(provider,cw::analysisPrompt(text,context));
}
void CWNotebook::openAnalysisPrompt(int provider,const std::string& prompt){
    auto handoff=cw::browserHandoff(provider,prompt);
    ImGui::SetClipboardText(prompt.c_str());
    pid_t pid;char* args[]={const_cast<char*>("/usr/bin/open"),handoff.url.data(),nullptr};
    aiNoticeAt_=ImGui::GetTime();
    if(posix_spawn(&pid,"/usr/bin/open",nullptr,nullptr,args,environ)!=0){
        aiNotice_="Could not open the browser. The complete analysis prompt is copied; paste it into your AI site.";return;
    }
    std::thread([pid]{int status;while(waitpid(pid,&status,0)<0&&errno==EINTR){}}).detach();
    const char* names[]={"ChatGPT","Claude","Gemini"};
    aiNotice_=std::string("Opened ")+names[provider]+(handoff.prefilled?
        ". Complete prompt copied too; paste it if the site does not prefill.":
        ". Complete prompt copied: paste it into the browser and send.");
}
