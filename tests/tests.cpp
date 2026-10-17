// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include "journal.h"
#include "engine.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unistd.h>
void require(bool b,const std::string& msg){if(!b)throw std::runtime_error(msg);}
std::string symbols(char c){for(const auto&s:std::vector<std::string>{".-","-...","-.-.","-..",".","..-.","--.","....","..",".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",".--","-..-","-.--","--..","-----",".----","..---","...--","....-",".....","-....","--...","---..","----.","-...-","-..-."})if(cw::morse(s)==std::string(1,c))return s;return "";}
std::vector<float> audio(const std::string& text,float wpm,int rate,float tone,float noise=0,bool fading=false){
    std::vector<float> result;std::mt19937 rng(42);std::normal_distribution<float> n(0,noise);double t=0;
    auto run=[&](bool on,double units){int count=std::lround(units*1.2/wpm*rate);for(int i=0;i<count;i++){
        float amplitude=fading?(.4f+.3f*std::sin(t*2)):0.6f;
        result.push_back((on?amplitude*std::sin(2*3.141592653589793*t*tone):0)+n(rng));t+=1.0/rate;}};
    run(false,8);
    for(char c:text){if(c==' '){run(false,4);continue;}for(char s:symbols(c)){run(true,s=='.'?1:3);run(false,1);}run(false,2);}
    run(false,10);return result;
}
void testTiming(){
    for(float wpm:{3,5,12,20,40,80,120,200}){
        cw::Timing t;t.extendedSpeed(true);std::string out;t.output=[&](const std::string&s){out+=s;};t.reset();
        auto run=[&](bool k,double u){for(int i=0;i<std::lround(u*1.2/wpm*1000);i++)t.tick(k);};
        for(char c:std::string("VVV DE 4XZ TEST 123")){if(c==' '){run(false,4);continue;}for(char s:symbols(c)){run(true,s=='.'?1:3);run(false,1);}run(false,2);}run(false,10);
        require(out.find("DE 4XZ TEST 123")!=std::string::npos,"Timing "+std::to_string(wpm)+": "+out);
    }
}
void testAudio(){
    for(auto wpm:{3,5,8,12,18,20,40,80,120,200}){
        auto samples=audio("VVV VVV DE 4XZ TEST 123",wpm,48000,800,.015,true);
        cw::Decoder d;d.extendedSpeed(true);std::string out;d.output=[&](const std::string&s){out+=s;};
        for(size_t i=0;i<samples.size();i+=977)d.process(samples.data()+i,std::min(size_t(977),samples.size()-i),48000);
        std::cout<<wpm<<" WPM: "<<out<<"\n";
        require(out.find("DE 4XZ TEST 123")!=std::string::npos,"Audio decode failed "+std::to_string(wpm));
    }
    cw::Decoder d;std::string out;d.output=[&](const std::string&s){out+=s;};
    auto noise=audio(std::string(30,' '),20,48000,800,.02);
    d.process(noise.data(),noise.size(),48000);
    require(out.empty(),"Noise generated text: "+out);
    for(int rate:{8000,44100,96000})for(float tone:{300.f,1500.f}){
        auto samples=audio("VVV VVV DE 4XZ TEST 123",20,rate,tone,.005);
        cw::Decoder other;std::string text;other.output=[&](const std::string&s){text+=s;};
        other.process(samples.data(),samples.size(),rate);
        require(text.find("DE 4XZ TEST 123")!=std::string::npos,"Rate/tone "+std::to_string(rate)+"/"+std::to_string(tone)+": "+text);
    }
}
void testRecording(){
#ifdef CW_FIXTURE
    std::ifstream in(CW_FIXTURE,std::ios::binary);require(bool(in),"Reception fixture");
    cw::Decoder d;std::string out;d.output=[&](const std::string&s){out+=s;};std::vector<float> block(500);
    while(in.read(reinterpret_cast<char*>(block.data()),2000) || in.gcount()>0)d.process(block.data(),in.gcount()/4,8000);
    for(const std::string& expected:{"NG0Q NR 001 TO NR 501 QQL","ZH4Y NR 801 II GR 02","SK0S NR 701 RW GR 52"})
        require(out.find(expected)!=std::string::npos,"Live fixture missing "+expected+": "+out);
    std::ifstream fading(std::filesystem::path(CW_FIXTURE).parent_path()/"4xz-fading-8k.f32",std::ios::binary);
    require(bool(fading),"Fading fixture");d.reset(25);out.clear();
    while(fading.read(reinterpret_cast<char*>(block.data()),2000) || fading.gcount()>0)d.process(block.data(),fading.gcount()/4,8000);
    require(out.find("VVV DE 4XZ 4XZ")!=std::string::npos,"Fading manual-speed fallback: "+out);
#else
    std::cout<<"Private reception fixtures not configured; synthetic coverage remains enabled\n";
#endif
}
void testJournal(){
    auto dir=std::filesystem::temp_directory_path()/("cw-notebook-test-"+std::to_string(getpid()));
    std::filesystem::create_directories(dir);
    {cw::Journal j(dir);j.create(6606960);j.append("VVV DE 4XZ | <AR>",6606960,20);j.rename("Navy reception");
        require(j.path().filename()=="Navy reception.md","Rename");
        std::ofstream(dir/"occupied.md")<<"PRESERVE";
        bool rejected=false;try{j.rename("occupied.md");}catch(...){rejected=true;}require(rejected,"Clobber refusal");
        rejected=false;try{j.rename("../escape");}catch(...){rejected=true;}require(rejected,"Traversal refusal");
        auto archive=j.clear();require(std::filesystem::exists(archive),"Clear preserves backup");
        std::ifstream in(archive);std::string text((std::istreambuf_iterator<char>(in)),{});require(text.find("VVV DE 4XZ")!=std::string::npos,"Backup content");
        j.append("NEW",7000000,25);
    }
    {cw::Journal j(dir);j.resume("Navy reception.md");j.append("RESUMED",7000000,25);}
    {cw::Journal j(dir);std::filesystem::create_symlink(dir/"occupied.md",dir/"link.md");bool rejected=false;try{j.resume("link.md");}catch(...){rejected=true;}require(rejected,"Symlink refusal");}
    {cw::Journal j(dir);std::filesystem::create_directory(dir/"retry.md");
        bool rejected=false;try{j.resume("retry.md");}catch(...){rejected=true;}require(rejected,"Unavailable log rejected");
        std::filesystem::remove(dir/"retry.md");j.append("RECOVERED",6606960,25);
        std::ifstream in(dir/"retry.md");std::string text((std::istreambuf_iterator<char>(in)),{});
        require(text.find("RECOVERED")!=std::string::npos,"Retry reopens recovered destination");}
    std::filesystem::remove_all(dir);
}
template<class Predicate> cw::Snapshot waitFor(cw::Engine& engine,Predicate done){
    for(int i=0;i<300;i++){auto s=engine.snapshot();if(done(s))return s;std::this_thread::sleep_for(std::chrono::milliseconds(10));}
    auto s=engine.snapshot();
    throw std::runtime_error("Engine operation timed out: text="+s.text+" error="+s.error+" path="+s.path+" drops="+std::to_string(s.dropped));
}
std::string savedOriginal(const std::string& content){
    // These short fixtures fit on each record line. Autosave may split a
    // phrase at any character; recovery records must never satisfy this check.
    std::istringstream lines(content);std::string line,result;
    const std::string prefix="Original decode: ";
    while(std::getline(lines,line))if(line.compare(0,prefix.size(),prefix)==0)
        result+=line.substr(prefix.size());
    return result;
}
void testEngine(){
    auto dir=std::filesystem::temp_directory_path()/("cw-engine-test-"+std::to_string(getpid()));
    std::filesystem::create_directories(dir);
    std::string log;
    {
        cw::Engine engine(dir);
        auto samples=audio("VVV VVV DE 4XZ TEST 123",20,8000,800,.005);
        bool splitSaved=false;
        for(size_t i=0;i<samples.size();i+=200){
            engine.audio(std::vector<float>(samples.begin()+i,samples.begin()+std::min(i+200,samples.size())),8000,6606960);
            if(!splitSaved && engine.snapshot().text.find("DE 4XZ")!=std::string::npos){
                engine.command("save");splitSaved=true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        auto s=waitFor(engine,[](const auto& x){return x.text.find("DE 4XZ TEST 123")!=std::string::npos;});
        require(s.recoveredText.find("DE 4XZ TEST 123")!=std::string::npos,"Recovery stream absent");
        require(s.timingRecovery,"Recovery defaults");
        const auto original=s.text;
        engine.command("timing_recovery","0");
        s=waitFor(engine,[](const auto& x){return !x.timingRecovery;});
        require(s.text==original,"Recovery switch rewrote original");
        require(s.dropped==0,"Unexpected drop");
        engine.command("save");s=waitFor(engine,[](const auto& x){return x.saved>15 && !x.path.empty();});
        engine.command("rename","Engine log");s=waitFor(engine,[](const auto& x){return x.path.find("Engine log.md")!=std::string::npos;});
        log=s.path;require(s.error.empty(),"Engine file error");
        engine.command("rename","../bad");waitFor(engine,[](const auto& x){return !x.error.empty();});
        engine.command("save");waitFor(engine,[](const auto& x){return x.error.empty();});
        engine.command("clear_view");waitFor(engine,[](const auto& x){return x.text.empty() && x.transcriptReset==1;});
        std::ifstream in(log);std::string content((std::istreambuf_iterator<char>(in)),{});
        require(splitSaved,"Mid-message save was not exercised");
        require(savedOriginal(content).find("DE 4XZ TEST 123")!=std::string::npos,"Markdown spacing and clear-view persistence");
        for(size_t i=0;i<samples.size();i+=200){
            engine.audio(std::vector<float>(samples.begin()+i,samples.begin()+std::min(i+200,samples.size())),8000,6606960);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        s=waitFor(engine,[](const auto& x){return x.text.find("DE 4XZ TEST 123")!=std::string::npos;});
        const auto measured=s.reception;
        require(std::isfinite(measured.snr)&&measured.snrAt>0&&measured.lastTextAt>0&&measured.lastTextUtc>0,"CW reception metadata was never populated");
        engine.command("pause","1");s=waitFor(engine,[](const auto& x){return x.paused&&x.pauseRevision==1;});
        require(s.reception.snr==measured.snr&&s.reception.lastTextUtc==measured.lastTextUtc,"CW pause erased retained SNR or refreshed received time");
        auto pausedText=s.text;auto pausedBlocks=s.blocks;
        engine.audio(audio("EE",20,8000,800),8000,6606960);
        s=waitFor(engine,[&](const auto& x){return x.blocks>pausedBlocks;});
        require(s.paused&&s.text==pausedText&&s.decoder.bufferedRuns==0&&s.recoveredDecoder.bufferedRuns==0,"Paused CW decoded or kept learning");
        engine.command("pause","0");s=waitFor(engine,[](const auto& x){return !x.paused&&x.pauseRevision==2;});
        require(s.text==pausedText&&!s.decoder.calibrated&&s.decoder.bufferedRuns==0,"CW resume lost text or reused old timing");
        require(s.reception.snr==measured.snr&&s.reception.lastTextUtc==measured.lastTextUtc,"CW resume erased held metadata before new reception");
        engine.command("pause","1");waitFor(engine,[](const auto& x){return x.paused&&x.pauseRevision==3;});
        // Failed archival must keep the transcript, including its held UI view.
        auto retained=engine.snapshot().text;
        std::filesystem::rename(log,dir/"retained.md");
        engine.command("clear_log");waitFor(engine,[](const auto& x){return !x.error.empty();});
        s=engine.snapshot();require(s.text==retained && s.transcriptReset==1,"Failed clear preserves transcript");
        std::filesystem::rename(dir/"retained.md",log);
        engine.command("clear_log");s=waitFor(engine,[](const auto& x){return x.transcriptReset==2;});
        require(s.text.empty() && s.recoveredText.empty() && s.error.empty(),"Clear log clears both transcripts after successful archive");
        bool archived=false;
        for(const auto& file:std::filesystem::directory_iterator(dir)){
            if(file.path().filename().string().find(".cleared-")==std::string::npos)continue;
            std::ifstream backup(file.path());std::string text((std::istreambuf_iterator<char>(backup)),{});
            archived=savedOriginal(text).find("DE 4XZ TEST 123")!=std::string::npos;
        }
        require(archived,"Clear log archives decoded text");
        engine.command("extended_speed","1");
        engine.command("wpm","25");waitFor(engine,[](const auto& x){return x.manualWpm==25&&x.extendedSpeed;});
    }
    {cw::Engine recovered(dir);auto s=waitFor(recovered,[](const auto& x){return !x.path.empty();});require(s.path==log,"Restart recovers renamed log");require(s.manualWpm==25,"Restart restores decoder preference");require(!s.timingRecovery,"Restart restores recovery choices");require(s.extendedSpeed,"Restart restores automatic speed range");}
    std::filesystem::remove_all(dir);
}
void testIdleSpacing(){
    auto dir=std::filesystem::temp_directory_path()/("cw-idle-test-"+std::to_string(getpid()));
    {
        cw::Engine engine(dir);
        uint64_t blocks=0;
        auto feed=[&](std::vector<float> samples,int rate,double frequency){
            engine.audio(std::move(samples),rate,frequency);++blocks;
            return waitFor(engine,[&](const auto& s){return s.blocks==blocks;});
        };
        for(int i=0;i<10;i++)feed(std::vector<float>(80),8000,7000000+i*100);
        require(engine.snapshot().text.empty(),"Silent tuning must not create blank lines");
        auto signal=audio("VVV VVV DE 4XZ TEST 123 ==",20,8000,800,.005);
        auto receive=[&]{for(size_t i=0;i<signal.size();i+=8000)
            feed(std::vector<float>(signal.begin()+i,signal.begin()+std::min(i+8000,signal.size())),8000,6606960);};
        receive();
        auto text=engine.snapshot().text;
        require(text.find("DE 4XZ TEST 123")!=std::string::npos && text.find("\n\n")!=std::string::npos,"Keep word and paragraph spacing");
        for(int i=0;i<120;i++)feed(std::vector<float>(8000),8000,6606960);
        require(engine.snapshot().text==text,"Two minutes of silence must not grow transcript");
        for(int i=0;i<20;i++)feed(std::vector<float>(80),i%2?8000:4000,7000000+i*100);
        require(engine.snapshot().text==text,"Silent retunes and rate resets must not grow transcript");
        receive();
        auto resumed=engine.snapshot();
        require(resumed.text.size()>text.size(),"Decoding resumes after silence");
        require(resumed.text.find("\n\n\n")==std::string::npos,"No accumulated empty paragraphs");
        require(resumed.dropped==0,"Idle test processed all audio");
        // Station changes must resume reception, add exactly one line break,
        // and preserve the entire preceding decode in both views.
        std::string before=resumed.text;
        engine.command("wpm","25");engine.command("tone","800");
        waitFor(engine,[](const auto& s){return s.manualWpm==25&&s.manualTone==800;});
        engine.command("retune","14021960");
        waitFor(engine,[](const auto& s){return s.frequency==14021960;});
        require(engine.snapshot().manualWpm==0&&engine.snapshot().manualTone==0,"Retune releases old station locks");
        require(engine.snapshot().text==before,"Silent retune inserted whitespace");
        for(size_t i=0;i<signal.size();i+=8000)
            feed(std::vector<float>(signal.begin()+i,signal.begin()+std::min(i+8000,signal.size())),8000,14021960);
        auto after=engine.snapshot();
        auto trimmed=before.substr(0,before.find_last_not_of(" \n")+1);
        require(after.text.compare(0,trimmed.size(),trimmed)==0,"Retune lost previous decode");
        require(after.text[trimmed.size()]=='\n'&&after.text[trimmed.size()+1]!='\n',"Retune needs one new line");
        require(after.text.find("DE 4XZ TEST 123",trimmed.size())!=std::string::npos,"Retune stopped decoding");
        engine.command("retune","14021961");
        auto learning=waitFor(engine,[](const auto& s){return s.frequency==14021961;});
        require(!learning.decoder.calibrated&&learning.decoder.wpm==0,"One-Hz retune must discard speed estimate");
        auto slow=audio("VVV VVV CQ CQ DE TEST SLOW CW",8,8000,800,.005);
        for(size_t i=0;i<slow.size();i+=8000)
            feed(std::vector<float>(slow.begin()+i,slow.begin()+std::min(i+8000,slow.size())),8000,14021961);
        auto adapted=engine.snapshot();
        require(adapted.text.find("CQ CQ DE TEST SLOW CW",after.text.size())!=std::string::npos,"Retune did not decode slower station");
        require(adapted.decoder.calibrated&&std::abs(adapted.decoder.wpm-8)<1,"Retune kept previous station speed");
        engine.command("retune","14021962");
        waitFor(engine,[](const auto& s){return s.frequency==14021962;});
        engine.command("tone","800");waitFor(engine,[](const auto& s){return s.manualTone==800;});
        auto ambiguous=audio("EEEEEE",12,8000,800);
        for(size_t i=0;i<ambiguous.size();i+=8000)
            feed(std::vector<float>(ambiguous.begin()+i,ambiguous.begin()+std::min(i+8000,ambiguous.size())),8000,14021962);
        auto buffered=engine.snapshot();
        require(buffered.decoder.bufferedRuns>0&&buffered.recoveredDecoder.bufferedRuns>0,"Retune test needs held learning data in both paths");
        engine.command("retune","14021963");
        auto cleared=waitFor(engine,[](const auto& s){return s.frequency==14021963;});
        require(cleared.decoder.bufferedRuns==0&&cleared.recoveredDecoder.bufferedRuns==0,"One-Hz retune must clear both learning buffers");
        require(cleared.text==buffered.text,"Retune must preserve already confirmed text");
        engine.command("auto_retune","0");engine.command("wpm","25");engine.command("pause","1");
        waitFor(engine,[](const auto& s){return s.paused&&s.manualWpm==25&&!s.autoRetune;});
        engine.command("retune","7021960");
        auto held=waitFor(engine,[](const auto& s){return s.frequency==7021960;});
        require(held.paused&&held.manualWpm==25,"Retune must honor Pause and optional manual lock retention");
    }
    std::filesystem::remove_all(dir);
}
int main(){try{testTiming();testAudio();testRecording();testJournal();testEngine();testIdleSpacing();std::cout<<"All tests passed\n";}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
