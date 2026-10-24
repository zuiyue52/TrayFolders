// SPDX-License-Identifier: GPL-3.0-only
using System.Drawing;
using System.Reflection;

internal static class AppAssets
{
    internal static Icon LoadIcon()
    {
        using (var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("TrayFolders.AppIcon"))
        using (var icon = new Icon(stream, 32, 32))
            return (Icon)icon.Clone();
    }
}
