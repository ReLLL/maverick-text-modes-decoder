// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "transcript_view.h"
#include "window_presentation.h"
#include "notebook_style.h"
#include "rtty_skimmer.h"
#include "cw_tab.h"
#include "reception_view.h"
#include "settings_navigation.h"
#include <imgui_internal.h>
#include <iostream>
#include <stdexcept>

// Brown's widget registry is unrelated to transcript rendering.
namespace ImGui {void sdrcppRegisterWidget(ImGuiID,int,const ImRect&) {}}

std::string clipboard;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
    ImGui::CreateContext();auto& io=ImGui::GetIO();
    const ImGuiStyle hostStyle=ImGui::GetStyle();
    const int hostColors=GImGui->ColorStack.Size,hostVars=GImGui->StyleVarStack.Size;
    {notebook::Style notebookStyle;require(ImGui::GetStyle().Colors[ImGuiCol_CheckMark].x==notebook::brass.x,"Notebook theme missing");}
    require(GImGui->ColorStack.Size==hostColors&&GImGui->StyleVarStack.Size==hostVars,"Notebook theme leaked style stack entries into host");
    for(int i=0;i<ImGuiCol_COUNT;++i){
        const auto a=hostStyle.Colors[i],b=ImGui::GetStyle().Colors[i];
        require(a.x==b.x&&a.y==b.y&&a.z==b.z&&a.w==b.w,"Notebook changed the host's theme");
    }
    require(ImGui::GetStyle().WindowPadding.x==hostStyle.WindowPadding.x&&ImGui::GetStyle().FramePadding.y==hostStyle.FramePadding.y,"Notebook changed host geometry");
    io.DisplaySize={1000,800};io.DeltaTime=1.f/60;io.IniFilename=nullptr;
    io.SetClipboardTextFn=[](void*,const char* s){clipboard=s;};
    io.GetClipboardTextFn=[](void*){return clipboard.c_str();};
    unsigned char* pixels;int w,h;io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
    TranscriptView view;bool follow=true;ImVec2 textPoint;ImGuiID inputID=0;
    auto frame=[&](const std::string& text,const radio::TextOptions& options=radio::TextOptions{},const TranscriptView::FlagProvider& flags=TranscriptView::FlagProvider{}){
        ImGui::NewFrame();ImGui::SetNextWindowPos({10,10});ImGui::SetNextWindowSize({800,600});
        ImGui::Begin("Transcript test",nullptr,ImGuiWindowFlags_NoSavedSettings);
        ImGui::PushID(&view);inputID=ImGui::GetID("##decoded_morse");ImGui::PopID();
        view.draw(text,300,1,follow,options,flags);
        for(auto* child:ImGui::GetCurrentWindow()->DC.ChildWindows)
            if(child->ChildId==inputID)textPoint={child->InnerRect.Min.x+15,child->InnerRect.Min.y+10};
        ImGui::End();ImGui::Render();
    };
    auto shown=[&](const char* token){view.copyReport(7020000,20,0,false);return clipboard.find(token)!=std::string::npos;};
    frame("CQ CQ");frame("CQ CQ");
    io.MousePos=textPoint;io.MouseDown[0]=true;frame("CQ CQ");
    io.MouseDown[0]=false;frame("CQ CQ DE TEST");frame("CQ CQ DE TEST");
    require(shown("DE TEST"),"Clicking transcript froze incoming text");
    require(follow,"A plain click should not disable auto-scroll");
    auto* state=ImGui::GetInputTextState(inputID);require(state!=nullptr,"Text widget was not active");
    state->Stb.select_start=0;state->Stb.select_end=2;
    frame("CQ CQ DE TEST NEXT");
    require(state->HasSelection()&&state->Stb.select_end==2,"Appending text lost selection");
    require(state->CurLenA==18,"Active ImGui buffer did not refresh");
    require(!follow,"Selection should hold scroll position");
    follow=true;frame("CQ CQ DE TEST NEXT");frame("CQ CQ DE TEST NEXT");
    require(follow,"Resuming auto-scroll immediately held the view again");
    follow=false;frame("CQ CQ DE TEST NEXT");
    require(shown("NEXT"),"Disabling follow froze content instead of only scrolling");
    io.MouseWheel=1;frame("CQ CQ DE TEST NEXT MORE");io.MouseWheel=0;
    require(shown("MORE"),"Scrolling froze incoming text");
    view.clear();follow=true;frame("NEW STATION");
    require(shown("NEW STATION")&&!shown("DE TEST"),"Clearing retained old transcript");
    const std::string hiddenUpdate="NEW STATION WHILE SETTINGS OPEN";
    view.copyReport(7020000,12,-20,false,false,&hiddenUpdate);
    require(clipboard.find("WHILE SETTINGS OPEN")!=std::string::npos,"Copy report omitted text received on another tab");
    rtty::Station rttyStation;rttyStation.frequency=10100580;rttyStation.center=10100805;
    rttyStation.baud=50;rttyStation.shift=450;rttyStation.reverse=true;rttyStation.text="RYRYRYRYRYRY\nCQ DE TEST";
    rtty::Status rttyCapture;rttyCapture.rate=3200000;rttyCapture.center=10114000;
    view.copyRTTYReport(rttyStation,rttyCapture,true);
    require(clipboard.find("RTTY reception")!=std::string::npos&&clipboard.find("10100.580 kHz")!=std::string::npos&&clipboard.find("30 m")!=std::string::npos,"RTTY clipboard omitted metadata");
    require(clipboard.find("[RY test pattern]")!=std::string::npos&&clipboard.find("CQ DE TEST")!=std::string::npos,"RTTY clipboard did not copy the displayed view");
    view.copyRTTYReport(rttyStation,rttyCapture,false);
    require(clipboard.find(rttyStation.text)!=std::string::npos,"Original RTTY clipboard lost received characters");
    rttyStation.psk=true;rttyStation.baud=31.25;rttyStation.shift=0;
    view.copyRTTYReport(rttyStation,rttyCapture,true);
    require(clipboard.find("PSK31 reception")!=std::string::npos&&clipboard.find("estimated carrier")!=std::string::npos,"PSK clipboard omitted mode/frequency metadata");
    require(clipboard.find(rttyStation.text)!=std::string::npos&&clipboard.find("[RY test pattern]")==std::string::npos,"PSK clipboard applied RTTY compaction");
    require(clipboard.find("ITA2")==std::string::npos&&clipboard.find("Polarity:")==std::string::npos,"PSK clipboard retained RTTY fields");
    rttyStation.psk=false;rttyStation.mode="JS8";rttyStation.format="JS8 Slow";rttyStation.call="K1ABC";rttyStation.grid="FN42";rttyStation.active=true;rttyStation.snr=-18;
    view.copyRTTYReport(rttyStation,rttyCapture,true);
    for(const char* value:{"JS8 Slow","K1ABC","FN42","-18.0 dB","2500 Hz","RYRYRYRYRYRY"})require(clipboard.find(value)!=std::string::npos,"JS8 clipboard lost metadata or literal text");
    rttyStation.mode="Olivia";rttyStation.format="Olivia 8/250";view.copyRTTYReport(rttyStation,rttyCapture,true);
    require(clipboard.find("Olivia 8/250")!=std::string::npos&&clipboard.find("Polarity:")==std::string::npos,"Olivia report used RTTY fields");
    view.copyReport(7020000,12,-20,false,false,&hiddenUpdate,-7.4);
    require(clipboard.find("-7.4 dB")!=std::string::npos,"CW clipboard omitted measured SNR");
    // The same presentation helper used on plugin enable and sidebar reopening
    // must recover a hidden/off-screen window after the host display shrinks.
    bool present=false;
    auto windowFrame=[&](bool request){
        ImGui::NewFrame();
        if(request){present=true;prepareNotebookWindow(present);}
        else {ImGui::SetNextWindowPos({1700,1200});ImGui::SetNextWindowSize({870,620});}
        ImGui::Begin("Notebook placement");ImGui::TextUnformatted("Transcript");ImGui::End();
        ImGui::Begin("Other panel");ImGui::End();
        ImGui::Render();
    };
    windowFrame(false);io.DisplaySize={640,480};windowFrame(true);
    auto* notebook=ImGui::FindWindowByName("Notebook placement");
    require(!present,"Presentation request was not consumed");
    require(notebook->Pos.x>=0&&notebook->Pos.y>=0&&
        notebook->Pos.x+notebook->Size.x<=640&&notebook->Pos.y+notebook->Size.y<=480,
        "Reopened window remained outside visible area");
    require(GImGui->NavWindow==notebook,"Reopened window was not focused");
    require(GImGui->Windows.back()==notebook,"Reopened window was not brought to front");
    // Standard SDR++ lacks Brown's draw event. Draw during EndFramePre with
    // no sidebar/main window, then remove the hook before module destruction.
    struct HookView {TranscriptView view;std::string text="CQ DE HOOK";bool follow=true;int frames=0;} hooked;
    ImGuiContextHook hook;hook.Type=ImGuiContextHookType_EndFramePre;hook.UserData=&hooked;
    hook.Callback=[](ImGuiContext*,ImGuiContextHook* h){
        auto& v=*static_cast<HookView*>(h->UserData);++v.frames;
        ImGui::Begin("Hook transcript");v.view.draw(v.text,200,1,v.follow);ImGui::End();
    };
    auto hookID=ImGui::AddContextHook(ImGui::GetCurrentContext(),&hook);
    ImGui::NewFrame();ImGui::Render();hooked.text+=" NEXT";
    ImGui::NewFrame();ImGui::Render();
    require(hooked.frames==2,"SDR++ frame hook did not draw without a sidebar");
    hooked.view.copyReport(7020000,20,0,false);
    require(clipboard.find("HOOK NEXT")!=std::string::npos,"Frame-hook transcript failed to refresh");
    ImGui::RemoveContextHook(ImGui::GetCurrentContext(),hookID);
    ImGui::NewFrame();ImGui::Render();
    require(hooked.frames==2,"Removed frame hook still invoked destroyed-module callback");
    auto popupFrame=[&](const char* owner,const char* title){
        ImGui::NewFrame();ImGui::Begin(owner);ImGui::OpenPopup(title);
        if(ImGui::BeginPopupModal(title)){ImGui::TextUnformatted("Modal test");ImGui::EndPopup();}
        ImGui::End();ImGui::Render();
    };
    popupFrame("CW Notebook###cw_notebook_window","Settings###cw_notebook_settings");
    require(GImGui->OpenPopupStack.Size==1,"Notebook modal fixture did not open");
    notebook::closePopups();require(GImGui->OpenPopupStack.empty(),"Disabled notebook left a modal blocking host input");
    popupFrame("Other plugin","How to use");notebook::closePopups();
    require(GImGui->OpenPopupStack.Size==1,"Notebook closed another plugin's modal");
    ImGui::ClosePopupToLevel(0,true);
    {
        notebook::CWTab cw;
        cw.request(false,0);cw.sync(false,0);
        require(!cw.enabled(),"Old worker snapshot reopened CW");
        cw.request(true,0);cw.sync(true,1);
        require(cw.enabled(),"Off acknowledgement undid a newer CW on request");
        cw.sync(false,2);require(cw.enabled(),"CW resume acknowledgement hid its tab");
        bool sawClose=false,sawDigital=false,sawCW=false;ImVec2 closePoint{};
        auto cwFrame=[&](bool select){
            ImGui::NewFrame();ImGui::SetNextWindowPos({10,10});ImGui::SetNextWindowSize({800,500});
            ImGui::Begin("CW close control",nullptr,ImGuiWindowFlags_NoSavedSettings);
            if(ImGui::BeginTabBar("receive tabs")){
                bool closed=false;const bool visible=cw.begin(select,closed);
                if(cw.open()){
                    auto min=ImGui::GetItemRectMin(),max=ImGui::GetItemRectMax();
                    closePoint={max.x-2*ImGui::GetStyle().FramePadding.x-ImGui::GetFontSize()/2,min.y+ImGui::GetFontSize()/2};
                }
                if(closed){sawClose=true;cw.request(false,2);}
                if(visible){sawCW=true;ImGui::TextUnformatted("Retained CW text");ImGui::EndTabItem();}
                if(ImGui::BeginTabItem("14085.125 kHz RTTY")){sawDigital=true;ImGui::EndTabItem();}
                ImGui::EndTabBar();
            }
            ImGui::End();ImGui::Render();
        };
        cwFrame(true);cwFrame(false);io.MousePos=closePoint;cwFrame(false);
        io.MouseDown[0]=true;cwFrame(false);io.MouseDown[0]=false;cwFrame(false);cwFrame(false);
        require(sawClose&&!cw.enabled(),"CW tab X did not disable CW");
        require(sawDigital,"Closing CW hid the digital tab");
        sawCW=false;cwFrame(true);require(!sawCW,"Selecting CW while disabled reopened it");
        cw.sync(false,2);require(!cw.enabled(),"Pending pause reopened the closed tab");
        cw.sync(true,3);cw.request(true,3);cw.sync(true,3);
        require(cw.enabled(),"CW on did not immediately reopen its tab");
        cw.sync(false,4);sawCW=false;cwFrame(true);cwFrame(false);
        require(sawCW,"Re-enabled CW tab did not show its contents");
        cw.sync(true,4);require(!cw.enabled(),"Worker safety pause did not disable CW");
        sawCW=false;cwFrame(false);require(sawCW,"Worker safety pause hid log recovery controls");
    }
    // Status changes draw inside a fixed footer, with no exterior margin and
    // no added rows before the tab strip. Test both selected and inactive dots.
    ImVec2 fixedTab[5]{};bool haveTab[5]{};
    for(int pass=0;pass<4;++pass){
        ImGui::NewFrame();ImGui::SetNextWindowPos({10,10});ImGui::SetNextWindowSize({1100,600});
        ImGui::Begin("RTTY status layout",nullptr,ImGuiWindowFlags_NoSavedSettings);
        bool cwEnabled=true,rttyEnabled=true,pskEnabled=true;
        ImGui::Checkbox("CW",&cwEnabled);ImGui::SameLine();
        ImGui::Checkbox("RTTY",&rttyEnabled);ImGui::SameLine();ImGui::Checkbox("PSK31/63",&pskEnabled);
        bool js8=true,olivia=true;ImGui::SameLine();ImGui::Checkbox("JS8",&js8);ImGui::SameLine();ImGui::Checkbox("Olivia",&olivia);
        if(ImGui::BeginTabBar("activity tabs")){
            for(int tab=0;tab<5;++tab){
                const char* captions[]{"CW","    10100.580 kHz RTTY","    14071.234 kHz PSK31","    14079.500 kHz JS8","    14072.500 kHz Olivia"};
                bool selected=ImGui::BeginTabItem(captions[tab]);
                if(tab){
                    auto* draw=ImGui::GetWindowDrawList();int start=draw->VtxBuffer.Size;
                    notebook::tabActivity(pass%2==0);
                    if(pass>0)require(draw->VtxBuffer.Size>start,"Inactive RTTY tab did not draw its filled activity circle");
                    const auto wanted=ImGui::GetColorU32(pass%2==0?ImVec4{.28f,.86f,.39f,1}:ImVec4{.98f,.78f,.20f,1});
                    bool color=false;for(int v=start;v<draw->VtxBuffer.Size;++v)if(draw->VtxBuffer[v].col==wanted)color=true;
                    if(pass>0)require(color,"RTTY circle used the wrong activity color");
                    auto point=ImGui::GetItemRectMin();
                    if(pass>0){
                        if(haveTab[tab])require(point.x==fixedTab[tab].x&&point.y==fixedTab[tab].y,"Status changes shifted frequency tabs");
                        fixedTab[tab]=point;haveTab[tab]=true;
                    }
                }
                if(selected){ImGui::TextUnformatted("Received text");ImGui::EndTabItem();}
            }
            ImGui::EndTabBar();
        }
        auto* draw=ImGui::GetWindowDrawList();int start=draw->VtxBuffer.Size;
        notebook::statusBar(pass%2?"RTTY monitoring | 16/16 decoders | 8 waiting | 123 gaps":"RTTY monitoring", "Detailed status",pass%2);
        require(draw->VtxBuffer[start].pos.x==10&&draw->VtxBuffer[start].pos.y==610-notebook::statusHeight(),"Status bar has an exterior margin");
        require(draw->VtxBuffer[start+2].pos.x==1110&&draw->VtxBuffer[start+2].pos.y==610,"Status bar does not reach window edges");
        ImGui::End();ImGui::Render();
    }
    {
        const auto low=notebook::snrColor(-30),mid=notebook::snrColor(-10),high=notebook::snrColor(10);
        require(low.x>low.y&&mid.x>mid.z&&high.y>high.x,"SNR color scale does not progress from red to green");
        const char* modes[]={"CW","RTTY","PSK31","PSK63","JS8","Olivia"};
        for(int i=0;i<6;i++)for(int j=i+1;j<6;j++){
            auto a=notebook::modeColor(modes[i]),b=notebook::modeColor(modes[j]);
            require(a.x!=b.x||a.y!=b.y||a.z!=b.z,"Modes have identical tab accents");
        }
        ImGui::NewFrame();ImGui::SetNextWindowPos({10,10});ImGui::SetNextWindowSize({700,400});
        ImGui::Begin("Reception metadata",nullptr,ImGuiWindowFlags_NoSavedSettings);
        radio::Reception rx;rx.observeSnr(-14.2,1);rx.received(radio::monotonicSeconds()-119,1792108800);
        ImGui::LogToClipboard();notebook::rxSnr(rx,false);notebook::lastReceived(rx);ImGui::LogFinish();
        require(clipboard.find("RX SNR -14.2 dB (last)")!=std::string::npos&&clipboard.find("119 seconds ago")!=std::string::npos,"Native header hid held SNR or recent reception age");
        ImGui::End();ImGui::Render();
    }
    {
        const std::string rich="CQ DE K1ABC 599 73 ";const auto fakeTexture=reinterpret_cast<ImTextureID>(uintptr_t(123));
        auto flag=[&](const std::string& code){require(code=="US","Unexpected inline flag country");return fakeTexture;};
        follow=false;frame(rich,{},flag);frame(rich,{},flag);
        auto* window=ImGui::FindWindowByName("Transcript test");bool colored=false,hasFlag=false;
        for(auto* child:window->DC.ChildWindows)if(child->ChildId==inputID){
            for(const auto& vertex:child->DrawList->VtxBuffer)if(vertex.col==ImGui::GetColorU32(ImVec4{.53f,.78f,.98f,1}))colored=true;
            for(const auto& command:child->DrawList->CmdBuffer)if(command.TextureId==fakeTexture)hasFlag=true;
        }
        require(colored&&hasFlag,"Native transcript omitted callsign color or inline flag texture");
        view.copyReport(7020000,20,0,false);
        require(clipboard.find(rich)!=std::string::npos&&clipboard.find("[US]")==std::string::npos,"Display annotations leaked into the original reception report");
        ImGui::FocusWindow(window);
        io.MousePos=textPoint;io.MouseDown[0]=true;frame(rich,{},flag);io.MouseDown[0]=false;frame(rich,{},flag);
        auto* selected=ImGui::GetInputTextState(inputID);require(selected!=nullptr&&ImGui::GetActiveID()==inputID,"Annotated transcript cannot be selected");
        selected->Stb.select_start=0;selected->Stb.select_end=selected->CurLenW;
        io.AddKeyEvent(ImGuiKey_ModSuper,true);io.AddKeyEvent(ImGuiKey_C,true);frame(rich,{},flag);
        io.AddKeyEvent(ImGuiKey_C,false);io.AddKeyEvent(ImGuiKey_ModSuper,false);
        require(clipboard.find(radio::unicodeFlag("US"))!=std::string::npos,"Selected inline flags did not copy as Unicode");
        frame(rich+"NEXT ",{},flag);require(selected->HasSelection(),"Appending annotated text lost the selection");
        frame(rich,{false,false});view.copyReport(7020000,20,0,false);
        require(clipboard.find(rich)!=std::string::npos,"Disabling annotations changed received text");
        const radio::TextOptions cautious{true,true,true};
        const std::string partial="LYL3NU YL3NU ";
        auto anyFlag=[&](const std::string&){return fakeTexture;};
        frame(partial,cautious,anyFlag);frame(partial,cautious,anyFlag);
        ImGui::FocusWindow(window);io.MousePos=textPoint;io.MouseDown[0]=true;
        frame(partial,cautious,anyFlag);io.MouseDown[0]=false;frame(partial,cautious,anyFlag);
        selected=ImGui::GetInputTextState(inputID);require(selected!=nullptr&&ImGui::GetActiveID()==inputID,"Guarded transcript cannot be selected");
        selected->Stb.select_start=0;selected->Stb.select_end=selected->CurLenW;
        frame(partial+"599 ",cautious,anyFlag);require(selected->HasSelection(),"Ordinary RTTY append cleared selection");
        const auto corroborated=partial+"599 YL3NU ";frame(corroborated,cautious,anyFlag);
        require(!selected->HasSelection(),"Removed flag left stale selection indices");
        view.copyReport(7020000,20,0,false);
        require(clipboard.find(corroborated)!=std::string::npos&&clipboard.find("[LT]")==std::string::npos,
            "Cautious RTTY annotations changed the original report");
        notebook::SettingsNavigation pages;
        for(int pass=0;pass<3;pass++){
            ImGui::NewFrame();ImGui::SetNextWindowPos({10,10});ImGui::SetNextWindowSize({740,550});ImGui::Begin("Settings navigation regression",nullptr,ImGuiWindowFlags_NoSavedSettings);
            pages.draw([&](int page){ImGui::Text("Page %d",page);require(ImGui::GetContentRegionAvail().x>400,"Settings page is clipped by its sidebar");});
            ImGui::End();ImGui::Render();
        }
        require(pages.page==0,"Settings did not open its display/callsign page");
    }
    ImGui::DestroyContext();std::cout<<"Transcript refresh and window presentation checks passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
