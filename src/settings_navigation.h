// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <imgui.h>
#include <algorithm>
namespace notebook {
class SettingsNavigation {
public:
    int page=0;
    template<class Content>void draw(Content content){
        const char* names[]={"Text & callsigns","CW decoding","Monitoring","Diagnostics"};
        const float width=std::min(174.f,std::max(130.f,ImGui::GetContentRegionAvail().x*.25f));
        ImGui::BeginChild("settings_navigation",{width,0},true);
        ImGui::TextDisabled("MAVERICK");ImGui::Spacing();
        for(int i=0;i<4;i++)if(ImGui::Selectable(names[i],page==i,0,{0,ImGui::GetFrameHeight()+8}))page=i;
        ImGui::EndChild();ImGui::SameLine();
        ImGui::BeginChild("settings_page",{0,0},false);
        ImGui::TextUnformatted(names[page]);ImGui::Separator();ImGui::Spacing();content(page);
        ImGui::EndChild();
    }
};
}
