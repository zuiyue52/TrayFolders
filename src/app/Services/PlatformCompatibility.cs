// SPDX-License-Identifier: GPL-3.0-only
using System;
using Microsoft.Win32;

internal static class PlatformCompatibility
{
    internal static bool HasConflictingMod()
    {
        foreach (string id in new[]
        {
            "system-tray-folders",
            "local@system-tray-folders-26200"
        }

        )
            using (var key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Windhawk\Engine\Mods\" + id))
                if (key != null && Convert.ToInt32(key.GetValue("Disabled", 1)) == 0)
                    return true;
        return false;
    }
}
