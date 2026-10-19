// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sharp_host.h"
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <cmath>
namespace notebook {
namespace {
bool transfer(int fd,void* data,size_t bytes,bool writing){
    auto* cursor=static_cast<char*>(data);
    while(bytes){int n=writing?_write(fd,cursor,unsigned(std::min<size_t>(bytes,65536))):_read(fd,cursor,unsigned(std::min<size_t>(bytes,65536)));
        if(n<=0)return false;cursor+=n;bytes-=size_t(n);}
    return true;
}
}
SharpHost& sharpHost(){static SharpHost host;return host;}
void SharpHost::start(){_setmode(0,_O_BINARY);_setmode(1,_O_BINARY);reader_=std::thread(&SharpHost::read,this);writer_=std::thread(&SharpHost::write,this);}
void SharpHost::finish(){
    alive=false;wake_.notify_all();
    auto cancel=[](unsigned long id){if(!id)return;auto h=OpenThread(THREAD_TERMINATE,FALSE,id);if(h){CancelSynchronousIo(h);CloseHandle(h);}};
    while((reader_.joinable()&&!readerDone_)||(writer_.joinable()&&!writerDone_)){
        if(!readerDone_)cancel(readerId_);if(!writerDone_)cancel(writerId_);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if(reader_.joinable())reader_.join();if(writer_.joinable())writer_.join();
}
void SharpHost::plan(Json value){
    std::lock_guard<std::mutex> lock(mutex_);value["kind"]="plan";
    if(!request_.is_null()){value["action"]=request_;value["action_id"]=requestId_;}
    if(plan_==value)return;plan_=std::move(value);dirty_=true;wake_.notify_one();
}
void SharpHost::request(Json value){std::lock_guard<std::mutex> lock(mutex_);request_=std::move(value);++requestId_;}
void SharpHost::read(){readerId_=GetCurrentThreadId();try{while(alive){
    uint32_t length=0;if(!transfer(0,&length,4,false)||!length||length>65536)break;
    std::string body(length,'\0');if(!transfer(0,body.data(),length,false))break;
    auto header=Json::parse(body);const auto kind=header.at("kind").get<std::string>();
    if(kind=="metadata"){
        SharpState next;next.center=header.at("center");next.frequency=header.at("frequency");next.rate=header.at("sample_rate");next.width=header.at("bandwidth");
        next.mode=header.at("mode").get<std::string>();next.playing=header.at("playing").get<bool>();next.epoch=header.at("epoch").get<uint64_t>();
        if(!std::isfinite(next.center)||!std::isfinite(next.frequency)||!std::isfinite(next.rate)||!std::isfinite(next.width)||
           next.center<0||next.center>1e12||next.frequency<=0||next.frequency>1e12||next.rate<0||next.rate>100e6||next.width<0||next.width>1e7||next.mode.size()>16)break;
        bool changed=false;{std::lock_guard<std::mutex> lock(mutex_);changed=state_.epoch!=next.epoch||(state_.playing&&!next.playing);state_=next;}
        if(changed&&consume)consume({{{"kind","reset"}}, {}});
    }else if(kind=="show")show=true;
    else if(kind=="reset"){if(consume)consume({std::move(header),{}});}
    else if(kind=="iq"){
        auto count=header.at("samples").get<int64_t>();const auto channel=header.at("channel").get<std::string>();
        double rate=header.at("sample_rate"),center=header.at("center"),frequency=header.at("frequency");
        if(count<=0||count>32768||(channel!="cw"&&channel!="digital")||!std::isfinite(rate)||rate<4000||rate>4096000||
           !std::isfinite(center)||center<0||center>1e12||!std::isfinite(frequency)||frequency<=0||frequency>1e12)break;
        SharpBlock block{std::move(header),std::vector<std::complex<float>>(size_t(count))};
        if(!transfer(0,block.iq.data(),size_t(count)*8,false))break;
        const auto current=state();
        const bool tuned=channel=="digital"||(frequency==current.frequency&&block.header.at("mode")==current.mode&&block.header.at("filter_width")==current.width);
        if(current.playing&&tuned&&block.header.at("epoch").get<uint64_t>()==current.epoch&&consume)consume(std::move(block));
    }else break;
}}catch(const std::exception&){}alive=false;readerDone_=true;wake_.notify_all();}
void SharpHost::write(){writerId_=GetCurrentThreadId();while(alive){
    Json value;{std::unique_lock<std::mutex> lock(mutex_);wake_.wait_for(lock,std::chrono::milliseconds(200),[&]{return dirty_||!alive;});
        if(!alive)break;if(!dirty_)continue;value=plan_;dirty_=false;}
    auto body=value.dump();uint32_t length=uint32_t(body.size());
    if(length>65536||!transfer(1,&length,4,true)||!transfer(1,body.data(),length,true)){alive=false;break;}
}writerDone_=true;}
}
