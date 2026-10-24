// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

internal sealed class InventoryReader
{
    readonly SettingsStore store;
    string lastStamp = "";
    internal InventoryReader(SettingsStore settings)
    {
        store = settings;
    }

    internal bool TryRefresh(List<Folder> savedFolders, bool force, out List<TrayIconInfo> current)
    {
        current = null;
        string stamp = Stamp(store.InventoryPath) + "/" + Stamp(store.AssignmentsPath);
        if (!force && stamp == lastStamp)
            return false;
        current = Read(savedFolders);
        foreach (var icon in current)
            icon.FolderId = store.ResolveFolder(icon, savedFolders);
        lastStamp = stamp;
        return true;
    }

    internal List<TrayIconInfo> Read(List<Folder> savedFolders)
    {
        var items = new List<TrayIconInfo>();
        if (!File.Exists(store.InventoryPath))
            return items;
        foreach (string line in File.ReadAllLines(store.InventoryPath, Encoding.UTF8))
        {
            var c = line.Split('\t');
            if (c.Length < 4)
                continue;
            int index;
            string id = c.Length >= 5 ? c[4] : int.TryParse(c[3], out index) && index >= 0 && index < savedFolders.Count ? savedFolders[index].Id : "";
            items.Add(new TrayIconInfo { AppName = c[0], ToolTip = c[1], AccessibleName = c[2], FolderId = id });
        }

        return items;
    }

    string Stamp(string path)
    {
        if (!File.Exists(path))
            return "missing";
        var info = new FileInfo(path);
        return info.LastWriteTimeUtc.Ticks + ":" + info.Length;
    }
}
