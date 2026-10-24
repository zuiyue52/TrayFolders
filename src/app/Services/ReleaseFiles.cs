// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;

// Checks files before symbol downloads or shell commands. DllMain only initializes
// the module handle; hooks are installed by TF_Command, never by this check.
internal static class ReleaseFiles
{
    internal static readonly string[] RequiredNames = {
        "TrayFolders.exe", "TrayFolders.exe.config", BuildInfo.NativeLibrary, "TrayFolders.Symbols.exe"
    };
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern IntPtr LoadLibraryEx(string path, IntPtr file, uint flags);
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi, ExactSpelling = true)]
    static extern IntPtr GetProcAddress(IntPtr module, string name);
    [DllImport("kernel32.dll")]
    static extern bool FreeLibrary(IntPtr module);

    internal static string[] Missing(string directory)
    {
        return RequiredNames.Where(name => !File.Exists(Path.Combine(directory, name))).ToArray();
    }
    internal static void EnsureReady(string directory)
    {
        string[] missing = Missing(directory);
        if (missing.Length != 0)
            throw new FileNotFoundException("便携包文件不完整 / Incomplete portable package: " +
                string.Join(", ", missing) + "\r\n目录 / Folder: " + directory +
                "\r\n请完整解压便携包，并保持以上文件在同一目录。不要只复制 EXE。 / Extract the complete portable package; keep all runtime files together.");
        // Restrict dependency lookup to this DLL's directory and Windows System32.
        IntPtr module = LoadLibraryEx(Path.GetFullPath(Path.Combine(directory, BuildInfo.NativeLibrary)), IntPtr.Zero, 0x100 | 0x800);
        if (module == IntPtr.Zero)
        {
            int error = Marshal.GetLastWin32Error();
            throw new InvalidOperationException("原生组件文件存在，但无法加载 / Native library exists but could not load: " +
                BuildInfo.NativeLibrary + "\r\nWin32 " + error + ": " + new Win32Exception(error).Message +
                "\r\n请使用 Windows 11 x64 的完整便携包；检查文件是否损坏、依赖是否缺失或文件是否被安全软件拦截。 / Check architecture, damaged files, dependencies and security-software history.");
        }
        try
        {
            if (GetProcAddress(module, "TF_Command") == IntPtr.Zero || GetProcAddress(module, "TF_Disconnect") == IntPtr.Zero)
                throw new InvalidOperationException("原生组件接口不匹配，请重新解压同版本完整便携包。 / Native exports do not match; extract the complete matching release.");
        }
        finally { FreeLibrary(module); }
    }
    internal static Exception LoadFailure(Exception error)
    {
        return new InvalidOperationException("无法加载 " + BuildInfo.NativeLibrary +
            "。请完整解压同版本便携包，并运行 TrayFolders.exe --check-files 查看具体原因。 / Cannot load the native component; extract the full matching package and run --check-files.\r\n" + error.Message, error);
    }
}
