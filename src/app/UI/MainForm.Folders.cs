// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;

internal sealed partial class MainForm : Form
{
    void CommitSelected()
    {
        if (selected == null)
            return;
        selected.Name = string.IsNullOrWhiteSpace(folderName.Text) ? "新文件夹" : folderName.Text.Trim();
        selected.Glyph = glyph.Text.Trim();
        selected.Patterns = patterns.Lines.Select(x => x.Trim()).Where(x => x.Length > 0).Distinct(StringComparer.OrdinalIgnoreCase).Take(500).ToList();
    }

    void LoadSelected()
    {
        bool has = selected != null;
        folderName.Enabled = glyph.Enabled = patterns.Enabled = addIcons.Enabled = removeIcons.Enabled = has;
        folderName.Text = has ? selected.Name : "";
        glyph.Text = has ? selected.Glyph : "";
        patterns.Lines = has ? selected.Patterns.ToArray() : new string[0];
        RefreshMemberList();
    }

    void RefreshFolders(Folder focus)
    {
        filling = true;
        folderList.Items.Clear();
        foreach (var f in folders)
            folderList.Items.Add(f);
        folderList.SelectedItem = focus;
        selected = focus;
        destination.Items.Clear();
        foreach (var f in folders)
            destination.Items.Add(f);
        if (folders.Count > 0)
            destination.SelectedItem = focus ?? folders[0];
        filling = false;
        LoadSelected();
    }

    void NewFolder()
    {
        CommitSelected();
        var f = new Folder();
        int n = 2;
        while (folders.Any(x => x.Name == f.Name))
            f.Name = "新文件夹 " + n++;
        folders.Add(f);
        RefreshFolders(f);
        if (SaveAndApply())
        {
            folderName.Focus();
            folderName.SelectAll();
            Tell("已创建文件夹。可选择图标加入，或填写匹配规则。");
        }
    }

    void DeleteFolder()
    {
        if (selected == null)
            return;
        folders.Remove(selected);
        RefreshFolders(folders.FirstOrDefault());
        if (SaveAndApply())
            Tell("文件夹已删除，图标会重新按其余规则归类。");
    }

    void MoveFolder(int offset)
    {
        if (selected == null)
            return;
        CommitSelected();
        int old = folders.IndexOf(selected), next = old + offset;
        if (next < 0 || next >= folders.Count)
            return;
        folders.RemoveAt(old);
        folders.Insert(next, selected);
        RefreshFolders(selected);
        SaveAndApply();
    }

    bool SaveAndApply()
    {
        try
        {
            CommitSelected();
            store.Save(folders, atEnd.Checked, showEmpty.Checked);
            appliedFolders = store.Load();
            filling = true;
            folderList.RefreshNames();
            filling = false;
            if (engine.IsEnabled)
            {
                int result = engine.ReloadSettings();
                if (result < 0)
                {
                    Tell(EngineController.ErrorText(result));
                    return false;
                }

                RequestInventory(false);
            }

            RefreshInventory(true);
            return true;
        }
        catch (Exception e)
        {
            filling = false;
            Tell("保存失败：" + e.Message);
            return false;
        }
    }

    void ChooseIcons()
    {
        if (selected == null)
            return;
        RequestInventory(false);
        RefreshInventory(true);
        using (var picker = new IconPickerDialog(inventory, folders, selected.Id, Icon))
        {
            if (picker.ShowDialog(this) == DialogResult.OK)
                AssignIcons(picker.Chosen, selected.Id);
        }
    }

    void AssignIcons(List<TrayIconInfo> icons, string folderId)
    {
        if (icons.Count == 0)
        {
            Tell("请先选择要添加或移出的图标。");
            return;
        }

        if (!SaveAndApply())
            return;
        try
        {
            store.Assign(icons, folderId);
            if (engine.IsEnabled)
            {
                int result = engine.ReloadSettings();
                if (result < 0)
                {
                    Tell(EngineController.ErrorText(result));
                    return;
                }

                RequestInventory(false);
            }

            RefreshInventory(true);
            Tell(folderId == null ? "已移出选中图标。" : "已加入选中图标，拖动与选择的归属均已保存。");
        }
        catch (Exception e)
        {
            Tell("分配图标失败：" + e.Message);
        }
    }
}
