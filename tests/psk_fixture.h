// Copyright (c) 2026 ReLLL and contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "rtty_fft.h"
#include <map>
#include <string>
#include <cmath>
#include <stdexcept>
namespace fixture {
// Independently transcribed ASCII Varicode values for these reference messages.
inline const std::map<char,std::string> codes{
 {' ',"1"},{'C',"10101101"},{'Q',"111011101"},{'D',"10110101"},{'E',"1110111"},
 {'T',"1101101"},{'S',"1101111"},{'K',"101111101"},{'H',"101010101"},{'L',"11010111"},
 {'O',"10101011"},{'1',"10111101"},{'2',"11101101"},{'3',"11111111"},{'a',"1011"},
 {'b',"1011111"},{'c',"101111"},{'d',"101101"},{'e',"11"},{'f',"111101"}};
struct Psk {
    double rate,offset,baud,time=0;
    std::complex<double> oscillator=std::polar(1.,.71),carrierStep;
    unsigned generated=0;
    std::string bits;
    size_t bit=0;
    float previous=1,current=1;
    Psk(double r,double f,double b,const std::string& message="CQ CQ DE TEST HELLO 123 abcdef K "):rate(r),offset(f),baud(b),carrierStep(std::polar(1.,2*3.141592653589793*f/r)){
        bits.assign(80,'0');
        for(int repeat=0;repeat<32;++repeat)for(char c:message)bits+=codes.at(c)+"00";
        bits.append(80,'0');current=-1;
    }
    rtty::IQ sample(){
        double position=time*baud;size_t wanted=size_t(position);
        while(bit<wanted){++bit;previous=current;if(bit<bits.size()&&bits[bit]=='0')current=-current;}
        float envelope=previous==current?current:(previous+current)*.5f+(previous-current)*.5f*float(std::cos(3.141592653589793*(position-wanted)));
        oscillator*=carrierStep;if((++generated&4095)==0)oscillator/=std::abs(oscillator);time+=1/rate;
        return {float(envelope*oscillator.real()),float(envelope*oscillator.imag())};
    }
};
struct Fsk {
    double rate,offset,phase=0,time=0;
    std::vector<bool> bits;
    Fsk(double r,double f):rate(r),offset(f){
        const int message[]{31,14,23,4,14,23,4,9,1,4,16,1,5,16,4,27,23,19,1,31,4,15,8,2};
        bits.assign(60,true);
        for(int repeat=0;repeat<32;++repeat)for(int code:message){bits.insert(bits.end(),2,false);
            for(int bit=0;bit<5;++bit)bits.insert(bits.end(),2,(code&(1<<bit))!=0);bits.insert(bits.end(),3,true);}
    }
    rtty::IQ sample(){size_t bit=size_t(time*45.45*2);bool high=bit<bits.size()?bits[bit]:true;
        phase=std::remainder(phase+2*3.141592653589793*(offset+(high?100:-100))/rate,2*3.141592653589793);time+=1/rate;return std::polar(1.f,float(phase));}
};
}
