// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "country_lookup.h"
#include <vector>
namespace radio {
struct TextOptions {
    bool detectCallsigns=true,highlightTerms=true;
    bool checkExtensions=false; // RTTY display context only; not a saved preference.
    bool operator==(const TextOptions& other)const{return detectCallsigns==other.detectCallsigns&&highlightTerms==other.highlightTerms&&checkExtensions==other.checkExtensions;}
};
enum class Ink {Plain,Call,Calling,Identity,Report,Courtesy,Flag};
struct CallTag {std::string call;Country country;};
struct TextSpan {size_t begin=0,end=0;Ink ink=Ink::Plain;std::string flag;};
struct TextMarkup {std::string display;std::vector<TextSpan> spans;std::vector<CallTag> calls;};
TextMarkup markText(const std::string& text,const TextOptions& options);
bool looksLikeCallsign(const std::string& text);
std::string qrzUrl(const std::string& call);
std::string unicodeFlag(const std::string& code);
}
