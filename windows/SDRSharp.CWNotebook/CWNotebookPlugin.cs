// Copyright (c) 2026 ReLLL and contributors. SPDX-License-Identifier: GPL-3.0-or-later
using SDRSharp.Common;
using SDRSharp.Radio;
namespace SDRSharp.CWNotebook;

public sealed class CWNotebookPlugin : ISharpPlugin, ICanLazyLoadGui, ISupportStatus, IExtendedNameProvider
{
    private ISharpControl? control;
    private DecoderBridge? bridge;
    private ControlPanel? panel;
    public string DisplayName => "Maverick Text Modes Decoder: CW / RTTY / JS8 / Olivia / PSK31 / PSK63";
    public string Category => "Digital";
    public string MenuItemName => DisplayName;
    public bool IsActive => bridge?.Enabled == true;
    public UserControl Gui { get { LoadGui(); return panel!; } }
    public void Initialize(ISharpControl value)
    {
        control=value;bridge=new DecoderBridge(value);
        value.RegisterStreamHook(bridge,ProcessorType.RawIQ);
    }
    public void LoadGui(){if(panel==null&&bridge!=null)panel=new ControlPanel(bridge);}
    public void Close()
    {
        if(bridge==null)return;
        control?.UnregisterStreamHook(bridge);panel?.Dispose();bridge.Dispose();bridge=null;
    }
}
internal sealed class ControlPanel : UserControl
{
    private readonly System.Windows.Forms.Timer timer=new(){Interval=1000};
    private readonly ToolTip tip=new();
    internal ControlPanel(DecoderBridge bridge)
    {
        AutoScaleMode=AutoScaleMode.Dpi;AutoSize=true;
        var flow=new FlowLayoutPanel{Dock=DockStyle.Fill,AutoSize=true,FlowDirection=FlowDirection.TopDown,WrapContents=false};
        var enabled=new CheckBox{Text="Enable Maverick Decoder",Checked=bridge.Enabled,AutoSize=true};
        var open=new Button{Text="Open Maverick Decoder",AutoSize=true};
        var status=new Label{AutoSize=true,MaximumSize=new Size(310,0)};
        enabled.CheckedChanged+=(_,_)=>bridge.Enabled=enabled.Checked;open.Click+=(_,_)=>bridge.ShowWindow();
        tip.SetToolTip(enabled,"Enable private CW capture and full-spectrum digital monitoring. Existing text and logs are retained while off.");
        tip.SetToolTip(open,"Show the current notebook. Closing its window hides it while reception continues.");
        flow.Controls.AddRange([enabled,open,status]);Controls.Add(flow);
        timer.Tick+=(_,_)=>{status.Text=bridge.Status;tip.SetToolTip(status,bridge.Diagnostics);};timer.Start();
    }
    protected override void Dispose(bool disposing){if(disposing){timer.Dispose();tip.Dispose();}base.Dispose(disposing);}
}
