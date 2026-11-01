// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "host_compatibility.h"
#include "host_images.h"
#include <CommonCrypto/CommonDigest.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <array>
#include <cstdio>
#include <cstring>

namespace {
std::string imageUUID(const mach_header* header){
    if(!header)return {};
    const auto headerSize=header->magic==MH_MAGIC_64?sizeof(mach_header_64):sizeof(mach_header);
    const auto* cursor=reinterpret_cast<const unsigned char*>(header)+headerSize;
    const auto* end=cursor+header->sizeofcmds;
    for(uint32_t i=0;i<header->ncmds;++i){
        if(static_cast<size_t>(end-cursor)<sizeof(load_command))return {};
        const auto* command=reinterpret_cast<const load_command*>(cursor);
        if(command->cmdsize<sizeof(load_command)||command->cmdsize>static_cast<size_t>(end-cursor))return {};
        if(command->cmd==LC_UUID && command->cmdsize>=sizeof(uuid_command)){
            const auto* uuid=reinterpret_cast<const uuid_command*>(command);
            constexpr char hex[]="0123456789abcdef";
            std::string result;
            for(auto byte:uuid->uuid){result+=hex[byte>>4];result+=hex[byte&15];}
            return result;
        }
        cursor+=command->cmdsize;
    }
    return {};
}
std::string digest(const char* path){
    auto* file=std::fopen(path,"rb");
    if(!file)return {};
    CC_SHA256_CTX context;
    CC_SHA256_Init(&context);
    std::array<unsigned char,65536> bytes;
    while(const auto size=std::fread(bytes.data(),1,bytes.size(),file))
        CC_SHA256_Update(&context,bytes.data(),static_cast<CC_LONG>(size));
    const bool failed=std::ferror(file);
    std::fclose(file);
    if(failed)return {};
    unsigned char result[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256_Final(result,&context);
    constexpr char hex[]="0123456789abcdef";
    std::string text;
    text.reserve(CC_SHA256_DIGEST_LENGTH*2);
    for(auto byte:result){text+=hex[byte>>4];text+=hex[byte&15];}
    return text;
}
}

namespace cw::host {
std::string compatibilityError(bool installerProbe){
    for(const auto& expected:expectedImages){
        if(installerProbe && std::strcmp(expected.name,"sdrpp")==0)continue;
        const char* found=nullptr;
        const mach_header* foundHeader=nullptr;
        for(uint32_t i=0;i<_dyld_image_count();++i){
            const char* path=_dyld_get_image_name(i);
            if(!path)continue;
            const auto* slash=std::strrchr(path,'/');
            const auto* leaf=slash?slash+1:path;
            // The real factory always verifies the main executable. Only the
            // separate installer-probe export may omit it (Python is its main).
            const auto* header=_dyld_get_image_header(i);
            if(std::strcmp(expected.name,"sdrpp")==0 && (!header || header->filetype!=MH_EXECUTE))continue;
            if(std::strcmp(leaf,expected.name)!=0)continue;
            if(found)return std::string("Ambiguous loaded host library: ")+expected.name;
            found=path;
            foundHeader=header;
        }
        const auto uuid=imageUUID(foundHeader);
        const char* reason=!found?"not loaded":uuid.empty()?"missing UUID":!std::strstr(expected.uuids,uuid.c_str())?"loaded UUID differs":nullptr;
        if(!reason && digest(found)!=expected.sha256)reason="file fingerprint differs or cannot be read";
        if(reason)
            return std::string("Unverified host image: ")+expected.name+" ("+reason+"). Quit the host and rerun the matching Maverick installer; a host update may require a matching build.";
    }
    return {};
}
}
