// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "journal.h"
#include "file_platform.h"
#include "radio_text.h"
#include <windows.h>
#include <fstream>
#include <iostream>
int main(){try{
    auto root=std::filesystem::temp_directory_path()/std::filesystem::u8path("maverick-tests-İ-受信-"+std::to_string(GetCurrentProcessId()));
    if(std::filesystem::exists(root))throw std::runtime_error("Test folder already exists");std::filesystem::create_directories(root);
    {
        cw::Journal log(root);log.create(7045000);log.append("CQ DE K1ABC 73",7045000,20);log.rename("K1ABC.md");
        auto path=log.path();std::ofstream(root/"occupied.md")<<"keep";bool rejected=false;try{log.rename("occupied.md");}catch(const std::exception&){rejected=true;}
        if(!rejected||!std::filesystem::exists(path))throw std::runtime_error("Rename overwrote a log");
        auto backup=log.clear();if(std::filesystem::file_size(backup)<=std::filesystem::file_size(log.path()))throw std::runtime_error("Archive lost original text");
        for(auto name:{"CON","NUL.md","a:b","trail.","../escape"}){bool bad=false;try{cw::Journal::validName(name);}catch(const std::exception&){bad=true;}if(!bad)throw std::runtime_error("Reserved filename accepted");}
    }
    auto target=root/"preferences.json",temp=root/"pending.json";std::ofstream(target)<<"old";std::ofstream(temp)<<"new";cw::replaceFile(temp,target);
    std::ifstream input(target);std::string value;input>>value;if(value!="new")throw std::runtime_error("Preference replacement failed");input.close();
    std::filesystem::remove_all(root);std::cout<<"Unicode paths, log exclusion/archive and atomic replacement passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
