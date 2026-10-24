// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading;

internal sealed class CompatibilityResult
{
    internal int Code;
    internal string Description;
}

internal sealed class CompatibilityService
{
    readonly string directory;
    internal CompatibilityService(string applicationDirectory)
    {
        directory = applicationDirectory;
    }

    internal static string FindTrayModule()
    {
        uint pid;
        NativeMethods.GetWindowThreadProcessId(NativeMethods.FindWindow("Shell_TrayWnd", null), out pid);
        if (pid == 0)
            return null;
        string taskbar = null, tray = null;
        string windows = Path.GetFullPath(Environment.GetFolderPath(Environment.SpecialFolder.Windows)) + Path.DirectorySeparatorChar;
        using (var process = Process.GetProcessById((int)pid))
            foreach (ProcessModule module in process.Modules)
            {
                if (!string.Equals(module.ModuleName, "Taskbar.View.dll", StringComparison.OrdinalIgnoreCase) && !string.Equals(module.ModuleName, "SystemTray.dll", StringComparison.OrdinalIgnoreCase))
                    continue;
                string path = Path.GetFullPath(module.FileName);
                if (!path.StartsWith(windows, StringComparison.OrdinalIgnoreCase))
                    continue;
                if (string.Equals(module.ModuleName, "SystemTray.dll", StringComparison.OrdinalIgnoreCase))
                    tray = path;
                else
                    taskbar = path;
            }

        return tray ?? taskbar;
    }

    internal static string Quote(string value)
    {
        var result = new StringBuilder("\"");
        int slashes = 0;
        foreach (char c in value)
        {
            if (c == '\\')
            {
                slashes++;
                continue;
            }

            if (c == '"')
            {
                result.Append('\\', slashes * 2 + 1);
                result.Append(c);
            }
            else
            {
                result.Append('\\', slashes);
                result.Append(c);
            }

            slashes = 0;
        }

        result.Append('\\', slashes * 2);
        result.Append('"');
        return result.ToString();
    }

    internal CompatibilityResult Prepare(Action<string> report, CancellationToken cancellation, bool offline = false)
    {
        try
        {
            cancellation.ThrowIfCancellationRequested();
            report("正在识别当前资源管理器使用的托盘组件…");
            string module = FindTrayModule();
            if (module == null)
                return new CompatibilityResult
                {
                    Code = -2,
                    Description = "托盘组件尚未加载，将稍后重试。"
                };
            report("组件：" + Path.GetFileName(module) + " · " + FileVersionInfo.GetVersionInfo(module).FileVersion);
            string helper = Path.Combine(directory, "TrayFolders.Symbols.exe");
            if (!File.Exists(helper))
                return new CompatibilityResult
                {
                    Code = -10,
                    Description = "缺少适配组件，请保留完整程序目录。"
                };
            string cache = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), @"TrayFolders\Symbols");
            var start = new ProcessStartInfo(helper)
            {
                Arguments = Quote(module) + " " + Quote(cache) + " " + Quote(Path.Combine(directory, "compatibility.bin")) + " --parent=" + Process.GetCurrentProcess().Id + (offline ? " --offline" : ""),
                WorkingDirectory = directory,
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true
            };
            var details = new StringBuilder();
            using (var process = new Process
            {
                StartInfo = start
            }

            )
            {
                process.OutputDataReceived += delegate (object sender, DataReceivedEventArgs e)
                {
                    if (e.Data == null)
                        return;
                    string text;
                    switch (e.Data)
                    {
                        case "phase=detect":
                            text = "正在读取组件身份与调试符号标识…";
                            break;
                        case "phase=download":
                            text = "正在从微软符号服务器下载匹配文件，首次可能需要几分钟…";
                            break;
                        case "phase=resolve":
                            text = "正在解析内部函数和接口…";
                            break;
                        case "phase=verify":
                            text = "正在校验函数签名、地址范围和组件身份…";
                            break;
                        case "phase=cache":
                            text = "已找到通过校验的本地适配缓存，无需下载。";
                            break;
                        default:
                            text = e.Data.StartsWith("ready=adapter-1") ? "已选择 Taskbar.View 托盘适配方案。" : e.Data.StartsWith("ready=adapter-2") ? "已选择 SystemTray 托盘适配方案。" : e.Data;
                            break;
                    }

                    report(text);
                };
                process.ErrorDataReceived += delegate (object sender, DataReceivedEventArgs e)
                {
                    if (e.Data != null)
                        lock (details)
                        {
                            if (details.Length < 16000)
                                details.AppendLine(e.Data);
                        }
                };
                process.Start();
                process.BeginOutputReadLine();
                process.BeginErrorReadLine();
                var elapsed = Stopwatch.StartNew();
                while (!process.WaitForExit(100))
                {
                    if (cancellation.IsCancellationRequested || elapsed.Elapsed > TimeSpan.FromMinutes(4))
                    {
                        try
                        {
                            process.Kill();
                            process.WaitForExit();
                        }
                        catch (InvalidOperationException)
                        {
                        }

                        cancellation.ThrowIfCancellationRequested();
                        return new CompatibilityResult
                        {
                            Code = -14,
                            Description = "适配超时，请检查网络后重新检测。"
                        };
                    }
                }

                process.WaitForExit(); // Drain redirected output callbacks.
                cancellation.ThrowIfCancellationRequested();
                if (process.ExitCode != 0)
                {
                    string error;
                    lock (details)
                        error = details.ToString().Trim();
                    bool network = error.IndexOf("download", StringComparison.OrdinalIgnoreCase) >= 0 || error.IndexOf("HTTP", StringComparison.OrdinalIgnoreCase) >= 0;
                    return new CompatibilityResult
                    {
                        Code = network ? -14 : -10,
                        Description = "适配未通过，未启用分组。\r\n" + error
                    };
                }
            }

            return new CompatibilityResult
            {
                Code = 1,
                Description = "组件与函数地址校验通过；展开隐藏图标后继续验证界面结构。"
            };
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception e)
        {
            return new CompatibilityResult
            {
                Code = -10,
                Description = "无法完成适配：" + e.Message
            };
        }
    }
}
