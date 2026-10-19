// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "engine.h"
#include "product_identity.h"
#include "transcript_view.h"
#include "stream_watchdog.h"
#include "receive_channel.h"
#include "ai_handoff.h"
#include "host_adapter.h"
#include "rtty_receiver.h"
#include "rtty_signal_list.h"
#include "cw_tab.h"
#include "country_lookup.h"
#include "flag_textures.h"
#include "settings_navigation.h"
#include <map>
#if CW_HOST_SDRSHARP
#include "notebook_platform.h"
#else
#include <module.h>
#include <gui/gui.h>
#include <dsp/sink/handler_sink.h>
#include <signal_path/signal_path.h>
#endif
#include <atomic>

class CWNotebook final:public ModuleManager::Instance {
public:
    explicit CWNotebook(std::string name);
    ~CWNotebook() override;
    void postInit() override;
    void enable() override;
    void disable() override;
    bool isEnabled() override { return enabled_; }
    std::string handleDebugCommand(const std::string& cmd,const std::string& args)
#if CW_HOST_BROWN
        override
#endif
        ;
    void sidebar();
    void draw();
#if CW_HOST_SDRSHARP
    void receiveWindows(notebook::SharpBlock block);
    void showWindows(){window_=true;presentWindow_=true;}
    void hideWindows(){window_=false;}
    bool windowsVisible()const{return window_;}
#endif
private:
    void select(const std::string& stream);
    void detach();
    void metadata();
#if !CW_HOST_SDRSHARP
    static void audio(dsp::complex_t* samples,int count,void* ctx);
#endif
    void openPath(const std::string& path,bool reveal);
    void explainWithAI(int provider);
    void openAnalysisPrompt(int provider,const std::string& prompt);
    void logControls(const cw::Snapshot& s);
    void decoderControls(const cw::Snapshot& s);
    void settings(const cw::Snapshot& s);
    void callTags(TranscriptView& view,const std::string& text,const std::string& headerCall={},bool checkExtensions=false);
    void cwStatus(const cw::Snapshot& s);
    void setCWMonitoring(bool enabled);
    void rttyControls();
    void receiveStatus(const cw::Snapshot& s);
    void rttyTabs(float footer);
    void setRttyMonitoring(bool enabled);
    void setPskMonitoring(bool enabled);
    void setFecMonitoring(bool js8,bool enabled);
    bool digitalEnabled() const{return rttyEnabled_||pskEnabled_||js8Enabled_||oliviaEnabled_;}
    void syncDigitalMonitoring();
    void saveRttySelection();
    void pinRtty(double center,double shift,bool pin);
    void ignoreRtty(double center,double shift,bool ignore);
    void signalPicker();
    void notebookModals(const cw::Snapshot& s);
    void operatingGuide();
    void openNotebookModal(int modal);
    void listenRtty(double mark);
    std::string name_,streamName_="Radio";
    cw::Engine engine_;
    std::unique_ptr<RttyReceiver> rtty_;
    struct RttyView {TranscriptView transcript;bool follow=true,compact=true;std::string seen,rawCache,compactCache,lastCall;radio::Country country;};
    std::map<uint64_t,RttyView> rttyViews_;
    notebook::FlagTextures flags_;
    radio::TextOptions textOptions_;
    notebook::SettingsNavigation settingsNavigation_;
    bool rttyEnabled_=false;
    bool pskEnabled_=false;
    bool js8Enabled_=false,oliviaEnabled_=false;
    std::atomic<bool> js8EnabledView_{false},oliviaEnabledView_{false};
    std::atomic<int> pendingJs8Enable_{-1},pendingOliviaEnable_{-1};
    std::atomic<bool> pskEnabledView_{false};
    std::atomic<int> pendingPskEnable_{-1};
    bool selectCW_=false;
    notebook::CWTab cwTab_;
    std::atomic<bool> cwEnabledView_{true};
    std::atomic<int> pendingCWEnable_{-1};
    bool cwSettingsOpen_=false,helpOpen_=false,signalsOpen_=false;
    rtty::Selection rttySelection_;
    rtty::SignalList signalList_;
    std::string rttyNotice_;
    std::atomic<int> pendingRttyEnable_{-1};
    std::atomic<int> pendingModal_{0};
    std::atomic<double> pendingRttyListen_{0};
    std::atomic<bool> pendingRttyCloseAll_{false},rttyEnabledView_{false};
    std::shared_ptr<const rtty::Status> rttyStatus_=std::make_shared<const rtty::Status>();
    TranscriptView transcript_;
    cw::StreamWatchdog watchdog_;
    std::atomic<uint64_t> inputBlocks_{0};
    uint64_t reconnects_=0;
    bool showRecovery_=false;
    double lastFrequency_=0,lastCenter_=0,lastRate_=0;
#if !CW_HOST_SDRSHARP
    dsp::sink::Handler<dsp::complex_t> sink_;
    dsp::channel::RxVFO* audioStream_=nullptr;
    EventHandler<std::string> registered_,unregistering_;
    EventHandler<ImGuiContext*> drawHandler_;
    ImGuiID drawHook_=0;
    EventHandler<ImGui::WaterFall::InputHandlerArgs> inputHandler_;
#endif
    ImVec2 windowMin_{0,0},windowMax_{0,0};
    std::atomic<int> sampleRate_{8000};
    std::atomic<double> frequency_{0};
    std::atomic<bool> accept_{false};
    double channelOffset_=0;
    cw::ReceiveChannel channel_;
    cw::ChannelMixer mixer_;
    std::mutex channelMutex_;
    std::string mode_;
    double bandwidth_=500;
    bool followsBandwidth_=false;
    bool enabled_=true,window_=true,follow_=true,manualSpeed_=false,manualPitch_=false;
    bool presentWindow_=true;
    bool supportedMode_=false,playing_=false;
    bool renaming_=false;
    bool ownsMouseGesture_=false;
    bool controlsPopupOpen_=false;
    std::string aiNotice_;
    double aiNoticeAt_=-10;
    float speed_=20,pitch_=800,textSize_=1.15;
    int frame_=0;
#if CW_HOST_SDRSHARP
    uint64_t sharpSequences_[2]{};
    bool sharpStarted_[2]{};
#endif
    uint64_t lastBlocks_=0, lastTranscriptReset_=0;
    double lastAudioTime_=0;
    char filename_[256]{};
    char grid_[9]{};
    bool gridError_=false;
};
