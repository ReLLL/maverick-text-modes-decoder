// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "transcript_view.h"
#include "notebook_style.h"
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
namespace {
ImVec4 color(radio::Ink ink){
    switch(ink){
        case radio::Ink::Call:return {.53f,.78f,.98f,1};
        case radio::Ink::Calling:return {.98f,.76f,.38f,1};
        case radio::Ink::Identity:return {.50f,.82f,.76f,1};
        case radio::Ink::Report:return {.81f,.68f,.94f,1};
        case radio::Ink::Courtesy:return {.65f,.82f,.52f,1};
        default:return notebook::muted;
    }
}
}
void TranscriptView::paint(ImGuiID id,const FlagProvider& flags){
    if(inkRuns_.empty())return;
    ImGuiWindow* child=nullptr;
    for(auto* candidate:ImGui::GetCurrentWindow()->DC.ChildWindows)if(candidate->ChildId==id){child=candidate;break;}
    if(!child)return;
    const float lineHeight=ImGui::GetTextLineHeight();
    const float scrollX=ImGui::GetActiveID()==id&&ImGui::GetInputTextState(id)?ImGui::GetInputTextState(id)->ScrollX:0;
    const auto padding=ImGui::GetStyle().FramePadding;
    const ImVec2 origin{child->DC.CursorStartPos.x+padding.x-scrollX,child->Pos.y+padding.y-child->Scroll.y};
    auto* draw=child->DrawList;
    // Recolor the host's actual glyph quads: selection, hit testing, scrolling
    // and copy remain in its original read-only InputTextMultiline widget.
    for(int i=0;i+5<draw->IdxBuffer.Size;){
        const auto base=draw->IdxBuffer[i];
        if(draw->IdxBuffer[i+1]!=base+1||draw->IdxBuffer[i+2]!=base+2||draw->IdxBuffer[i+3]!=base||draw->IdxBuffer[i+4]!=base+2||draw->IdxBuffer[i+5]!=base+3||base+3>=draw->VtxBuffer.Size){++i;continue;}
        i+=6;auto* v=&draw->VtxBuffer[base];
        if(v[0].uv.x==v[2].uv.x&&v[0].uv.y==v[2].uv.y)continue; // Filled shapes, selection and caret.
        const float x=(v[0].pos.x+v[2].pos.x)/2-origin.x;
        const int row=int(std::floor(((v[0].pos.y+v[2].pos.y)/2-origin.y)/lineHeight));
        auto run=std::lower_bound(inkRuns_.begin(),inkRuns_.end(),row,[](const auto& run,int r){return run.row<r;});
        for(;run!=inkRuns_.end()&&run->row==row;++run)if(x>=run->first&&x<run->last){
            const auto hidden=run->ink==radio::Ink::Flag&&flags&&!run->flag.empty()&&flags(run->flag);
            const auto ink=hidden?0:ImGui::GetColorU32(color(run->ink));
            for(int n=0;n<4;n++)v[n].col=ink;break;
        }
    }
    if(flags){
        draw->PushClipRect(child->InnerClipRect.Min,child->InnerClipRect.Max,false);
        for(const auto& run:inkRuns_)if(run.ink==radio::Ink::Flag&&!run.flag.empty()){
            const float y=origin.y+run.row*lineHeight;
            if(y+lineHeight<child->InnerClipRect.Min.y||y>child->InnerClipRect.Max.y)continue;
            const auto texture=flags(run.flag);if(!texture)continue;
            const float width=std::min(run.last-run.first,lineHeight*1.5f);
            const float x=origin.x+run.first+(run.last-run.first-width)/2;
            draw->AddImage(texture,{x,y},{x+width,y+lineHeight});
        }
        draw->PopClipRect();
    }
}
