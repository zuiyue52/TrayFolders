// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Windows.Forms;
using Microsoft.Win32;

internal sealed class FakeStartupPreferencesStore : IStartupPreferencesStore
{
    internal StartupPreferences State = new StartupPreferences
    {
        Enabled = true,
        Silent = true
    };
    internal int Writes;
    internal bool FailWrite, FailRead;
    public StartupPreferences Read()
    {
        if (FailRead)
            throw new InvalidOperationException("test read error");
        return State;
    }

    public void Save(bool enabled, bool silent)
    {
        if (FailWrite)
            throw new InvalidOperationException("test write error");
        Writes++;
        State = new StartupPreferences
        {
            Enabled = enabled,
            Silent = silent
        };
    }
}

internal static class StartupSettingsTests
{
    static void Check(bool condition, string message)
    {
        if (!condition)
            throw new Exception(message);
    }

    static IEnumerable<Control> Descendants(Control root)
    {
        foreach (Control child in root.Controls)
        {
            yield return child;
            foreach (var nested in Descendants(child))
                yield return nested;
        }
    }

    static void Click(Button button)
    {
        Check(button.Enabled, "Attempt to click disabled button: " + button.Text);
        typeof(Button).GetMethod("OnClick", BindingFlags.Instance | BindingFlags.NonPublic).Invoke(button, new object[] { EventArgs.Empty });
    }

    internal static void Run()
    {
        VerifyRegistry();
        var store = new FakeStartupPreferencesStore();
        using (var panel = new StartupSettingsPanel(store))
        {
            var controls = Descendants(panel).ToList();
            var enabled = controls.OfType<CheckBox>().Single(x => x.Name == "startupEnabled");
            var silent = controls.OfType<CheckBox>().Single(x => x.Name == "startupSilent");
            var save = controls.OfType<Button>().Single(x => x.Text == "保存启动设置");
            var refresh = controls.OfType<Button>().Single(x => x.Text == "刷新状态");
            var feedback = controls.OfType<Label>().Single(x => x.Name == "startupFeedback");
            Check(enabled.Checked && silent.Checked && !save.Enabled && store.Writes == 0, "Opening settings changed startup state");
            enabled.Checked = false;
            Check(!silent.Enabled && silent.Checked && store.Writes == 0, "Toggle must wait for save and retain silent preference");
            panel.RefreshState(false);
            Check(!enabled.Checked && save.Enabled, "Automatic refresh discarded startup edits");
            Click(save);
            Check(!store.State.Enabled && store.State.Silent && store.Writes == 1 && !save.Enabled, "Disabling startup was not saved");
            enabled.Checked = true;
            silent.Checked = false;
            Click(save);
            Check(store.State.Enabled && !store.State.Silent && store.Writes == 2, "Non-silent startup was not saved");
            silent.Checked = true;
            Click(refresh);
            Check(!silent.Checked && !save.Enabled, "Explicit refresh did not load actual state");
            store.FailWrite = true;
            enabled.Checked = false;
            Click(save);
            Check(store.State.Enabled && save.Enabled && feedback.Text.StartsWith("保存失败"), "Failed save was shown as success");
            store.FailRead = true;
            Click(refresh);
            Check(!save.Enabled && !enabled.Enabled, "Unreadable startup state must not be overwritten");
            store.FailRead = false;
            store.FailWrite = false;
            Click(refresh);
            Check(enabled.Enabled && enabled.Checked, "Read retry did not recover");
        }
    }

    static void VerifyRegistry()
    {
        // All registry writes stay inside a unique non-startup test branch.
        string testRoot = @"Software\TrayFolders.Tests\" + Guid.NewGuid().ToString("N");
        string prefix = testRoot + @"\";
        string runPath = prefix + @"Software\Microsoft\Windows\CurrentVersion\Run";
        string approvalPath = prefix + @"Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run";
        var store = new RegistryStartupPreferencesStore(@"C:\测试目录\Tray Folders\TrayFolders.exe", prefix);
        try
        {
            Check(!store.Read().Enabled && store.Read().Silent, "Default startup settings incorrect");
            using (var key = Registry.CurrentUser.CreateSubKey(runPath))
                key.SetValue("UnrelatedApp", "keep me");
            store.Save(true, true);
            using (var key = Registry.CurrentUser.OpenSubKey(runPath))
                Check((string)key.GetValue("TrayFolders") == "\"C:\\测试目录\\Tray Folders\\TrayFolders.exe\" --silent", "Startup command was not correctly quoted");
            Check(store.Read().Enabled && store.Read().Silent && store.Read().TargetsCurrentExecutable, "Silent registration readback failed");
            store.Save(false, false);
            Check(!store.Read().Enabled && !store.Read().Silent, "Disabled startup lost its silent preference");
            store.Save(true, false);
            Check(store.Read().Enabled && !store.Read().Silent, "Visible registration readback failed");
            using (var key = Registry.CurrentUser.CreateSubKey(approvalPath))
            {
                key.SetValue("TrayFolders", new byte[] { 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
                key.SetValue("UnrelatedApp", new byte[] { 3, 0, 0, 0 });
            }

            Check(store.Read().DisabledByWindows && !store.Read().Enabled, "Windows-disabled startup was shown as enabled");
            store.Save(true, true);
            Check(store.Read().Enabled && !store.Read().DisabledByWindows, "Explicit enable did not reset own approval flag");
            using (var key = Registry.CurrentUser.OpenSubKey(approvalPath))
                Check(key.GetValue("UnrelatedApp") != null, "Unrelated app approval was modified");
            using (var key = Registry.CurrentUser.CreateSubKey(runPath))
                key.SetValue("TrayFolders", "\"C:\\old\\TrayFolders.exe\" --silent");
            Check(!store.Read().TargetsCurrentExecutable && store.Read().Silent, "Moved application path was not detected");
            store.Save(true, true);
            Check(store.Read().TargetsCurrentExecutable, "Startup path repair failed");
            store.Save(false, true);
            using (var key = Registry.CurrentUser.OpenSubKey(runPath))
                Check(key.GetValue("TrayFolders") == null && (string)key.GetValue("UnrelatedApp") == "keep me", "Disable modified unrelated startup entries");
        }
        finally
        {
            Registry.CurrentUser.DeleteSubKeyTree(testRoot, false);
        }
    }
}
