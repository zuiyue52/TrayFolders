// SPDX-License-Identifier: GPL-3.0-only
using System;

// Login may precede creation of Shell_TrayWnd and loading of Taskbar.View.dll.
internal sealed class StartupRetry
{
    const int MaxAttempts = 20;
    int attempts;
    DateTime nextAttempt = DateTime.MinValue;
    bool pending;
    internal StartupRetry(bool enabled)
    {
        pending = enabled;
    }

    internal bool IsDue(DateTime now)
    {
        return pending && now >= nextAttempt;
    }

    internal void Cancel()
    {
        pending = false;
    }

    internal void RecordResult(int result, DateTime now)
    {
        attempts++;
        bool transient = result == -1 || result == -2 || result == -8 || (result == -14 && attempts < 3);
        pending = transient && attempts < MaxAttempts;
        nextAttempt = now.AddSeconds(result == -14 ? 30 : 3);
    }
}
