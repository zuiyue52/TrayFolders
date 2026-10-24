// SPDX-License-Identifier: GPL-3.0-only
using System;
using Microsoft.Win32;

internal sealed class StartupPreferences
{
    internal bool Enabled;
    internal bool Silent = true;
    internal bool DisabledByWindows;
    internal bool TargetsCurrentExecutable = true;
}

internal interface IStartupPreferencesStore
{
    StartupPreferences Read();
    void Save(bool enabled, bool silent);
}

internal sealed class RegistryStartupPreferencesStore : IStartupPreferencesStore
{
    const string ValueName = "TrayFolders";
    readonly string executable;
    readonly string runPath, approvalPath, preferencesPath;
    // The optional prefix keeps integration tests away from real startup entries.
    internal RegistryStartupPreferencesStore(string executablePath, string registryPrefix = "")
    {
        executable = executablePath;
        runPath = registryPrefix + @"Software\Microsoft\Windows\CurrentVersion\Run";
        approvalPath = registryPrefix + @"Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run";
        preferencesPath = registryPrefix + @"Software\TrayFolders";
    }

    internal string Command(bool silent)
    {
        return "\"" + executable + "\"" + (silent ? " --silent" : "");
    }

    public StartupPreferences Read()
    {
        string command;
        using (var key = Registry.CurrentUser.OpenSubKey(runPath))
            command = key == null ? null : key.GetValue(ValueName) as string;
        bool registered = !string.IsNullOrWhiteSpace(command);
        bool silent;
        using (var key = Registry.CurrentUser.OpenSubKey(preferencesPath))
            silent = key == null || !object.Equals(key.GetValue("SilentAutostart"), 0);
        if (registered)
            silent = command.TrimEnd().EndsWith(" --silent", StringComparison.OrdinalIgnoreCase);
        bool disabled = false;
        using (var key = Registry.CurrentUser.OpenSubKey(approvalPath))
        {
            var approval = key == null ? null : key.GetValue(ValueName) as byte[];
            disabled = approval != null && approval.Length >= 4 && (BitConverter.ToUInt32(approval, 0) == 3 || BitConverter.ToUInt32(approval, 0) == 7);
        }

        return new StartupPreferences
        {
            Enabled = registered && !disabled,
            Silent = silent,
            DisabledByWindows = registered && disabled,
            TargetsCurrentExecutable = !registered || string.Equals(command.Trim(), Command(silent), StringComparison.OrdinalIgnoreCase)
        };
    }

    public void Save(bool enabled, bool silent)
    {
        using (var key = Registry.CurrentUser.CreateSubKey(preferencesPath))
            key.SetValue("SilentAutostart", silent ? 1 : 0, RegistryValueKind.DWord);
        if (enabled)
        {
            using (var key = Registry.CurrentUser.CreateSubKey(runPath))
                key.SetValue(ValueName, Command(silent), RegistryValueKind.String);
            // Clear only our own disabled flag when the user explicitly enables startup.
            using (var key = Registry.CurrentUser.OpenSubKey(approvalPath, true))
                if (key != null)
                    key.DeleteValue(ValueName, false);
        }
        else
        {
            using (var key = Registry.CurrentUser.OpenSubKey(runPath, true))
                if (key != null)
                    key.DeleteValue(ValueName, false);
        }
    }
}
