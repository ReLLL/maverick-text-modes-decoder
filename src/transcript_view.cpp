// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "transcript_view.h"
#include "reception.h"
#include "notebook_style.h"
#include "rtty_report.h"
#include <imgui_internal.h>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>
#include <cmath>
#include <map>

void TranscriptView::clear(){
    source_.clear();selection_.clear();buffer_.assign(1,'\0');width_=0;resetSelection_=true;markupReady_=false;inkRuns_.clear();
}
const std::vector<radio::CallTag>& TranscriptView::callsigns(const std::string& text,const radio::TextOptions& options){
    if(!markupReady_||markupSource_!=text||!(options_==options)){
        if(!(options_==options))resetSelection_=true;
        auto next=radio::markText(text,options);
        // Later corroboration can remove an earlier flag. Discard stale display
        // indices instead of copying a different selection after that change.
        if(markupReady_&&next.display.compare(0,markup_.display.size(),markup_.display)!=0)resetSelection_=true;
        options_=options;markupSource_=text;markup_=std::move(next);markupReady_=true;markupChanged_=true;
    }
    return markup_.calls;
}
void TranscriptView::copied(const std::string& text,const char* kind){
    ImGui::SetClipboardText(text.c_str());
    copyNotice_=std::string("Copied ")+kind+" ("+std::to_string(text.size())+" characters).";
    copiedAt_=ImGui::GetTime();
}
void TranscriptView::copyAll(const std::string& text){copied(text,"all text");}
void TranscriptView::copyRTTYReport(const rtty::Station& station,const rtty::Status& capture,bool compact){
    const auto kind=std::string(rtty::modeName(station))+" reception details and displayed text";
    copied(rtty::receptionReport(station,capture,compact,std::time(nullptr)),kind.c_str());
}
void TranscriptView::copyReport(double frequency,float wpm,float carrierOffset,bool manualSpeed,bool recovered,const std::string* currentText,double snr,bool snrLast,double lastReceivedUtc){
    const auto& text=currentText?*currentText:source_;
    const auto now=std::time(nullptr);
    std::tm local{};
    localtime_r(&now,&local);
    char offset[16]{};
    std::strftime(offset,sizeof(offset),"%z",&local);
    std::string zone=offset;
    if(zone.size()==5)zone.insert(3,":");
    std::ostringstream report;
    report.imbue(std::locale::classic());
    report<<"CW reception\n"
          <<"Date/time:      "<<std::put_time(&local,"%Y-%m-%d %H:%M:%S")<<'\n'
          <<"Day:            "<<std::put_time(&local,"%A")<<'\n'
          <<"Time zone:      Local (UTC"<<zone<<")\n"
          <<"Frequency:      "<<std::fixed<<std::setprecision(3)<<frequency/1000<<" kHz\n"
          <<"Speed:          "<<std::setprecision(1)<<wpm<<" WPM ("<<(manualSpeed?"manual":"estimated")<<")\n"
          <<"Carrier offset: ";
    if(std::isfinite(carrierOffset))report<<std::showpos<<std::setprecision(0)<<carrierOffset<<std::noshowpos<<" Hz";
    else report<<"Unavailable (AM audio tone)";
    if(lastReceivedUtc>0)report<<"\nLast received:  "<<radio::receptionTimestamp(lastReceivedUtc);
    if(std::isfinite(snr))report<<"\nRX SNR:         "<<std::showpos<<std::setprecision(1)<<snr<<std::noshowpos<<" dB ("<<(snrLast?"last valid measurement; ":"")<<"estimated here; 2500 Hz reference)";
    else report<<"\nRX SNR:         Unavailable";
    report<<"\n\n"<<(recovered?"Timing + fade recovery (unverified)":"Original decode")<<"\n-------------\n"<<text;
    if(text.empty() || text.back()!='\n')report<<'\n';
    copied(report.str(),"reception details and displayed text");
}
void TranscriptView::update(const std::string& text,float width){
    if(!markupChanged_&&source_==text && width_==width && fontSize_==ImGui::GetFontSize())return;
    if(text.compare(0,source_.size(),source_)!=0 || width_!=width || fontSize_!=ImGui::GetFontSize())resetSelection_=true;
    if(resetSelection_)selection_.clear();
    source_=text;width_=width;fontSize_=ImGui::GetFontSize();buffer_.clear();markupChanged_=false;
    const auto& display=markup_.display;
    std::vector<size_t> mapping(display.size()+1);
    const char* p=display.data();const char* end=p+display.size();
    auto append=[&](const char* last){while(p<last){mapping[size_t(p-display.data())]=buffer_.size();buffer_.push_back(*p++);}};
    while(p<end){
        const char* paragraph=std::find(p,end,'\n');
        while(p<paragraph){
            const char* next=ImGui::GetFont()->CalcWordWrapPositionA(ImGui::GetFontSize()/ImGui::GetFont()->FontSize,p,paragraph,width);
            if(next<=p)next=p+1; // Decoder output is ASCII, including prosigns.
            append(next);
            if(p<paragraph){while(p<paragraph && *p==' '){mapping[size_t(p-display.data())]=buffer_.size();++p;}buffer_.push_back('\n');}
        }
        if(p<end){mapping[size_t(p-display.data())]=buffer_.size();buffer_.push_back('\n');++p;}
    }
    mapping.back()=buffer_.size();
    std::vector<radio::Ink> ink(buffer_.size(),radio::Ink::Plain);std::map<size_t,std::string> flags;
    for(const auto& span:markup_.spans){
        const auto first=mapping[span.begin],last=mapping[span.end];
        std::fill(ink.begin()+first,ink.begin()+last,span.ink);
        if(span.ink==radio::Ink::Flag&&std::find(buffer_.begin()+first,buffer_.begin()+last,'\n')==buffer_.begin()+last)flags[first]=span.flag;
    }
    inkRuns_.clear();float x=0;int row=0;
    for(size_t i=0;i<buffer_.size();){
        if(buffer_[i]=='\n'){x=0;++row;++i;continue;}
        const auto first=i;const auto type=ink[i];const float start=x;
        while(i<buffer_.size()&&buffer_[i]!='\n'&&ink[i]==type)x+=ImGui::GetFont()->GetCharAdvance(static_cast<unsigned char>(buffer_[i++]))*fontSize_/ImGui::GetFont()->FontSize;
        if(type!=radio::Ink::Plain)inkRuns_.push_back({start,x,row,type,flags.count(first)?flags[first]:"",first,i});
    }
    buffer_.push_back('\0');
}
int TranscriptView::selectionCallback(ImGuiInputTextCallbackData* data){
    auto& self=*static_cast<TranscriptView*>(data->UserData);
    int first=std::min(data->SelectionStart,data->SelectionEnd);
    int last=std::max(data->SelectionStart,data->SelectionEnd);
    self.selection_.assign(data->Buf+first,data->Buf+last);
    for(auto it=self.inkRuns_.rbegin();it!=self.inkRuns_.rend();++it)
        if(it->ink==radio::Ink::Flag&&!it->flag.empty()&&it->begin>=size_t(first)&&it->end<=size_t(last))
            self.selection_.replace(it->begin-size_t(first),it->end-it->begin,radio::unicodeFlag(it->flag));
    if(!self.selection_.empty() && ImGui::GetIO().KeySuper && ImGui::IsKeyPressed(ImGuiKey_C))self.copied(self.selection_,"selection");
    return 0;
}
void TranscriptView::draw(const std::string& text,float height,float scale,bool& follow,const radio::TextOptions& options,const FlagProvider& flags){
    callsigns(text,options);
    ImGui::PushID(this);
    ImGui::SetWindowFontScale(scale);
    const auto id=ImGui::GetID("##decoded_morse");
    if(follow && !wasFollowing_){
        if(ImGui::GetActiveID()==id)ImGui::ClearActiveID();
        if(auto* state=ImGui::GetInputTextState(id))state->ClearSelection();
        selection_.clear();
    }
    // Following controls scroll position, never delivery of new text.
    update(text,std::max(80.f,ImGui::GetContentRegionAvail().x-2*ImGui::GetStyle().FramePadding.x-ImGui::GetStyle().ScrollbarSize-8));
    if(resetSelection_){
        if(auto* state=ImGui::GetInputTextState(id))state->ClearSelection();
        resetSelection_=false;
    }
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(.045f,.050f,.054f,1));
    const bool macKeys=ImGui::GetIO().ConfigMacOSXBehaviors;
    ImGui::GetIO().ConfigMacOSXBehaviors=true;
    ImGui::InputTextMultiline("##decoded_morse",buffer_.data(),buffer_.size(),ImVec2(-1,height),
        ImGuiInputTextFlags_ReadOnly|ImGuiInputTextFlags_CallbackAlways,selectionCallback,this);
    ImGui::GetIO().ConfigMacOSXBehaviors=macKeys;
    const bool hovered=ImGui::IsItemHovered();
    paint(id,flags);
    if(hovered && (ImGui::GetIO().MouseWheel!=0 || (ImGui::GetActiveID()==id && !selection_.empty())))follow=false;
    popupOpen_=ImGui::BeginPopupContextItem("copy_morse");
    if(popupOpen_){
        if(ImGui::MenuItem("Copy selection","Cmd+C",false,!selection_.empty()))copied(selection_,"selection");
        notebook::hint("Copy only the selected displayed text to the clipboard.");
        if(ImGui::MenuItem("Copy all",nullptr,false,!text.empty()))copyAll(text);
        notebook::hint("Copy the whole displayed version without reception metadata. Use Copy to clipboard for a reception report with metadata.");
        ImGui::Separator();
        if(ImGui::MenuItem("Follow live text"))follow=true;
        notebook::hint("Scroll to the newest text. Reception has continued while you read earlier lines.");
        ImGui::EndPopup();
    }
    if(follow){
        // This is the host's existing multiline child, not a second text widget.
        // Internal headers are checked against the installed Brown ABI at build validation.
        for(auto* child:ImGui::GetCurrentWindow()->DC.ChildWindows)
            if(child->ChildId==id)ImGui::SetScrollY(child,child->ScrollMax.y);
    }
    ImGui::PopStyleColor();ImGui::SetWindowFontScale(1);
    if(ImGui::GetTime()-copiedAt_<3)ImGui::TextColored(ImVec4(.36f,.85f,.68f,1),"%s",copyNotice_.c_str());
    else ImGui::TextDisabled(follow?"Select text and press Cmd+C, or right-click for copy options.":"New text continues below. Enable Follow text to scroll to the latest.");
    wasFollowing_=follow;
    ImGui::PopID();
}
