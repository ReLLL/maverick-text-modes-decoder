// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "host_adapter.h"
#include "sharp_host.h"
#include "file_platform.h"
#include <fstream>
#include <cctype>
namespace {
using json=notebook::Json;
struct Settings {
    json conf={{"moduleInstances",json::object()}};std::mutex mutex;
    Settings(){auto file=notebook::sharpHost().root/"receiver.json";if(!std::filesystem::exists(file))return;
        if(std::filesystem::file_size(file)>1024*1024)throw std::runtime_error("Receiver preferences exceed the size limit");
        std::ifstream input(file);auto loaded=json::parse(input,nullptr,false);
        if(!loaded.is_object()||!loaded.value("moduleInstances",json{}).is_object())throw std::runtime_error("Receiver preferences are damaged; restore their backup before reopening");
        conf=std::move(loaded);
    }
    void acquire(){mutex.lock();}
    void release(bool save=false){auto copy=conf;mutex.unlock();if(!save)return;
        const auto path=notebook::sharpHost().root/"receiver.json",temp=notebook::sharpHost().root/"receiver.pending.json";
        {std::ofstream out(temp,std::ios::binary|std::ios::trunc);out.exceptions(std::ios::badbit|std::ios::failbit);out<<copy.dump(2);}
        cw::replaceFile(temp,path);
    }
};
Settings& settings(){static Settings value;return value;}
}
namespace cw::host {
std::filesystem::path logFolder(){return notebook::sharpHost().root/"CW Logs";}
std::string mode(const std::string&){return notebook::sharpHost().state().mode;}
void useCW(const std::string&){notebook::sharpHost().request({{"mode","CW"}});}
bool listenRTTY(const std::string&,double mark){
    const auto host=notebook::sharpHost().state();const double dial=mark-2125;
    if(!std::isfinite(mark)||dial<=0||!host.playing||dial-100<host.center-host.rate/2||dial+3200>host.center+host.rate/2)return false;
    notebook::sharpHost().request({{"frequency",dial},{"mode","USB"},{"bandwidth",3200}});return true;
}
radio::TextOptions textOptions(const std::string& instance){
    radio::TextOptions options;settings().acquire();const auto& instances=settings().conf["moduleInstances"];
    if(instances.contains(instance)&&instances[instance].is_object()){
        const auto& module=instances[instance];
        if(module.contains("detectCallsigns")&&module["detectCallsigns"].is_boolean())options.detectCallsigns=module["detectCallsigns"].get<bool>();
        if(module.contains("highlightHamTerms")&&module["highlightHamTerms"].is_boolean())options.highlightTerms=module["highlightHamTerms"].get<bool>();
    }
    settings().release();return options;
}
void setTextOptions(const std::string& instance,const radio::TextOptions& options){
    settings().acquire();auto& module=settings().conf["moduleInstances"][instance];
    module["detectCallsigns"]=options.detectCallsigns;module["highlightHamTerms"]=options.highlightTerms;settings().release(true);
}
std::string grid(const std::string& instance){
    settings().acquire();
    const auto& config=settings().conf;
    std::string result;
    if(brown){
        if(config.contains("operatorLocation")&&config["operatorLocation"].is_string())result=config["operatorLocation"].get<std::string>();
    }else if(config.contains("moduleInstances")&&config["moduleInstances"].contains(instance)){
        const auto& module=config["moduleInstances"][instance];
        if(module.is_object()&&module.contains("receiverGrid")&&module["receiverGrid"].is_string())result=module["receiverGrid"].get<std::string>();
    }
    settings().release();
    return result;
}
bool rttyEnabled(const std::string& instance){
    settings().acquire();const auto& instances=settings().conf["moduleInstances"];
    bool enabled=true;
    if(instances.contains(instance)&&instances[instance].is_object()&&instances[instance].contains("rttyEnabled")&&instances[instance]["rttyEnabled"].is_boolean())
        enabled=instances[instance]["rttyEnabled"].get<bool>();
    settings().release();return enabled;
}
void setRttyEnabled(const std::string& instance,bool enabled){
    settings().acquire();settings().conf["moduleInstances"][instance]["rttyEnabled"]=enabled;settings().release(true);
}
bool pskEnabled(const std::string& instance){
    settings().acquire();const auto& instances=settings().conf["moduleInstances"];
    bool enabled=true;
    if(instances.contains(instance)&&instances[instance].is_object()&&instances[instance].contains("pskEnabled")&&instances[instance]["pskEnabled"].is_boolean())
        enabled=instances[instance]["pskEnabled"].get<bool>();
    settings().release();return enabled;
}
void setPskEnabled(const std::string& instance,bool enabled){
    settings().acquire();settings().conf["moduleInstances"][instance]["pskEnabled"]=enabled;settings().release(true);
}
bool fecEnabled(const std::string& instance,bool js8){
    const char* key=js8?"js8Enabled":"oliviaEnabled";bool enabled=true;
    settings().acquire();const auto& instances=settings().conf["moduleInstances"];
    if(instances.contains(instance)&&instances[instance].is_object()&&instances[instance].contains(key)&&instances[instance][key].is_boolean())enabled=instances[instance][key].get<bool>();
    settings().release();return enabled;
}
void setFecEnabled(const std::string& instance,bool js8,bool enabled){
    settings().acquire();settings().conf["moduleInstances"][instance][js8?"js8Enabled":"oliviaEnabled"]=enabled;settings().release(true);
}
rtty::Selection rttySelection(const std::string& instance){
    rtty::Selection selection;
    settings().acquire();
    const auto& instances=settings().conf["moduleInstances"];
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
    settings().release();return rtty::validated(std::move(selection));
}
void setRttySelection(const std::string& instance,const rtty::Selection& selection){
    json saved{{"onlyPinned",selection.onlyPinned},{"pinned",json::array()},{"ignored",json::array()}};
    for(const auto& t:selection.pinned)saved["pinned"].push_back({t.center,t.shift});
    for(const auto& t:selection.ignored)saved["ignored"].push_back({t.center,t.shift});
    settings().acquire();settings().conf["moduleInstances"][instance]["rttySelection"]=std::move(saved);settings().release(true);
}
bool setGrid(std::string value,const std::string& instance){
    for(auto& c:value)c=char(std::toupper(static_cast<unsigned char>(c)));
    if(!value.empty()){
        if(value.size()!=4&&value.size()!=6&&value.size()!=8)return false;
        for(size_t i=0;i<value.size();++i){bool valid=i<2?value[i]>='A'&&value[i]<='R':i<4||i>=6?value[i]>='0'&&value[i]<='9':value[i]>='A'&&value[i]<='X';if(!valid)return false;}
    }
    settings().acquire();settings().conf["moduleInstances"][instance]["receiverGrid"]=value;settings().release(true);return true;
}
}
