// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

internal sealed class IconPickerDialog : Form
{
    readonly ListView icons = UiFactory.CreateIconList(true);
    internal List<TrayIconInfo> Chosen
    {
        get
        {
            return icons.CheckedItems.Cast<ListViewItem>().Select(x => (TrayIconInfo)x.Tag).ToList();
        }
    }

    internal IconPickerDialog(List<TrayIconInfo> current, List<Folder> folders, string folderId, Icon icon)
    {
        SuspendLayout();
        AutoScaleMode = AutoScaleMode.Dpi;
        AutoScaleDimensions = new SizeF(96, 96);
        Text = "选择要加入的图标";
        Icon = icon;
        Font = new Font("Microsoft YaHei UI", 9F);
        ClientSize = new Size(800, 480);
        MinimumSize = new Size(540, 340);
        StartPosition = FormStartPosition.CenterParent;
        ShowInTaskbar = false;
        MinimizeBox = false;
        MaximizeBox = false;
        var layout = UiFactory.Table(1, 3);
        layout.Padding = new Padding(14);
        UiFactory.Rows(layout, new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.Percent, 100), new RowStyle(SizeType.AutoSize));
        Controls.Add(new PageViewport(layout, 500, 280));
        layout.Controls.Add(UiFactory.Label(current.Count == 0 ? "暂未读取到图标，请展开隐藏图标面板后在主窗口刷新。" : "勾选要加入的图标，可一次选择多个。"), 0, 0);
        icons.CheckBoxes = true;
        icons.Columns.RemoveAt(3);
        layout.Controls.Add(icons, 0, 1);
        foreach (var item in current)
        {
            if (item.FolderId == folderId)
                continue;
            var row = new ListViewItem(item.AppName)
            {
                Tag = item
            };
            row.SubItems.Add(item.ToolTip);
            var f = folders.FirstOrDefault(x => x.Id == item.FolderId);
            row.SubItems.Add(f == null ? "未分组" : f.Name);
            icons.Items.Add(row);
        }

        var actions = UiFactory.Flow();
        actions.FlowDirection = FlowDirection.RightToLeft;
        var ok = UiFactory.Button("加入文件夹", delegate
        {
            if (Chosen.Count > 0)
            {
                DialogResult = DialogResult.OK;
                Close();
            }
        }, 130);
        var cancel = UiFactory.Button("取消", delegate
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }, 90);
        actions.Controls.Add(ok);
        actions.Controls.Add(cancel);
        AcceptButton = ok;
        CancelButton = cancel;
        layout.Controls.Add(actions, 0, 2);
        DisplayLayout.FitToScreen(this, new Size(540, 340));
        ResumeLayout(true);
    }
}
