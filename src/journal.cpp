// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "journal.h"
#include <cerrno>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace cw {
namespace {
void check(bool ok,const char* action){if(!ok)throw std::runtime_error(std::string(action)+": "+std::strerror(errno));}
const std::string header="# CW reception log\n\nDecoded locally with Maverick Decoder. Times are UTC.\nAutomatic decoding can contain errors; `[?]` marks unrecognized Morse.\n";
}
std::string utc(const char* format){auto now=std::time(nullptr);std::tm t{};gmtime_r(&now,&t);char buf[80];std::strftime(buf,sizeof(buf),format,&t);return buf;}
Journal::Journal(std::filesystem::path folder):folder_(std::move(folder)){}
Journal::~Journal(){close();}
void Journal::close(){if(fd_>=0){::fsync(fd_);::close(fd_);fd_=-1;}}
void Journal::open(bool exclusive){
    frequency_=-1;lastWrite_=0;column_=0;
    std::filesystem::create_directories(folder_);
    fd_=::open(path_.c_str(),O_WRONLY|O_APPEND|O_CREAT|O_NOFOLLOW|(exclusive?O_EXCL:0),0600);
    check(fd_>=0,"Open log");
    struct stat st{};
    if(::fstat(fd_,&st)!=0 || !S_ISREG(st.st_mode)){close();throw std::runtime_error("Log must be a regular file");}
    if(st.st_size==0){write(header);sync();}
}
void Journal::write(const std::string& text){
    if(fd_<0)throw std::runtime_error("No open log");
    size_t pos=0;
    while(pos<text.size()){
        auto n=::write(fd_,text.data()+pos,text.size()-pos);
        if(n<0 && errno==EINTR)continue;
        check(n>0,"Write log");pos+=size_t(n);
    }
}
void Journal::sync(){if(fd_>=0)check(::fsync(fd_)==0,"Sync log");}
std::string Journal::validName(const std::string& name){
    if(name.empty() || name.size()>180 || name.front()=='.' || name.find_first_of("/\\\n\r\t")!=std::string::npos)
        throw std::runtime_error("Use a filename without folders, starting with a letter or number");
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
void Journal::resume(const std::string& filename){close();path_=folder_/validName(filename);open(false);}
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
    auto target=folder_/validName(name);if(target==path_)return;
    sync();
    // Exclusive link prevents clobbering another file, including races and symlinks.
    check(::link(path_.c_str(),target.c_str())==0,"Rename log");
    if(::unlink(path_.c_str())!=0){::unlink(target.c_str());check(false,"Rename log");}
    path_=target;
}
std::filesystem::path Journal::clear(){
    auto old=path_;sync();
    std::filesystem::path archive;
    for(int n=0;n<10000;n++){
        archive=folder_/(old.stem().string()+".cleared-"+utc("%Y%m%d-%H%M%SZ")+(n?"-"+std::to_string(n):"")+".md");
        if(::link(old.c_str(),archive.c_str())==0)break;
        if(errno!=EEXIST)check(false,"Archive log");
        if(n==9999)throw std::runtime_error("Could not create log backup");
    }
    close();check(::unlink(old.c_str())==0,"Clear log");
    path_=old;open(true);return archive;
}
}
