// Copyright (c) 2026 ReLLL and contributors. SPDX-License-Identifier: GPL-3.0-or-later
using SDRSharp.Radio;
namespace SDRSharp.CWNotebook;
internal unsafe sealed class DecoderBridge
{
    internal readonly List<Complex> Output=[];
    internal void EnqueueIq(string channel,Complex* input,int count,double rate,double center,CapturePlan? plan)
    {if(channel!="cw"||rate!=plan?.Rate)throw new Exception("Capture metadata mismatch");for(int i=0;i<count;i++)Output.Add(input[i]);}
}
internal static unsafe class Program
{
    static void Require(bool value,string message){if(!value)throw new Exception(message);}
    static (double Hz,double Gain) Tone(double rate,int outputRate,double width,double tone)
    {
        const long center=14000000;var plan=new CapturePlan(center+8000,center+8000,outputRate,width,"CW",false);
        using var channel=new CaptureChannel(plan,rate,center);var bridge=new DecoderBridge();
        var data=new Complex[8192];long sample=0;
        for(int block=0;block<Math.Ceiling(rate*.5/data.Length);block++)
        {
            for(int i=0;i<data.Length;i++,sample++){double phase=2*Math.PI*(8000+tone)*sample/rate;data[i].Real=(float)(.5*Math.Cos(phase));data[i].Imag=(float)(.5*Math.Sin(phase));}
            var copy=(Complex[])data.Clone();fixed(Complex* input=data)channel.Process(input,data.Length,rate,bridge);
            for(int i=0;i<data.Length;i++)Require(copy[i].Real==data[i].Real&&copy[i].Imag==data[i].Imag,"Host IQ buffer changed");
        }
        int skip=outputRate/5;Require(bridge.Output.Count>skip+100,"No native resampler output");
        double re=0,im=0,power=0;
        for(int i=skip+1;i<bridge.Output.Count;i++){var a=bridge.Output[i-1];var b=bridge.Output[i];re+=a.Real*b.Real+a.Imag*b.Imag;im+=a.Real*b.Imag-a.Imag*b.Real;power+=b.Real*b.Real+b.Imag*b.Imag;}
        return(Math.Atan2(im,re)*outputRate/(2*Math.PI),Math.Sqrt(power/(bridge.Output.Count-skip-1))/.5);
    }
    static void Main()
    {
        foreach(var rate in new[]{192000d,256000d,2048000d,3200000d})
        foreach(var format in new[]{(8000,400d,80d),(16000,3000d,1000d),(32000,12000d,4000d),(64000,24000d,8000d)})
        {
            var result=Tone(rate,format.Item1,format.Item2,format.Item3);
            Console.WriteLine($"input={rate} output={format.Item1} width={format.Item2} tone={result.Hz:F2} gain={result.Gain:F4}");
            Require(Math.Abs(result.Hz-format.Item3)<1,"Mixing direction/frequency mismatch");
            Require(result.Gain>.89&&result.Gain<1.12,"Passband gain exceeds one dB");
        }
        var rejected=Tone(192000,8000,400,2000);Require(rejected.Gain<.01,"CW out-of-filter carrier was not rejected");
        Console.WriteLine("Production SDR# CW DSP: sample rates, signs, passbands, rejection and unchanged host buffers passed");
    }
}
