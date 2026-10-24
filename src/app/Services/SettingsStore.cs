// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;

internal sealed class SettingsStore
{
    internal readonly string Root, SettingsPath, AssignmentsPath, InventoryPath;
    internal SettingsStore(string root)
    {
        Root = root;
        SettingsPath = Path.Combine(root, "settings.ini");
        AssignmentsPath = Path.Combine(root, "assignments.ini");
        InventoryPath = Path.Combine(root, "icons.tsv");
    }

    internal string Get(string section, string key, string fallback)
    {
        var b = new StringBuilder(4096);
        NativeMethods.GetPrivateProfileString(section, key, fallback, b, 4096, SettingsPath);
        return b.ToString();
    }

    internal int Number(string section, string key, int fallback)
    {
        int n;
        return int.TryParse(Get(section, key, ""), out n) ? n : fallback;
    }

    internal List<Folder> Load()
    {
        var list = new List<Folder>();
        int count = Math.Max(0, Math.Min(100, Number("General", "FolderCount", 0)));
        for (int i = 0; i < count; i++)
        {
            string section = "Folder" + i;
            var folder = new Folder
            {
                Id = Get(section, "Id", i.ToString()),
                Name = Get(section, "Name", "Folder"),
                Glyph = Get(section, "Glyph", "")
            };
            int n = Math.Max(0, Math.Min(500, Number(section, "Count", 0)));
            for (int j = 0; j < n; j++)
            {
                string p = Get(section, "Icon" + j, "").Trim();
                if (p.Length > 0)
                    folder.Patterns.Add(p);
            }

            list.Add(folder);
        }

        return list;
    }

    static string Clean(string value)
    {
        return (value ?? "").Replace("\r", "").Replace("\n", "").Trim();
    }

    internal void Save(List<Folder> folders, bool end, bool empty)
    {
        Directory.CreateDirectory(Root);
        var b = new StringBuilder();
        b.AppendLine("[General]");
        b.AppendLine("FolderCount=" + folders.Count);
        b.AppendLine("FoldersAtEnd=" + (end ? "1" : "0"));
        b.AppendLine("ShowEmptyFolders=" + (empty ? "1" : "0"));
        for (int i = 0; i < folders.Count; i++)
        {
            var f = folders[i];
            b.AppendLine();
            b.AppendLine("[Folder" + i + "]");
            b.AppendLine("Id=" + Clean(f.Id));
            b.AppendLine("Name=" + Clean(f.Name));
            b.AppendLine("Glyph=" + Clean(f.Glyph));
            b.AppendLine("Count=" + f.Patterns.Count);
            for (int j = 0; j < f.Patterns.Count; j++)
                b.AppendLine("Icon" + j + "=" + Clean(f.Patterns[j]));
        }

        string temp = SettingsPath + ".tmp";
        File.WriteAllText(temp, b.ToString(), Encoding.Unicode);
        if (File.Exists(SettingsPath))
            File.Replace(temp, SettingsPath, SettingsPath + ".bak");
        else
            File.Move(temp, SettingsPath);
    }

    internal void Assign(IEnumerable<TrayIconInfo> icons, string folderId)
    {
        if (!File.Exists(AssignmentsPath))
            File.WriteAllText(AssignmentsPath, "[Moves]\r\n", Encoding.Unicode);
        foreach (var icon in icons)
            if (!NativeMethods.WritePrivateProfileString("Moves", icon.AssignmentKey, folderId ?? "@root", AssignmentsPath))
                throw new Win32Exception();
    }

    internal string Assignment(TrayIconInfo icon)
    {
        var b = new StringBuilder(512);
        NativeMethods.GetPrivateProfileString("Moves", icon.AssignmentKey, "", b, 512, AssignmentsPath);
        return b.ToString();
    }

    internal string ResolveFolder(TrayIconInfo icon, List<Folder> folders)
    {
        string move = Assignment(icon);
        if (move == "@root")
            return "";
        if (move.Length > 0 && folders.Any(f => f.Id == move))
            return move;
        foreach (var f in folders)
            if (f.Patterns.Any(icon.Matches))
                return f.Id;
        return "";
    }
}
