// Copyright (c) 2026 ReLLL and contributors. SPDX-License-Identifier: GPL-3.0-or-later
using System.Runtime.InteropServices;
using SDRSharp.Radio;
namespace SDRSharp.CWNotebook;
internal sealed record CapturePlan(double Center,double Dial,int Rate,double Width,string Mode,bool AmAudio,double FilterWidth=0);

// Native SDR# DSP runs on private buffers. It never writes the host IQ buffer.
internal unsafe sealed class CaptureChannel : IDisposable
{
    internal readonly CapturePlan Plan;
    private readonly DownConverter converter;
    private readonly IQResampler resampler;
    private readonly ComplexFilter filter;
    private Complex* buffer;
    private Complex* output;
    private int capacity,outputCapacity;
    internal CaptureChannel(CapturePlan plan,double rate,long center)
    {
        Plan=plan;int ratio=1;
        while(rate/(ratio*2)>=plan.Rate*2&&ratio<4096)ratio*=2;
        converter=new DownConverter(rate,ratio){Frequency=plan.Center-center};
        if(converter.DecimationRatio!=ratio){converter.Dispose();throw new InvalidOperationException("Unsupported SDR# decimation ratio");}
        try
        {
            resampler=new IQResampler(rate/ratio,plan.Rate);
            if(Math.Abs(resampler.ActualSampleRate-plan.Rate)>.01)throw new InvalidOperationException("Unsupported SDR# output sample rate");
            filter=new ComplexFilter(FilterBuilder.MakeLowPassKernel(plan.Rate,255,plan.Width/2,WindowType.BlackmanHarris4));
        }
        catch{resampler?.Dispose();converter.Dispose();throw;}
    }
    internal void Process(Complex* input,int count,double rate,DecoderBridge bridge)
    {
        if(count>capacity){NativeMemory.Free(buffer);buffer=(Complex*)NativeMemory.Alloc(checked((nuint)count*(nuint)sizeof(Complex)));capacity=count;}
        Buffer.MemoryCopy(input,buffer,(long)capacity*sizeof(Complex),(long)count*sizeof(Complex));
        int converted=converter.Process(buffer,count);if(converted<=0||converted>count)return;
        int required=checked((int)Math.Ceiling(converted*(double)Plan.Rate/(rate/converter.DecimationRatio))+4096);
        if(required>outputCapacity){NativeMemory.Free(output);output=(Complex*)NativeMemory.Alloc(checked((nuint)required*(nuint)sizeof(Complex)));outputCapacity=required;}
        int produced=resampler.Process(buffer,output,converted);
        if(produced<=0||produced>outputCapacity)throw new InvalidOperationException("Invalid SDR# resampler length");
        filter.Process(output,produced);bridge.EnqueueIq("cw",output,produced,Plan.Rate,Plan.Center,Plan);
    }
    public void Dispose(){filter.Dispose();resampler.Dispose();converter.Dispose();NativeMemory.Free(buffer);NativeMemory.Free(output);buffer=output=null;}
}
