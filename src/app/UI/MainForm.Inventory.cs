// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;

internal sealed partial class MainForm : Form
{
    void RefreshNow()
    {
        try
        {
            RequestInventory(true);
            RefreshInventory(true);
        }
        catch (Exception e)
        {
            Tell("刷新失败：" + e.Message);
        }
    }

    internal void RefreshInventory(bool force)
    {
        try
        {
            List<TrayIconInfo> current;
            if (!inventoryReader.TryRefresh(appliedFolders, force, out current))
                return;
            inventory = current;
            foreach (var f in folders)
                f.IconCount = inventory.Count(x => x.FolderId == f.Id);
            filling = true;
            folderList.RefreshNames();
            filling = false;
            RefreshMemberList();
            FillList(allIcons, inventory, true);
            inventoryStatus.Text = inventory.Count == 0 ? "请展开一次隐藏图标面板，再点击右上角“刷新图标”。" : "已读取 " + inventory.Count + " 个隐藏托盘图标 · 列表随拖动自动更新";
        }
        catch (IOException)
        {
        }
        catch (UnauthorizedAccessException)
        {
            Tell("无法读取图标名单，请检查程序目录的访问权限。");
        }
    }

    void RefreshMemberList()
    {
        FillList(members, selected == null ? new List<TrayIconInfo>() : inventory.Where(x => x.FolderId == selected.Id).ToList(), false);
    }

    void FillList(ListView view, IEnumerable<TrayIconInfo> values, bool folderColumn)
    {
        var selection = new HashSet<string>(UiFactory.SelectedIcons(view).Select(x => x.AssignmentKey));
        view.BeginUpdate();
        view.Items.Clear();
        foreach (var icon in values)
        {
            var item = new ListViewItem(icon.AppName)
            {
                Tag = icon
            };
            item.SubItems.Add(icon.ToolTip);
            if (folderColumn)
            {
                var f = folders.FirstOrDefault(x => x.Id == icon.FolderId);
                item.SubItems.Add(f == null ? "未分组" : f.Name);
                item.SubItems.Add(icon.Pattern);
            }

            view.Items.Add(item);
            item.Selected = selection.Contains(icon.AssignmentKey);
        }

        view.EndUpdate();
    }
}
