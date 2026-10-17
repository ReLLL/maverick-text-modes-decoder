// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "country_lookup.h"
#include "country_data.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>
#include <vector>
namespace radio {
namespace {
std::string trim(const std::string& v){auto a=v.find_first_not_of(" \t\r\n"),b=v.find_last_not_of(" \t\r\n");return a==std::string::npos?"":v.substr(a,b-a+1);}
struct Database {
    std::unordered_map<std::string,Country> prefixes,exact;
    Database(){
        std::unordered_map<std::string,std::string> flags;
        std::istringstream codes(notebookFlagCodes);std::string line;
        while(std::getline(codes,line)){auto tab=line.find('\t');if(tab!=std::string::npos)flags[line.substr(0,tab)]=trim(line.substr(tab+1));}
        std::istringstream data(notebookCountryData);Country current;
        while(std::getline(data,line)){
            if(trim(line).empty())continue;
            if(!std::isspace(static_cast<unsigned char>(line.front()))){
                auto name=trim(line.substr(0,line.find(':')));current={name,flags[name]};
                if(name=="Fed. Rep. of Germany")current.name="Germany";
                continue;
            }
            std::istringstream aliases(line);std::string prefix;
            while(std::getline(aliases,prefix,',')){
                prefix=trim(prefix);if(prefix.empty())continue;bool full=prefix.front()=='=';if(full)prefix.erase(0,1);
                prefix=prefix.substr(0,prefix.find_first_of("([{<~; \t\r\n"));if(prefix.empty())continue;
                auto& target=full?exact:prefixes;auto [it,inserted]=target.emplace(prefix,current);
                if(!inserted&&it->second.name!=current.name)it->second={}; // Ambiguous aliases never guess.
            }
        }
    }
    Country prefix(std::string value)const{
        while(!value.empty()){auto it=prefixes.find(value);if(it!=prefixes.end())return it->second;value.pop_back();}return {};
    }
};
bool hasLetter(const std::string& s){return s.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ")!=std::string::npos;}
bool hasDigit(const std::string& s){return s.find_first_of("0123456789")!=std::string::npos;}
}
Country callsignCountry(const std::string& input){
    if(input.size()<3||input.size()>32)return {};
    std::string call;for(unsigned char c:input){if(!std::isalnum(c)&&c!='/')return {};call+=char(std::toupper(c));}
    if(!hasLetter(call)||!hasDigit(call))return {};
    static const Database database;
    if(auto it=database.exact.find(call);it!=database.exact.end())return it->second;
    std::vector<std::string> parts;std::istringstream split(call);std::string part;
    while(std::getline(split,part,'/')){if(part.empty())return {};parts.push_back(part);}
    if(call.back()=='/')return {};
    // Maritime/airborne operation does not establish a land country.
    if(std::find(parts.begin(),parts.end(),"MM")!=parts.end()||std::find(parts.begin(),parts.end(),"AM")!=parts.end())return {};
    if(parts.size()>3)return {};
    auto suffix=[](const std::string& p){return p=="P"||p=="M"||p=="QRP"||p=="QRPP"||!hasLetter(p);};
    while(parts.size()>1&&suffix(parts.back()))parts.pop_back();
    if(parts.size()>1){
        // Recognized portable prefixes take precedence over the home call.
        if(parts.front().size()<=4&&(!hasDigit(parts.front())||std::isdigit(static_cast<unsigned char>(parts.front().back())))){
            auto c=database.prefix(parts.front());if(!c.name.empty())return c;
        }
        if(parts.back().size()<=4){auto c=database.prefix(parts.back());if(!c.name.empty())return c;}
        return {}; // Unrecognized operating suffix: don't silently claim home.
    }
    call=parts.front();if(!hasLetter(call)||!hasDigit(call))return {};
    if(auto it=database.exact.find(call);it!=database.exact.end())return it->second;
    return database.prefix(call);
}
}
