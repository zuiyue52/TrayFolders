// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Drawing;
using System.Windows.Forms;

internal sealed class StartupSettingsPanel : UserControl
{
    readonly IStartupPreferencesStore store;
    readonly CheckBox enabled = new CheckBox(), silent = new CheckBox();
    readonly Label state = new Label(), feedback = new Label();
    readonly Button save;
    bool filling, dirty, loaded;
    internal StartupSettingsPanel(IStartupPreferencesStore preferences)
    {
        SuspendLayout();
        AutoScaleMode = AutoScaleMode.Inherit;
        store = preferences;
        Dock = DockStyle.Fill;
        var layout = UiFactory.Table(1, 8);
        layout.Padding = new Padding(20, 14, 20, 14);
        UiFactory.Rows(layout, new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.Percent, 100));
        Controls.Add(new PageViewport(layout, 580, 360));
        var title = new HeadingLabel("启动设置", 1F);
        title.Padding = new Padding(0, 4, 0, 8);
        layout.Controls.Add(title, 0, 0);
        enabled.Text = "开机自启动（登录 Windows 后）";
        silent.Text = "自启动时静默运行";
        enabled.Name = "startupEnabled";
        silent.Name = "startupSilent";
        enabled.Dock = silent.Dock = DockStyle.Fill;
        enabled.AutoSize = silent.AutoSize = true;
        enabled.Padding = silent.Padding = new Padding(0, 6, 0, 6);
        layout.Controls.Add(enabled, 0, 1);
        layout.Controls.Add(silent, 0, 2);
        var help = UiFactory.Label("静默启动后只保留托盘图标，双击托盘图标可打开窗口。\n手动双击程序时仍会显示窗口。");
        help.ForeColor = Color.DimGray;
        layout.Controls.Add(help, 0, 3);
        state.Dock = feedback.Dock = DockStyle.Fill;
        state.AutoSize = feedback.AutoSize = true;
        state.Padding = feedback.Padding = new Padding(0, 8, 0, 8);
        state.TextAlign = feedback.TextAlign = ContentAlignment.MiddleLeft;
        state.Name = "startupState";
        feedback.Name = "startupFeedback";
        layout.Controls.Add(state, 0, 4);
        var actions = UiFactory.Flow();
        save = UiFactory.Button("保存启动设置", delegate
        {
            Save();
        }, 150);
        actions.Controls.Add(save);
        actions.Controls.Add(UiFactory.Button("刷新状态", delegate
        {
            RefreshState(true);
        }, 110));
        layout.Controls.Add(actions, 0, 5);
        layout.Controls.Add(feedback, 0, 6);
        enabled.CheckedChanged += Changed;
        silent.CheckedChanged += Changed;
        RefreshState(true);
        ResumeLayout(true);
    }

    void Changed(object sender, EventArgs e)
    {
        if (filling)
            return;
        dirty = true;
        silent.Enabled = loaded && enabled.Checked;
        save.Enabled = loaded;
        feedback.ForeColor = Color.DimGray;
        feedback.Text = "有未保存的更改。";
    }

    internal void RefreshState(bool discardEdits)
    {
        try
        {
            var actual = store.Read();
            loaded = true;
            state.ForeColor = Color.FromArgb(30, 90, 65);
            state.Text = actual.DisabledByWindows ? "当前状态：已被 Windows 禁用。勾选开机自启动并保存可重新开启。" : actual.Enabled ? "当前状态：已开启开机自启动 · " + (actual.Silent ? "静默运行" : "显示窗口") : "当前状态：已关闭开机自启动。";
            if (!actual.TargetsCurrentExecutable)
            {
                state.ForeColor = Color.DarkGoldenrod;
                state.Text += "\n启动项指向其他位置；勾选开机自启动并保存可更新到此程序。";
            }

            if (discardEdits || !dirty)
            {
                filling = true;
                enabled.Checked = actual.Enabled;
                silent.Checked = actual.Silent;
                filling = false;
                dirty = false;
                feedback.Text = "";
            }

            enabled.Enabled = true;
            silent.Enabled = enabled.Checked;
            // A save also repairs a startup entry pointing to an older copy.
            save.Enabled = dirty || !actual.TargetsCurrentExecutable;
        }
        catch (Exception error)
        {
            loaded = false;
            enabled.Enabled = silent.Enabled = save.Enabled = false;
            state.ForeColor = Color.Firebrick;
            state.Text = "无法读取启动设置：" + error.Message;
        }
    }

    void Save()
    {
        try
        {
            store.Save(enabled.Checked, silent.Checked);
            RefreshState(true);
            feedback.ForeColor = loaded ? Color.FromArgb(30, 90, 65) : Color.Firebrick;
            feedback.Text = loaded ? "启动设置已保存，下次登录 Windows 时生效。" : "已写入启动设置，但无法确认当前状态；请点击刷新状态重试。";
        }
        catch (Exception error)
        {
            RefreshState(false);
            feedback.ForeColor = Color.Firebrick;
            feedback.Text = "保存失败：" + error.Message;
        }
    }
}
