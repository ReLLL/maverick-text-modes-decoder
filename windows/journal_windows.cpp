// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "journal.h"
#include <cerrno>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <io.h>
#include <windows.h>

namespace cw {
namespace {
void check(bool ok,const char* action){if(!ok)throw std::runtime_error(std::string(action)+": "+std::strerror(errno));}
void systemError(){const auto code=GetLastError();errno=code==ERROR_FILE_EXISTS||code==ERROR_ALREADY_EXISTS?EEXIST:code==ERROR_FILE_NOT_FOUND||code==ERROR_PATH_NOT_FOUND?ENOENT:code==ERROR_ACCESS_DENIED||code==ERROR_SHARING_VIOLATION?EACCES:EIO;}
int linkFile(const wchar_t* source,const wchar_t* target){if(CreateHardLinkW(target,source,nullptr))return 0;systemError();return -1;}
int deleteFile(const wchar_t* path){if(DeleteFileW(path))return 0;systemError();return -1;}
const std::string header="# CW reception log\n\nDecoded locally with Maverick Decoder. Times are UTC.\nAutomatic decoding can contain errors; `[?]` marks unrecognized Morse.\n";
}
std::string utc(const char* format){auto now=std::time(nullptr);std::tm t{};gmtime_r(&now,&t);char buf[80];std::strftime(buf,sizeof(buf),format,&t);return buf;}
Journal::Journal(std::filesystem::path folder):folder_(std::move(folder)){}
Journal::~Journal(){close();}
void Journal::close(){if(fd_>=0){::_commit(fd_);::_close(fd_);fd_=-1;}}
void Journal::open(bool exclusive){
    frequency_=-1;lastWrite_=0;column_=0;
    std::filesystem::create_directories(folder_);
    HANDLE handle=CreateFileW(path_.c_str(),FILE_APPEND_DATA|GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,
        exclusive?CREATE_NEW:OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    if(handle==INVALID_HANDLE_VALUE){systemError();fd_=-1;}
    else{
        BY_HANDLE_FILE_INFORMATION info{};
        if(!GetFileInformationByHandle(handle,&info)||(info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))){
            CloseHandle(handle);throw std::runtime_error("Log must be an ordinary file, not a link or directory");
        }
        fd_=_open_osfhandle(intptr_t(handle),_O_WRONLY|_O_APPEND|_O_BINARY|_O_NOINHERIT);
        if(fd_<0)CloseHandle(handle);
    }
    check(fd_>=0,"Open log");
    struct _stat64 st{};
    if(::_fstat64(fd_,&st)!=0 || (st.st_mode&_S_IFMT)!=_S_IFREG){close();throw std::runtime_error("Log must be a regular file");}
    if(st.st_size==0){write(header);sync();}
}
void Journal::write(const std::string& text){
    if(fd_<0)throw std::runtime_error("No open log");
    size_t pos=0;
    while(pos<text.size()){
        auto n=::_write(fd_,text.data()+pos,unsigned(std::min<size_t>(text.size()-pos,65536)));
        if(n<0 && errno==EINTR)continue;
        check(n>0,"Write log");pos+=size_t(n);
    }
}
void Journal::sync(){if(fd_>=0)check(::_commit(fd_)==0,"Sync log");}
std::string Journal::validName(const std::string& name){
    if(name.empty() || name.size()>180 || name.front()=='.' || name.find_first_of("/\\\n\r\t")!=std::string::npos)
        throw std::runtime_error("Use a filename without folders, starting with a letter or number");
    if(name.find_first_of("<>:\"|?*")!=std::string::npos||name.back()==' '||name.back()=='.')throw std::runtime_error("Filename has Windows-reserved characters");
    auto stem=name.substr(0,name.find('.'));for(auto& c:stem)c=char(std::toupper(static_cast<unsigned char>(c)));
    if(stem=="CON"||stem=="PRN"||stem=="AUX"||stem=="NUL"||(stem.size()==4&&(stem.substr(0,3)=="COM"||stem.substr(0,3)=="LPT")&&stem[3]>='0'&&stem[3]<='9'))throw std::runtime_error("Filename is reserved by Windows");
    for(unsigned char c:name)if(c<32)throw std::runtime_error("Filename contains a control character");
    return name.size()>=3 && name.substr(name.size()-3)==".md"?name:name+".md";
}
void Journal::create(double frequency){
    close();
    std::ostringstream base;base<<"CW_"<<utc("%Y-%m-%d_%H-%M-%SZ")<<"_"<<std::fixed<<std::setprecision(3)<<frequency/1000<<"kHz";
    for(int n=0;n<10000;n++){
        path_=folder_/(base.str()+(n?"_"+std::to_string(n):"")+".md");
        try{open(true);return;}catch(const std::exception&){if(errno!=EEXIST)throw;}
    }
    throw std::runtime_error("Could not choose an unused log filename");
}
void Journal::resume(const std::string& filename){close();path_=folder_/std::filesystem::u8path(validName(filename));open(false);}
void Journal::append(const std::string& text,double frequency,float wpm){
    if(text.empty())return;
    if(fd_<0)open(false);
    std::ostringstream row;
    auto now=std::time(nullptr);
    if(std::abs(frequency-frequency_)>5 || now-lastWrite_>15){
        row<<"\n\n## "<<utc()<<" UTC\n\n**"<<std::fixed<<std::setprecision(3)<<frequency/1000<<" kHz | "<<std::setprecision(1)<<wpm<<" WPM estimated**\n\n";
        frequency_=frequency;column_=0;
    }
    for(char c:text){
        if(c=='\n'){row<<'\n';column_=0;continue;}
        if(c==' ' && column_>78){row<<'\n';column_=0;continue;}
        if(c=='|' || c=='\\' || c=='`' || c=='<' || c=='>' || c=='*' || c=='_')row<<'\\';
        row<<(c=='\r'?' ':c);column_++;
    }
    write(row.str());sync();lastWrite_=now;
}
void Journal::rename(const std::string& name){
    auto target=folder_/std::filesystem::u8path(validName(name));if(target==path_)return;
    sync();
    // Exclusive link prevents clobbering another file, including races and symlinks.
    check(linkFile(path_.c_str(),target.c_str())==0,"Rename log");
    if(deleteFile(path_.c_str())!=0){deleteFile(target.c_str());check(false,"Rename log");}
    path_=target;
}
std::filesystem::path Journal::clear(){
    auto old=path_;sync();
    std::filesystem::path archive;
    for(int n=0;n<10000;n++){
        archive=folder_/(old.stem().u8string()+".cleared-"+utc("%Y%m%d-%H%M%SZ")+(n?"-"+std::to_string(n):"")+".md");
        if(linkFile(old.c_str(),archive.c_str())==0)break;
        if(errno!=EEXIST)check(false,"Archive log");
        if(n==9999)throw std::runtime_error("Could not create log backup");
    }
    close();check(deleteFile(old.c_str())==0,"Clear log");
    path_=old;open(true);return archive;
}
}
