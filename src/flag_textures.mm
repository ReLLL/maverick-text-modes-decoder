// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#import <AppKit/AppKit.h>
#include "flag_textures.h"
#include <imgui.h>
#include <utils/opengl_include_code.h>
#include <algorithm>
#include <cstring>
#include <vector>
namespace notebook {
namespace {
std::vector<unsigned char> pixels(const std::string& code){@autoreleasepool{
    NSString* glyph=@"\U0001F3F3\uFE0F";
    if(!code.empty()){
        unichar chars[]={0xD83C,unichar(0xDDE6+code[0]-'A'),0xD83C,unichar(0xDDE6+code[1]-'A')};
        glyph=[NSString stringWithCharacters:chars length:4];
    }
    NSBitmapImageRep* bitmap=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nil pixelsWide:64 pixelsHigh:64 bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bitmapFormat:0 bytesPerRow:256 bitsPerPixel:32];
    if(!bitmap)return {};
    std::memset(bitmap.bitmapData,0,64*256);[NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithBitmapImageRep:bitmap]];
    NSFont* font=[NSFont fontWithName:@"Apple Color Emoji" size:54]?:[NSFont systemFontOfSize:54];
    NSDictionary* attributes=@{NSFontAttributeName:font};NSSize size=[glyph sizeWithAttributes:attributes];
    [glyph drawAtPoint:NSMakePoint((64-size.width)/2,(64-size.height)/2) withAttributes:attributes];[NSGraphicsContext restoreGraphicsState];
    std::vector<unsigned char> out(bitmap.bitmapData,bitmap.bitmapData+64*256);[bitmap release];
    for(size_t i=0;i<out.size();i+=4)if(out[i+3])for(int c=0;c<3;c++)out[i+c]=uint8_t(std::min(255u,unsigned(out[i+c])*255/unsigned(out[i+3])));
    return out;
}}
unsigned int upload(const std::vector<unsigned char>& data){
    if(data.size()!=64*64*4)return 0;
    GLint old=0,row=0,alignment=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&old);glGetIntegerv(GL_UNPACK_ROW_LENGTH,&row);glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    GLuint texture=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,data.data());
    glBindTexture(GL_TEXTURE_2D,GLuint(old));glPixelStorei(GL_UNPACK_ROW_LENGTH,row);glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);return texture;
}
}
FlagTextures::~FlagTextures(){for(const auto& entry:textures_)if(entry.second){auto texture=entry.second;glDeleteTextures(1,&texture);}}
unsigned int FlagTextures::get(const std::string& code){
    if(!code.empty()&&(code.size()!=2||code[0]<'A'||code[0]>'Z'||code[1]<'A'||code[1]>'Z'))return 0;
    auto it=textures_.find(code);if(it==textures_.end())it=textures_.emplace(code,upload(pixels(code))).first;
    return it->second;
}
void FlagTextures::draw(const std::string& code){
    const auto texture=get(code);
    float size=ImGui::GetTextLineHeight();
    if(texture)ImGui::Image(reinterpret_cast<ImTextureID>(uintptr_t(texture)),{size,size});else ImGui::Dummy({size,size});
    ImGui::SameLine(0,5);
}
}
