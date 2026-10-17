// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <map>
#include <string>
namespace notebook {
// GUI/context thread only. Native emoji rasterized once per ISO country.
class FlagTextures {
public:
    ~FlagTextures();
    FlagTextures()=default;
    FlagTextures(const FlagTextures&)=delete;
    FlagTextures& operator=(const FlagTextures&)=delete;
    void draw(const std::string& code);
private:
    std::map<std::string,unsigned int> textures_;
};
}
