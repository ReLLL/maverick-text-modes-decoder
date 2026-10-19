// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "flag_textures.h"
#include "sharp_host.h"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <GL/gl.h>
#include <imgui.h>
#include <vector>
namespace notebook {
namespace {
struct Imaging {ULONG_PTR token=0;Imaging(){Gdiplus::GdiplusStartupInput input;Gdiplus::GdiplusStartup(&token,&input,nullptr);}~Imaging(){if(token)Gdiplus::GdiplusShutdown(token);}};
}
FlagTextures::~FlagTextures(){for(auto& entry:textures_)if(entry.second)glDeleteTextures(1,&entry.second);}
unsigned int FlagTextures::get(const std::string& code){
    if(code.size()!=2||code[0]<'A'||code[0]>'Z'||code[1]<'A'||code[1]>'Z')return 0;
    auto old=textures_.find(code);if(old!=textures_.end())return old->second;
    static Imaging imaging;const auto file=sharpHost().binary/"flags"/(code+".png");
    Gdiplus::Bitmap image(file.c_str());unsigned int texture=0;
    if(image.GetLastStatus()==Gdiplus::Ok&&image.GetWidth()<=256&&image.GetHeight()<=256){
        const int w=int(image.GetWidth()),h=int(image.GetHeight());Gdiplus::Rect rectangle(0,0,w,h);Gdiplus::BitmapData source{};
        if(image.LockBits(&rectangle,Gdiplus::ImageLockModeRead,PixelFormat32bppARGB,&source)==Gdiplus::Ok){
            std::vector<unsigned char> rgba(size_t(w)*h*4);
            for(int y=0;y<h;++y){auto* row=static_cast<unsigned char*>(source.Scan0)+y*source.Stride;
                for(int x=0;x<w;++x){auto at=(size_t(y)*w+x)*4;rgba[at]=row[x*4+2];rgba[at+1]=row[x*4+1];rgba[at+2]=row[x*4];rgba[at+3]=row[x*4+3];}}
            image.UnlockBits(&source);GLint previous=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&previous);
            glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());glBindTexture(GL_TEXTURE_2D,GLuint(previous));
        }
    }
    textures_.emplace(code,texture);return texture;
}
void FlagTextures::draw(const std::string& code){const auto texture=get(code);const auto size=ImGui::GetTextLineHeight();if(texture)ImGui::Image(reinterpret_cast<ImTextureID>(uintptr_t(texture)),{size*1.5f,size});else ImGui::Dummy({size*1.5f,size});ImGui::SameLine(0,5);}
}
