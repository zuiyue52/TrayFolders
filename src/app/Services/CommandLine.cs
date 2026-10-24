// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.IO;
using System.Text;

internal static class CommandLine
{
    internal static bool TryRun(string[] args, string directory, out int exitCode)
    {
        exitCode = 0;
        StartupOptions options;
        if (StartupOptions.TryParse(args, out options))
            return false;
        string resultPath = Path.Combine(directory, "command-result.txt");
        var transport = new NativeEngineTransport();
        bool connected = false;
        try
        {
            EngineCommand command;
            switch (args[0])
            {
                case "--check-files":
                    ReleaseFiles.EnsureReady(directory);
                    File.WriteAllText(resultPath, "PASS: 运行文件完整，原生组件可加载。 / Runtime files present; native library loads.\r\nNo Explorer hooks were installed.\r\n" + string.Join("\r\n", ReleaseFiles.RequiredNames), Encoding.UTF8);
                    return true;
                case "--probe":
                case "--prepare":
                    var report = new StringBuilder();
                    var prepared = new CompatibilityService(directory).Prepare(text =>
                    {
                        lock (report)
                            report.AppendLine(text);
                    }, System.Threading.CancellationToken.None, args[0] == "--probe");
                    report.AppendLine(prepared.Description);
                    File.WriteAllText(resultPath, report.ToString(), Encoding.UTF8);
                    exitCode = prepared.Code == 1 ? 0 : 1;
                    return true;
                case "--compat-status":
                    command = EngineCommand.CheckCompatibility;
                    break;
                case "--status":
                    command = EngineCommand.Query;
                    break;
                case "--stop":
                    command = EngineCommand.Stop;
                    break;
                case "--refresh":
                    command = EngineCommand.RefreshInventory;
                    break;
                default:
                    File.WriteAllText(resultPath, "未知参数。测试请运行 scripts/Test.ps1。", Encoding.UTF8);
                    exitCode = 2;
                    return true;
            }

            int result = transport.Send(command);
            connected = true;
            File.WriteAllText(resultPath, result.ToString(), Encoding.UTF8);
            exitCode = result >= 0 && result <= 2 ? 0 : 1;
        }
        catch (Exception error)
        {
            File.WriteAllText(resultPath, error.ToString(), Encoding.UTF8);
            exitCode = 1;
        }
        finally
        {
            if (connected)
                transport.Disconnect();
        }

        return true;
    }
}
