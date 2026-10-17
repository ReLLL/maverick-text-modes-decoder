// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <deque>
#include <string>
namespace fec {
class FrameHistory {
public:
    bool accept(std::string tag){
        if(std::find(tags_.begin(),tags_.end(),tag)!=tags_.end())return false;
        tags_.push_back(std::move(tag));if(tags_.size()>128)tags_.pop_front();return true;
    }
private:
    std::deque<std::string> tags_;
};
}
