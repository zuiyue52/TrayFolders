// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.Text;
using System.Text.RegularExpressions;

internal sealed class Folder
{
    internal string Id = Guid.NewGuid().ToString("N"), Name = "新文件夹", Glyph = "";
    internal List<string> Patterns = new List<string>();
    internal int IconCount;
    public override string ToString()
    {
        return Name + "  (" + IconCount + ")";
    }
}

internal sealed class TrayIconInfo
{
    internal string AppName, ToolTip, AccessibleName, FolderId;
    internal string DisplayName
    {
        get
        {
            return string.IsNullOrWhiteSpace(ToolTip) ? AppName : ToolTip;
        }
    }

    internal string PreferredName
    {
        get
        {
            return string.IsNullOrEmpty(AppName) || IsShell(AppName) ? ToolTip : AppName;
        }
    }

    internal string Pattern
    {
        get
        {
            return "*" + PreferredName + "*";
        }
    }

    internal static bool IsShell(string name)
    {
        return name.Equals("Windows 资源管理器", StringComparison.OrdinalIgnoreCase) || name.Equals("Windows Explorer", StringComparison.OrdinalIgnoreCase);
    }

    internal string AssignmentKey
    {
        get
        {
            string identity = !string.IsNullOrEmpty(AppName) && !IsShell(AppName) ? "A:" + AppName.ToLowerInvariant() : "T:" + (string.IsNullOrEmpty(ToolTip) ? AccessibleName : ToolTip).ToLowerInvariant();
            var b = new StringBuilder();
            foreach (char c in identity)
                b.Append(((int)c).ToString("x4"));
            return b.ToString();
        }
    }

    internal bool Matches(string pattern)
    {
        string regex = "^" + Regex.Escape(pattern).Replace("\\*", ".*").Replace("\\?", ".") + "$";
        return Regex.IsMatch(AppName ?? "", regex, RegexOptions.IgnoreCase) || Regex.IsMatch(ToolTip ?? "", regex, RegexOptions.IgnoreCase) || Regex.IsMatch(AccessibleName ?? "", regex, RegexOptions.IgnoreCase);
    }
}
