// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "radio_text.h"
#include <algorithm>
#include <cctype>
#include <unordered_map>
namespace radio {
namespace {
bool word(unsigned char c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='/';}
std::string upper(std::string value){for(auto& c:value)c=char(std::toupper(static_cast<unsigned char>(c)));return value;}
bool ordinaryCall(const std::string& s){
    if(s.size()<3||s.size()>12)return false;
    const auto digit=s.find_last_of("0123456789");
    if(digit==std::string::npos||digit==0||digit>4||digit+1==s.size()||s.size()-digit-1>5)return false;
    if(s.substr(0,digit).find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ")==std::string::npos)return false;
    return s.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ",digit+1)==std::string::npos;
}
Ink term(const std::string& token){
    if(token=="CQ"||token=="QRZ"||token=="QSY"||token=="QRL")return Ink::Calling;
    if(token=="DE"||token=="QTH"||token=="NAME"||token=="OP")return Ink::Identity;
    if(token=="RST"||token=="SNR"||token=="RPT"||token=="599"||token=="559"||token=="579"||token=="QSB"||token=="QRM"||token=="QRN")return Ink::Report;
    if(token=="73"||token=="88"||token=="TNX"||token=="TKS"||token=="TU"||token=="SK"||token=="AR"||token=="RR73")return Ink::Courtesy;
    return Ink::Plain;
}
}
bool looksLikeCallsign(const std::string& input){
    if(input.size()>32)return false;const auto call=upper(input);
    if(std::any_of(call.begin(),call.end(),[](unsigned char c){return !word(c);}))return false;
    size_t start=0;int count=0;bool normal=false;
    while(start<call.size()){
        auto slash=call.find('/',start);auto part=call.substr(start,slash==std::string::npos?slash:slash-start);
        if(part.empty()||++count>3)return false;normal=normal||ordinaryCall(part);
        if(slash==std::string::npos)break;start=slash+1;if(start==call.size())return false;
    }
    return normal;
}
TextMarkup markText(const std::string& raw,const TextOptions& options){
    TextMarkup result;if(!options.detectCallsigns&&!options.highlightTerms){result.display=raw;return result;}
    result.display.reserve(raw.size()+raw.size()/16);
    std::unordered_map<std::string,Country> cache;
    const auto provisional=raw.find("[PROVISIONAL - learning");
    std::unordered_map<std::string,unsigned> occurrences;
    if(options.detectCallsigns&&options.checkExtensions){
        // A bounded context pass, only when the displayed RTTY text changes.
        // Saturate counts at two; no identity is inferred from provisional text.
        for(size_t at=0;at<raw.size()&&at<provisional;){
            if(!word(static_cast<unsigned char>(raw[at]))){++at;continue;}
            size_t end=at+1;while(end<raw.size()&&word(static_cast<unsigned char>(raw[end])))++end;
            if(end<raw.size()&&end<=provisional&&!(at>0&&raw[at-1]=='<')){
                auto token=upper(raw.substr(at,end-at));
                if(looksLikeCallsign(token)){
                    auto found=occurrences.find(token);
                    if(found!=occurrences.end())found->second=2;
                    else if(occurrences.size()<256)occurrences.emplace(std::move(token),1);
                }
            }
            at=end;
        }
    }
    auto ambiguousExtension=[&](const std::string& token){
        const auto own=occurrences.find(token);
        if(own==occurrences.end()||own->second!=1||token.find('/')!=std::string::npos||token[0]<'A'||token[0]>'Z')return false;
        const auto shorter=token.substr(1);const auto seen=occurrences.find(shorter);
        return seen!=occurrences.end()&&seen->second>=2&&!callsignCountry(shorter).name.empty();
    };
    for(size_t at=0;at<raw.size();){
        if(!word(static_cast<unsigned char>(raw[at]))){result.display+=raw[at++];continue;}
        size_t end=at+1;while(end<raw.size()&&word(static_cast<unsigned char>(raw[end])))++end;
        const auto token=raw.substr(at,end-at),normalized=upper(token);const auto begin=result.display.size();result.display+=token;
        Ink ink=options.highlightTerms?term(normalized):Ink::Plain;Country country;
        // Wait for a delimiter before annotating a still-arriving callsign.
        if(options.detectCallsigns&&end<raw.size()&&at<provisional&&!(at>0&&raw[at-1]=='<')&&looksLikeCallsign(normalized)&&!ambiguousExtension(normalized)){
            auto found=cache.find(normalized);
            if(found!=cache.end())country=found->second;
            else{country=callsignCountry(normalized);if(cache.size()<256)cache.emplace(normalized,country);}
            if(!country.name.empty()){
                ink=Ink::Call;
                auto previous=std::find_if(result.calls.begin(),result.calls.end(),[&](const auto& c){return c.call==normalized;});
                if(previous!=result.calls.end())result.calls.erase(previous);
                result.calls.push_back({normalized,country});if(result.calls.size()>24)result.calls.erase(result.calls.begin());
            }
        }
        if(ink!=Ink::Plain)result.spans.push_back({begin,result.display.size(),ink,{}});
        if(ink==Ink::Call&&!country.flag.empty()){
            const auto flag=result.display.size();result.display+='['+country.flag+']';
            result.spans.push_back({flag,result.display.size(),Ink::Flag,country.flag});
        }
        at=end;
    }
    return result;
}
std::string qrzUrl(const std::string& input){
    if(!looksLikeCallsign(input))return {};
    std::string encoded;for(char c:upper(input))encoded+=c=='/'?"%2F":std::string(1,c);
    return "https://www.qrz.com/db/"+encoded;
}
std::string unicodeFlag(const std::string& code){
    if(code.size()!=2||code[0]<'A'||code[0]>'Z'||code[1]<'A'||code[1]>'Z')return {};
    std::string result;for(char c:code){const unsigned value=0x1F1E6+unsigned(c-'A');
        result+=char(0xF0|(value>>18));result+=char(0x80|((value>>12)&63));result+=char(0x80|((value>>6)&63));result+=char(0x80|(value&63));}
    return result;
}
}
