// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Runtime.InteropServices;
using System.Text;

internal static class NativeMethods
{
    [DllImport(BuildInfo.NativeLibrary)]
    internal static extern int TF_Command(int command);
    [DllImport(BuildInfo.NativeLibrary)]
    internal static extern void TF_Disconnect();
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    internal static extern uint GetPrivateProfileString(string section, string key, string fallback, StringBuilder value, uint size, string path);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    internal static extern bool WritePrivateProfileString(string section, string key, string value, string path);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    internal static extern IntPtr FindWindow(string name, string title);
    [DllImport("user32.dll")]
    internal static extern uint GetWindowThreadProcessId(IntPtr window, out uint pid);
}
