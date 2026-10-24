// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Drawing;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

internal sealed partial class MainForm : Form
{
    readonly TextBox compatibilityDetails = new TextBox();
    CancellationTokenSource preparation;
    bool preparing, runtimePending;
    int validationTicks;
    Button detect;
    void InitializeCompatibility()
    {
        var page = new TabPage("系统适配");
        var layout = UiFactory.Table(1, 3);
        layout.Padding = new Padding(14);
        UiFactory.Rows(layout, new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.Percent, 100), new RowStyle(SizeType.AutoSize));
        layout.Controls.Add(UiFactory.Label("启用分组时自动识别系统组件。首次适配需联网下载微软符号文件，之后可使用本地缓存。\n检测未通过时会保留原始托盘。"), 0, 0);
        compatibilityDetails.Multiline = true;
        compatibilityDetails.ReadOnly = true;
        compatibilityDetails.ScrollBars = ScrollBars.Vertical;
        compatibilityDetails.Dock = DockStyle.Fill;
        compatibilityDetails.BackColor = Color.White;
        compatibilityDetails.Text = "尚未检测。启用分组时会自动开始。";
        layout.Controls.Add(compatibilityDetails, 0, 1);
        var actions = UiFactory.Flow();
        detect = UiFactory.Button("重新检测并启用", delegate
        {
            if (StopEngine())
                StartEngine();
        }, 170);
        actions.Controls.Add(detect);
        layout.Controls.Add(actions, 0, 2);
        page.Controls.Add(new PageViewport(layout, 580, 300));
        tabs.TabPages.Add(page);
    }

    void ReportCompatibility(string text)
    {
        if (IsDisposed || Disposing)
            return;
        if (InvokeRequired)
        {
            if (IsHandleCreated)
                try
                {
                    BeginInvoke(new Action<string>(ReportCompatibility), text);
                }
                catch (InvalidOperationException)
                {
                }

            return;
        }

        if (compatibilityDetails.TextLength > 20000)
            compatibilityDetails.Clear();
        compatibilityDetails.AppendText(text + Environment.NewLine);
    }

    async void BeginStartEngine(bool automatic)
    {
        if (preparing || engine.IsEnabled)
            return;
        int result = -10;
        var cancellation = new CancellationTokenSource();
        preparation = cancellation;
        preparing = true;
        UpdateState();
        compatibilityDetails.Clear();
        try
        {
            ReleaseFiles.EnsureReady(store.Root);
            if (PlatformCompatibility.HasConflictingMod())
            {
                Tell("请先停用 Windhawk 中的托盘分组模组。");
                result = -11;
                return;
            }

            if (!SaveAndApply())
            {
                result = -12;
                return;
            }

            Tell("正在准备系统适配，可在“系统适配”页查看进度。");
            var service = new CompatibilityService(store.Root);
            var prepared = await Task.Run(() => service.Prepare(ReportCompatibility, cancellation.Token));
            cancellation.Token.ThrowIfCancellationRequested();
            if (IsDisposed || Disposing)
                return;
            ReportCompatibility(prepared.Description);
            result = prepared.Code;
            if (result != 1)
            {
                Tell("系统适配未完成，请查看“系统适配”页。");
                return;
            }

            result = engine.Start();
            if (result < 0)
            {
                Tell(EngineController.ErrorText(result));
                ReportCompatibility(EngineController.ErrorText(result));
            }
            else
            {
                runtimePending = true;
                validationTicks = 0;
                Tell("适配已加载，请展开一次隐藏图标完成界面验证。");
                RequestInventory(false);
            }
        }
        catch (OperationCanceledException)
        {
        }
        catch (Exception e)
        {
            result = -6;
            if (!IsDisposed)
            {
                Tell("启用失败：" + e.Message);
                ReportCompatibility(e.Message);
            }
        }
        finally
        {
            if (automatic && !cancellation.IsCancellationRequested)
                startup.RecordResult(result, DateTime.UtcNow);
            if (preparation == cancellation)
                preparation = null;
            cancellation.Dispose();
            preparing = false;
            if (!IsDisposed && !Disposing)
                UpdateState();
        }
    }

    void CheckRuntimeCompatibility()
    {
        if (!runtimePending || !engine.IsEnabled || ++validationTicks < 4)
            return;
        validationTicks = 0;
        try
        {
            int result = engine.CheckCompatibility();
            if (result == 1)
            {
                runtimePending = false;
                ReportCompatibility("界面结构与图标读取验证通过，分组已启用。");
                Tell("分组已启用。");
            }
            else if (result == -13)
            {
                runtimePending = false;
                string text = "当前托盘界面结构未通过校验，需要更新适配逻辑。";
                if (StopEngine())
                    text += "已恢复原始托盘。";
                Tell(text);
                ReportCompatibility(text);
            }
        }
        catch (Exception e)
        {
            ReportCompatibility("无法查询界面验证结果：" + e.Message);
            runtimePending = false;
        }
    }
}
