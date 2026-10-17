// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "notebook_style.h"
#include <imgui_internal.h>
#include <initializer_list>

namespace notebook {
void closePopups(){
    auto* context=ImGui::GetCurrentContext();if(!context)return;
    auto* window=ImGui::FindWindowByName("CW Notebook###cw_notebook_window");if(!window)return;
    for(int i=0;i<context->OpenPopupStack.Size;++i){
        auto id=context->OpenPopupStack[i].PopupId;
        for(const char* title:{"CW Settings","How to use","Digital signals"}){
            if(id==ImHashStr(title,0,window->ID)){ImGui::ClosePopupToLevel(i,true);return;}
        }
    }
}
}
