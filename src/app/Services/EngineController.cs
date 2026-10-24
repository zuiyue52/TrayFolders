// SPDX-License-Identifier: GPL-3.0-only
using System;

internal interface IEngineTransport
{
    int Send(EngineCommand command);
    uint ShellProcessId { get; }

    void Disconnect();
}

// Owns native connection state independently from the window.
internal sealed class EngineController : IDisposable
{
    readonly IEngineTransport transport;
    uint shellPid;
    bool connected;
    internal EngineController(IEngineTransport connection)
    {
        transport = connection;
    }

    internal bool IsEnabled { get; private set; }

    int Send(EngineCommand command)
    {
        connected = true;
        return transport.Send(command);
    }

    internal int Start()
    {
        int result = Send(EngineCommand.Start);
        IsEnabled = result == 1;
        if (IsEnabled)
            shellPid = transport.ShellProcessId;
        return result;
    }

    internal int ReloadSettings()
    {
        return IsEnabled ? Send(EngineCommand.ReloadSettings) : 0;
    }

    internal int RefreshInventory()
    {
        // Result 2 means the panel has not opened yet, not that grouping stopped.
        return IsEnabled ? Send(EngineCommand.RefreshInventory) : 0;
    }

    internal int Stop()
    {
        int result = Send(EngineCommand.Stop);
        if (result >= 0)
        {
            IsEnabled = false;
            Disconnect();
        }

        return result;
    }

    internal int CheckCompatibility()
    {
        return IsEnabled ? Send(EngineCommand.CheckCompatibility) : 0;
    }

    internal bool PrepareReconnect()
    {
        if (!IsEnabled)
            return false;
        uint current = transport.ShellProcessId;
        if (current == 0 || current == shellPid)
            return false;
        Disconnect();
        IsEnabled = false;
        return true;
    }

    void Disconnect()
    {
        if (!connected)
            return;
        transport.Disconnect();
        connected = false;
    }

    public void Dispose()
    {
        Disconnect();
    }

    internal static string ErrorText(int result)
    {
        switch (result)
        {
            case -1:
                return "未找到任务栏，请稍后重试。";
            case -2:
                return "已加载组件与适配数据不匹配，请重新检测；系统更新后可能需要重新登录。";
            case -13:
                return "托盘界面结构未通过校验，需要更新适配逻辑。";
            case -14:
                return "符号下载未完成，请检查网络后重新检测。";
            case -3:
                return "请先创建至少一个文件夹。";
            case -4:
                return "无法启用托盘分组。";
            case -5:
                return "恢复托盘未完成，请保持程序运行。";
            case -7:
                return "无法连接资源管理器，请使用当前用户权限运行。";
            case -8:
                return "资源管理器响应超时，请稍后重试。";
            default:
                return "操作失败（" + result + "）。";
        }
    }
}
