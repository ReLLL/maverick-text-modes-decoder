// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "host_adapter.h"
#include <core.h>
#include <cstdlib>
#include <cctype>
#include <radio_interface.h>
#include <gui/tuner.h>
#include <gui/gui.h>
#include <signal_path/signal_path.h>
namespace cw::host {
radio::TextOptions textOptions(const std::string& instance){
    radio::TextOptions options;core::configManager.acquire();const auto& instances=core::configManager.conf["moduleInstances"];
    if(instances.contains(instance)&&instances[instance].is_object()){
        const auto& module=instances[instance];
        if(module.contains("detectCallsigns")&&module["detectCallsigns"].is_boolean())options.detectCallsigns=module["detectCallsigns"].get<bool>();
        if(module.contains("highlightHamTerms")&&module["highlightHamTerms"].is_boolean())options.highlightTerms=module["highlightHamTerms"].get<bool>();
    }
    core::configManager.release();return options;
}
void setTextOptions(const std::string& instance,const radio::TextOptions& options){
    core::configManager.acquire();auto& module=core::configManager.conf["moduleInstances"][instance];
    module["detectCallsigns"]=options.detectCallsigns;module["highlightHamTerms"]=options.highlightTerms;core::configManager.release(true);
}
std::filesystem::path logFolder(){
    // Explicit process-local override for isolated replay/diagnostic profiles.
    if(const char* folder=std::getenv("CW_NOTEBOOK_LOG_DIR")){
        std::filesystem::path path(folder);if(path.is_absolute())return path;
    }
    const char* home=std::getenv("HOME");
    return std::filesystem::path(home?home:".")/"Documents"/name/"CW Logs";
}
std::string mode(const std::string& receiver){
#if CW_HOST_BROWN
    auto it=core::moduleManager.instances.find(receiver);
    if(it!=core::moduleManager.instances.end()&&it->second.instance){
        try{return json::parse(it->second.instance->handleDebugCommand("get_demod","")).value("demod","");}
        catch(const json::exception&){}
    }
#else
    if(core::modComManager.interfaceExists(receiver)&&core::modComManager.getModuleName(receiver)=="radio"){
        int selected=-1;
        if(core::modComManager.callInterface(receiver,RADIO_IFACE_CMD_GET_MODE,nullptr,&selected)){
            switch(selected){
                case RADIO_IFACE_MODE_CW:return "CW";
                case RADIO_IFACE_MODE_USB:return "USB";
                case RADIO_IFACE_MODE_LSB:return "LSB";
                case RADIO_IFACE_MODE_DSB:return "DSB";
                case RADIO_IFACE_MODE_AM:return "AM";
                case RADIO_IFACE_MODE_NFM:return "NFM";
                case RADIO_IFACE_MODE_WFM:return "WFM";
                case RADIO_IFACE_MODE_RAW:return "RAW";
            }
        }
    }
#endif
    return {};
}
void useCW(const std::string& receiver){
#if CW_HOST_BROWN
    auto it=core::moduleManager.instances.find(receiver);
    if(it!=core::moduleManager.instances.end()&&it->second.instance)
        it->second.instance->handleDebugCommand("set_demod","CW");
#else
    if(core::modComManager.interfaceExists(receiver)&&core::modComManager.getModuleName(receiver)=="radio"){
        int selected=RADIO_IFACE_MODE_CW;
        core::modComManager.callInterface(receiver,RADIO_IFACE_CMD_SET_MODE,&selected,nullptr);
    }
#endif
}
std::string grid(const std::string& instance){
    core::configManager.acquire();
    const auto& config=core::configManager.conf;
    std::string result;
    if(brown){
        if(config.contains("operatorLocation")&&config["operatorLocation"].is_string())result=config["operatorLocation"].get<std::string>();
    }else if(config.contains("moduleInstances")&&config["moduleInstances"].contains(instance)){
        const auto& module=config["moduleInstances"][instance];
        if(module.is_object()&&module.contains("receiverGrid")&&module["receiverGrid"].is_string())result=module["receiverGrid"].get<std::string>();
    }
    core::configManager.release();
    return result;
}
bool rttyEnabled(const std::string& instance){
    core::configManager.acquire();const auto& instances=core::configManager.conf["moduleInstances"];
    bool enabled=true;
    if(instances.contains(instance)&&instances[instance].is_object()&&instances[instance].contains("rttyEnabled")&&instances[instance]["rttyEnabled"].is_boolean())
        enabled=instances[instance]["rttyEnabled"].get<bool>();
    core::configManager.release();return enabled;
}
void setRttyEnabled(const std::string& instance,bool enabled){
    core::configManager.acquire();core::configManager.conf["moduleInstances"][instance]["rttyEnabled"]=enabled;core::configManager.release(true);
}
bool pskEnabled(const std::string& instance){
    core::configManager.acquire();const auto& instances=core::configManager.conf["moduleInstances"];
    bool enabled=true;
    if(instances.contains(instance)&&instances[instance].is_object()&&instances[instance].contains("pskEnabled")&&instances[instance]["pskEnabled"].is_boolean())
        enabled=instances[instance]["pskEnabled"].get<bool>();
    core::configManager.release();return enabled;
}
void setPskEnabled(const std::string& instance,bool enabled){
    core::configManager.acquire();core::configManager.conf["moduleInstances"][instance]["pskEnabled"]=enabled;core::configManager.release(true);
}
bool fecEnabled(const std::string& instance,bool js8){
    const char* key=js8?"js8Enabled":"oliviaEnabled";bool enabled=true;
    core::configManager.acquire();const auto& instances=core::configManager.conf["moduleInstances"];
    if(instances.contains(instance)&&instances[instance].is_object()&&instances[instance].contains(key)&&instances[instance][key].is_boolean())enabled=instances[instance][key].get<bool>();
    core::configManager.release();return enabled;
}
void setFecEnabled(const std::string& instance,bool js8,bool enabled){
    core::configManager.acquire();core::configManager.conf["moduleInstances"][instance][js8?"js8Enabled":"oliviaEnabled"]=enabled;core::configManager.release(true);
}
rtty::Selection rttySelection(const std::string& instance){
    rtty::Selection selection;
    core::configManager.acquire();
    const auto& instances=core::configManager.conf["moduleInstances"];
    if(instances.contains(instance)&&instances[instance].contains("rttySelection")){
        const auto& saved=instances[instance]["rttySelection"];
        if(saved.is_object()){
            if(saved.contains("onlyPinned")&&saved["onlyPinned"].is_boolean())selection.onlyPinned=saved["onlyPinned"].get<bool>();
            auto load=[&](const char* key,auto& list,size_t limit){
                if(!saved.contains(key)||!saved[key].is_array())return;
                for(const auto& item:saved[key]){
                    if(list.size()>=limit)break;
                    if(item.is_array()&&item.size()==2&&item[0].is_number()&&item[1].is_number()){
                        rtty::Target target{item[0].get<double>(),item[1].get<double>()};if(rtty::valid(target))list.push_back(target);
                    }
                }
            };
            load("pinned",selection.pinned,16);load("ignored",selection.ignored,64);
        }
    }
    core::configManager.release();return rtty::validated(std::move(selection));
}
void setRttySelection(const std::string& instance,const rtty::Selection& selection){
    json saved{{"onlyPinned",selection.onlyPinned},{"pinned",json::array()},{"ignored",json::array()}};
    for(const auto& t:selection.pinned)saved["pinned"].push_back({t.center,t.shift});
    for(const auto& t:selection.ignored)saved["ignored"].push_back({t.center,t.shift});
    core::configManager.acquire();core::configManager.conf["moduleInstances"][instance]["rttySelection"]=std::move(saved);core::configManager.release(true);
}
bool listenRTTY(const std::string& receiver,double mark){
    const double dial=mark-2125,center=gui::waterfall.getCenterFrequency(),rate=sigpath::iqFrontEnd.getSampleRate();
    if(!std::isfinite(mark)||dial<center-rate/2+100||dial+3200>center+rate/2-100||!sigpath::vfoManager.vfoExists(receiver))return false;
    if(!core::modComManager.interfaceExists(receiver)||core::modComManager.getModuleName(receiver)!="radio")return false;
#if CW_HOST_BROWN
    auto instance=core::moduleManager.instances.find(receiver);
    if(instance==core::moduleManager.instances.end()||!instance->second.instance)return false;
    instance->second.instance->handleDebugCommand("set_demod","USB");
#else
    int usb=RADIO_IFACE_MODE_USB;core::modComManager.callInterface(receiver,RADIO_IFACE_CMD_SET_MODE,&usb,nullptr);
#endif
    float width=3200;core::modComManager.callInterface(receiver,RADIO_IFACE_CMD_SET_BANDWIDTH,&width,nullptr);
    tuner::tune(tuner::TUNER_MODE_NORMAL,receiver,dial);return true;
}
bool setGrid(std::string value,const std::string& instance){
    if(brown)return false; // Brown owns its native location setting.
    for(char& c:value)c=char(std::toupper(static_cast<unsigned char>(c)));
    if(!value.empty()){
        if(value.size()!=4&&value.size()!=6&&value.size()!=8)return false;
        for(size_t i=0;i<value.size();++i){
            bool valid=i<2?value[i]>='A'&&value[i]<='R':
                i<4||i>=6?value[i]>='0'&&value[i]<='9':value[i]>='A'&&value[i]<='X';
            if(!valid)return false;
        }
    }
    // Upstream removes unrecognized top-level keys. Keep settings in this instance.
    core::configManager.acquire();core::configManager.conf["moduleInstances"][instance]["receiverGrid"]=value;
    core::configManager.release(true);return true;
}
}
