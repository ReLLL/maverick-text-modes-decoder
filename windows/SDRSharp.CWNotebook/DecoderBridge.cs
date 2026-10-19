// Copyright (c) 2026 ReLLL and contributors. SPDX-License-Identifier: GPL-3.0-or-later
using System.Buffers;
using System.Buffers.Binary;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Text.Json;
using SDRSharp.Common;
using SDRSharp.Radio;
namespace SDRSharp.CWNotebook;

internal unsafe sealed class DecoderBridge : IIQProcessor,IStreamProcessor,IDisposable
{
    private readonly ISharpControl control;
    private readonly object captureLock=new();
    private readonly record struct Packet(byte[] Data,int Length,bool Pooled=false);
    private BlockingCollection<Packet> outgoing=new(32);
    private CancellationTokenSource stopping=new();
    private readonly System.Windows.Forms.Timer timer=new(){Interval=250};
    private Process? worker;
    private Thread? writer,reader;
    private CaptureChannel? cwChannel;
    private CapturePlan? desired;
    private volatile bool disposed,running,cwRequested,digitalRequested;
    private bool enabled=true;
    private double sampleRate,configuredRate,lastBlockTime;
    private long configuredCenter,skipped,blocks,queuedBytes,cwSequence,rawSequence,actionId,captureEpoch,processEpoch;
    private int restarting,resetNeeded=1,openRequested;
    private sealed record PendingAction(JsonElement Value);
    private PendingAction? action;
    private string error="Start the radio to receive.";
    public bool Enabled {get=>Volatile.Read(ref enabled);set{Volatile.Write(ref enabled,value);Invalidate();Utils.SaveSetting("maverickDecoder.enabled",value);SendMetadata();}}
    public double SampleRate {set{if(Volatile.Read(ref sampleRate)==value)return;Volatile.Write(ref sampleRate,value);Invalidate();SendMetadata();}}
    internal string Status=>!string.IsNullOrEmpty(error)?error:!running?"Decoder stopped. Open to retry.":!Enabled?"Decoder off":control.IsPlaying?"Receiving":"Radio stopped";
    internal string Diagnostics=>$"{Interlocked.Read(ref blocks)} capture blocks; {Interlocked.Read(ref skipped)} gaps. Closing the window keeps enabled reception active.";
    internal DecoderBridge(ISharpControl control)
    {
        this.control=control;enabled=Utils.GetBooleanSetting("maverickDecoder.enabled",true);
        control.PropertyChanged+=ControlChanged;
        timer.Tick+=(_,_)=>{if(disposed)return;EnsureWorker(Interlocked.Exchange(ref openRequested,0)!=0);ApplyAction();SendMetadata();};timer.Start();
    }
    private void Invalidate(){Interlocked.Increment(ref captureEpoch);Interlocked.Exchange(ref resetNeeded,1);}
    private void ControlChanged(object? sender,System.ComponentModel.PropertyChangedEventArgs e)
    {
        if(disposed)return;
        if(e.PropertyName is "IsPlaying" or "CenterFrequency")Invalidate();
        if(e.PropertyName is "IsPlaying" or "CenterFrequency" or "Frequency" or "FilterBandwidth" or "DetectorType")SendMetadata();
    }
    private void EnsureWorker(bool force=false)
    {
        if((!Enabled&&!force)||disposed||running||Interlocked.Exchange(ref restarting,1)!=0)return;
        try
        {
            if(worker!=null)return;
            var folder=Path.GetDirectoryName(typeof(CWNotebookPlugin).Assembly.Location)!;
            var binary=Path.Combine(folder,"MaverickDecoder","cw_sdrsharp.exe");
            if(!File.Exists(binary)){error="Decoder files missing. Reinstall the complete Maverick package.";return;}
            var root=Environment.GetEnvironmentVariable("MAVERICK_DECODER_DATA_ROOT")??Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Maverick Decoder","SDRSharp");
            var start=new ProcessStartInfo(binary){UseShellExecute=false,RedirectStandardInput=true,RedirectStandardOutput=true,RedirectStandardError=true,CreateNoWindow=true,WorkingDirectory=Path.GetDirectoryName(binary)!};
            start.ArgumentList.Add("--data-root");start.ArgumentList.Add(root);start.ArgumentList.Add("--parent");start.ArgumentList.Add(Environment.ProcessId.ToString());
            var process=System.Diagnostics.Process.Start(start)??throw new IOException("Decoder did not start");worker=process;
            process.PriorityClass=ProcessPriorityClass.BelowNormal;
            error="";running=!process.HasExited;var queue=outgoing;var token=stopping.Token;
            process.EnableRaisingEvents=true;process.Exited+=(_,_)=>Disconnected(process,token,"Decoder stopped. Open to retry; CW logs are retained.");
            process.ErrorDataReceived+=(_,e)=>{if(ReferenceEquals(worker,process)&&!string.IsNullOrWhiteSpace(e.Data))error=e.Data[..Math.Min(e.Data.Length,300)];};process.BeginErrorReadLine();
            writer=new Thread(()=>WriteLoop(process,queue,token)){IsBackground=true,Name="Maverick capture IPC"};
            reader=new Thread(()=>ReadLoop(process,token)){IsBackground=true,Name="Maverick control IPC"};writer.Start();reader.Start();
            SendMetadata();
        }
        catch(Exception e)when(e is IOException or System.ComponentModel.Win32Exception or InvalidOperationException){error="Decoder could not start: "+e.Message;running=false;}
        finally{Interlocked.Exchange(ref restarting,0);}
    }
    internal void ShowWindow()
    {
        if(disposed)return;
        if(worker!=null&&!running&&Interlocked.CompareExchange(ref restarting,1,0)==0)
        {
            _=Task.Run(()=>{try{lock(captureLock){if(!disposed)StopSession();}}finally{Interlocked.Exchange(ref restarting,0);Interlocked.Exchange(ref openRequested,1);}});return;
        }
        EnsureWorker(true);Enqueue(new{kind="show"});
    }
    private void SendMetadata()
    {
        if(!running)return;
        Enqueue(new{kind="metadata",center=control.CenterFrequency,frequency=control.Frequency,
            sample_rate=Volatile.Read(ref sampleRate)>0?Volatile.Read(ref sampleRate):control.InputSampleRate,
            bandwidth=control.FilterBandwidth,mode=control.DetectorType.ToString(),playing=Enabled&&control.IsPlaying,epoch=Interlocked.Read(ref captureEpoch)});
    }
    private void ApplyAction()
    {
        var next=Interlocked.Exchange(ref action,null);if(next==null)return;
        var value=next.Value;
        if(value.TryGetProperty("frequency",out var frequency))
        {
            var dial=frequency.GetDouble();var rate=Volatile.Read(ref sampleRate);
            if(!double.IsFinite(dial)||dial-100<control.CenterFrequency-rate/2||dial+3200>control.CenterFrequency+rate/2)return;
            control.Frequency=(long)Math.Round(dial);
        }
        if(value.TryGetProperty("mode",out var mode)&&Enum.TryParse<DetectorType>(mode.GetString(),out var detector))control.DetectorType=detector;
        if(value.TryGetProperty("bandwidth",out var width))control.FilterBandwidth=width.GetInt32();
    }
    public void Process(Complex* input,int count)
    {
        if(disposed||!Enabled||!running||(!cwRequested&&!digitalRequested)||input==null||count<=0||count>1024*1024)return;
        if(!Monitor.TryEnter(captureLock)){Interlocked.Exchange(ref resetNeeded,1);Interlocked.Increment(ref skipped);return;}
        try
        {
            var rate=Volatile.Read(ref sampleRate);var center=control.CenterFrequency;
            if(disposed||!control.IsPlaying||!double.IsFinite(rate)||rate<8000)return;
            if(Interlocked.Read(ref queuedBytes)>Budget(rate)){Interlocked.Exchange(ref resetNeeded,1);Interlocked.Increment(ref skipped);return;}
            var now=Stopwatch.GetTimestamp()/(double)Stopwatch.Frequency;
            var epoch=Interlocked.Read(ref captureEpoch);
            bool gap=Interlocked.Exchange(ref resetNeeded,0)!=0||epoch!=processEpoch||lastBlockTime>0&&now-lastBlockTime>Math.Max(1,count/rate*4);
            processEpoch=epoch;
            lastBlockTime=now;
            if(gap||configuredRate!=rate||configuredCenter!=center)
            {
                cwChannel?.Dispose();cwChannel=null;configuredRate=rate;configuredCenter=center;
                SendMetadata();Enqueue(new{kind="reset",channel="all"});
            }
            var plan=Volatile.Read(ref desired);
            bool useCw=cwRequested&&plan!=null&&plan.Dial==control.Frequency&&plan.Mode==control.DetectorType.ToString()&&plan.FilterWidth==control.FilterBandwidth&&plan.Center-plan.Width/2>=center-rate/2&&plan.Center+plan.Width/2<=center+rate/2;
            if(!useCw){cwChannel?.Dispose();cwChannel=null;}
            else if(cwChannel?.Plan!=plan)
            {
                cwChannel?.Dispose();cwChannel=new CaptureChannel(plan!,rate,center);Enqueue(new{kind="reset",channel="cw"});
            }
            int step=Math.Clamp((int)(rate*.016),128,32768);
            for(int offset=0;offset<count;offset+=step)
            {
                int length=Math.Min(step,count-offset);
                if(digitalRequested&&rate<=4096000)EnqueueIq("digital",input+offset,length,rate,center,null);
                if(useCw)cwChannel?.Process(input+offset,length,rate,this);
            }
            Interlocked.Increment(ref blocks);
        }
        catch(Exception e)when(e is ArgumentException or InvalidOperationException or OverflowException)
        {error="Capture reset: "+e.Message;cwChannel?.Dispose();cwChannel=null;configuredRate=0;Interlocked.Exchange(ref resetNeeded,1);Interlocked.Increment(ref skipped);}
        finally{Monitor.Exit(captureLock);}
    }
    private long Budget(double rate)=>(long)Math.Clamp((rate+64000)*8*.20,32768,8*1024*1024);
    internal void EnqueueIq(string channel,Complex* input,int count,double rate,double center,CapturePlan? plan)
    {
        for(int offset=0;offset<count;offset+=32768)
        {
            int n=Math.Min(32768,count-offset);
            var header=JsonSerializer.SerializeToUtf8Bytes(new{kind="iq",channel,samples=n,sample_rate=rate,center,
                frequency=plan?.Dial??control.Frequency,mode=plan?.Mode??control.DetectorType.ToString(),bandwidth=plan?.Width??control.FilterBandwidth,
                am_audio=plan?.AmAudio??false,filter_width=plan?.FilterWidth??control.FilterBandwidth,epoch=processEpoch,sequence=channel=="cw"?cwSequence++:rawSequence++});
            int payload=n*sizeof(Complex),length=4+header.Length+payload;
            var bytes=ArrayPool<byte>.Shared.Rent(length);BinaryPrimitives.WriteInt32LittleEndian(bytes,header.Length);header.CopyTo(bytes,4);
            fixed(byte* target=&bytes[4+header.Length])Buffer.MemoryCopy(input+offset,target,payload,payload);
            if(!TryQueue(new(bytes,length,true))){ArrayPool<byte>.Shared.Return(bytes);Interlocked.Exchange(ref resetNeeded,1);Interlocked.Increment(ref skipped);}
        }
    }
    private void Enqueue<T>(T value)
    {
        if(disposed||stopping.IsCancellationRequested)return;
        var header=JsonSerializer.SerializeToUtf8Bytes(value);var bytes=new byte[header.Length+4];
        BinaryPrimitives.WriteInt32LittleEndian(bytes,header.Length);header.CopyTo(bytes,4);
        if(!TryQueue(new(bytes,bytes.Length)))Interlocked.Exchange(ref resetNeeded,1);
    }
    private bool TryQueue(Packet packet)
    {
        var total=Interlocked.Add(ref queuedBytes,packet.Length);
        if(packet.Pooled&&total>Budget(Volatile.Read(ref sampleRate))){Interlocked.Add(ref queuedBytes,-packet.Length);return false;}
        try{if(outgoing.TryAdd(packet))return true;}
        catch(Exception e)when(e is InvalidOperationException or ObjectDisposedException){}
        Interlocked.Add(ref queuedBytes,-packet.Length);return false;
    }
    private void WriteLoop(Process process,BlockingCollection<Packet> queue,CancellationToken token)
    {
        try{foreach(var p in queue.GetConsumingEnumerable(token)){try{process.StandardInput.BaseStream.Write(p.Data,0,p.Length);process.StandardInput.BaseStream.Flush();}finally{Interlocked.Add(ref queuedBytes,-p.Length);if(p.Pooled)ArrayPool<byte>.Shared.Return(p.Data);}}}
        catch(Exception e)when(e is IOException or OperationCanceledException or ObjectDisposedException or InvalidOperationException){Disconnected(process,token,"Decoder disconnected. Open to retry.");}
    }
    private void ReadLoop(Process process,CancellationToken token)
    {
        try
        {
            var input=process.StandardOutput.BaseStream;var prefix=new byte[4];
            while(!token.IsCancellationRequested)
            {
                input.ReadExactly(prefix);int n=BinaryPrimitives.ReadInt32LittleEndian(prefix);if(n<=0||n>65536)throw new IOException("Invalid decoder response");
                var bytes=new byte[n];input.ReadExactly(bytes);using var doc=JsonDocument.Parse(bytes);var root=doc.RootElement;
                if(root.GetProperty("kind").GetString()!="plan")continue;
                cwRequested=root.GetProperty("cw").GetBoolean();digitalRequested=root.GetProperty("digital").GetBoolean();
                var p=root.GetProperty("cw_plan");
                var plan=new CapturePlan(p.GetProperty("center").GetDouble(),p.GetProperty("dial").GetDouble(),p.GetProperty("rate").GetInt32(),p.GetProperty("width").GetDouble(),p.GetProperty("mode").GetString()!,p.GetProperty("am_audio").GetBoolean(),p.GetProperty("filter_width").GetDouble());
                if(plan.Rate is not (8000 or 16000 or 32000 or 64000)||!double.IsFinite(plan.Center)||!double.IsFinite(plan.Width)||plan.Width<50||plan.Width>24000)throw new IOException("Invalid CW capture plan");
                Volatile.Write(ref desired,plan);
                if(root.TryGetProperty("action_id",out var id)&&id.GetInt64()!=Interlocked.Read(ref actionId))
                {Interlocked.Exchange(ref actionId,id.GetInt64());Interlocked.Exchange(ref action,new PendingAction(root.GetProperty("action").Clone()));}
            }
        }
        catch(Exception e)when(e is IOException or JsonException or KeyNotFoundException or InvalidOperationException or ObjectDisposedException)
        {Disconnected(process,token,"Decoder controls disconnected. Open to retry.");}
    }
    private void Disconnected(Process process,CancellationToken token,string reason)
    {if(disposed||token.IsCancellationRequested||!ReferenceEquals(worker,process))return;running=false;Interlocked.Exchange(ref resetNeeded,1);error=reason;}
    private void StopSession()
    {
        var process=worker;worker=null;running=false;stopping.Cancel();outgoing.CompleteAdding();
        try{if(process is {HasExited:false}){process.StandardInput.Close();if(!process.WaitForExit(8000)){process.Kill(true);process.WaitForExit(2000);}}}catch(InvalidOperationException){}
        cwChannel?.Dispose();cwChannel=null;configuredRate=0;desired=null;cwRequested=digitalRequested=false;action=null;actionId=0;Invalidate();
        writer?.Join(1000);reader?.Join(1000);process?.Dispose();stopping.Dispose();
        while(outgoing.TryTake(out var p)){Interlocked.Add(ref queuedBytes,-p.Length);if(p.Pooled)ArrayPool<byte>.Shared.Return(p.Data);}
        outgoing.Dispose();writer=reader=null;stopping=new();outgoing=new(32);Interlocked.Exchange(ref queuedBytes,0);
    }
    public void Dispose(){disposed=true;control.PropertyChanged-=ControlChanged;timer.Dispose();lock(captureLock){StopSession();stopping.Dispose();outgoing.Dispose();}}
}
