// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
namespace cw {
inline void replaceFile(const std::filesystem::path& from,const std::filesystem::path& to){
#ifdef _WIN32
    if(!MoveFileExW(from.c_str(),to.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
        throw std::filesystem::filesystem_error("Replace preferences",from,to,std::error_code(GetLastError(),std::system_category()));
#else
    std::filesystem::rename(from,to);
#endif
}
inline std::string pathText(const std::filesystem::path& path){return path.u8string();}
}
