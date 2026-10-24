// SPDX-License-Identifier: GPL-3.0-only
using System;

internal static class CompatibilityTests
{
    static void Check(bool ok, string message)
    {
        if (!ok)
            throw new Exception(message);
    }

    internal static void Run()
    {
        Check(CompatibilityService.Quote(@"C:\中文 目录\TrayFolders.exe") == "\"C:\\中文 目录\\TrayFolders.exe\"", "Path quoting lost Unicode or spaces");
        Check(CompatibilityService.Quote("a\"b") == "\"a\\\"b\"", "Quoted argument not escaped");
        Check(CompatibilityService.Quote("C:\\cache\\") == "\"C:\\cache\\\\\"", "Trailing slash not escaped");
        var now = DateTime.UtcNow;
        var retry = new StartupRetry(true);
        retry.RecordResult(-14, now);
        Check(!retry.IsDue(now.AddSeconds(29)) && retry.IsDue(now.AddSeconds(30)), "Network retry should back off");
        retry.RecordResult(-14, now.AddSeconds(30));
        retry.RecordResult(-14, now.AddSeconds(60));
        Check(!retry.IsDue(now.AddHours(1)), "Failed symbol download retried without limit");
    }
}
