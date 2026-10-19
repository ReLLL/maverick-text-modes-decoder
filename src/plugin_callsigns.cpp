// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "notebook_style.h"
#include <algorithm>
void CWNotebook::callTags(TranscriptView& view,const std::string& text,const std::string& headerCall,bool checkExtensions){
    if(!textOptions_.detectCallsigns)return;
    auto options=textOptions_;options.checkExtensions=checkExtensions;
    auto calls=view.callsigns(text,options);
    if(!headerCall.empty()&&radio::looksLikeCallsign(headerCall)){
        calls.erase(std::remove_if(calls.begin(),calls.end(),[&](auto& tag){return tag.call==headerCall;}),calls.end());
        calls.push_back({headerCall,radio::callsignCountry(headerCall)});
    }
    ImGui::BeginChild("callsign_tags",{0,ImGui::GetFrameHeight()+ImGui::GetStyle().ScrollbarSize+6},false,ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::AlignTextToFramePadding();ImGui::TextDisabled("Callsigns");ImGui::SameLine();
    if(calls.empty())ImGui::TextDisabled("Appear as text is received");
    for(auto it=calls.rbegin();it!=calls.rend();++it){
        ImGui::PushID(it->call.c_str());const auto pos=ImGui::GetCursorScreenPos();
        const auto size=ImGui::GetTextLineHeight();const auto label=ImGui::CalcTextSize(it->call.c_str());
        const ImVec2 bounds{label.x+size*1.5f+26,ImGui::GetFrameHeight()};
        const bool clicked=ImGui::Button("##callsign",bounds);
        auto* draw=ImGui::GetWindowDrawList();const bool hover=ImGui::IsItemHovered();
        draw->AddRectFilled(pos,{pos.x+bounds.x,pos.y+bounds.y},ImGui::GetColorU32(hover?ImVec4{.23f,.29f,.33f,1}:ImVec4{.13f,.18f,.21f,1}),5);
        const float y=pos.y+(bounds.y-size)/2;const auto texture=flags_.get(it->country.flag);
        if(texture)draw->AddImage(reinterpret_cast<ImTextureID>(uintptr_t(texture)),{pos.x+7,y},{pos.x+7+size*1.5f,y+size});
        draw->AddText({pos.x+size*1.5f+13,y},ImGui::GetColorU32(ImVec4{.63f,.83f,.97f,1}),it->call.c_str());
        if(hover){ImGui::BeginTooltip();ImGui::Text("%s | %s",it->call.c_str(),it->country.name.empty()?"Country unknown":it->country.name.c_str());ImGui::TextUnformatted("Prefix estimate. Click to look up this callsign on QRZ.com.");ImGui::EndTooltip();}
        if(clicked){const auto url=radio::qrzUrl(it->call);if(!url.empty())openPath(url,false);}
        ImGui::PopID();ImGui::SameLine();
    }
    ImGui::EndChild();
}
