// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

internal static class UiFactory
{
    internal static TableLayoutPanel Table(int columns, int rows)
    {
        var panel = new TableLayoutPanel
        {
            ColumnCount = columns,
            RowCount = rows,
            Dock = DockStyle.Fill,
            Margin = Padding.Empty
        };
        // A default AutoSize row can keep a child's initial 100px height even
        // when its parent is shorter, clipping the centered title at the tabs.
        if (rows == 1)
            panel.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        if (columns == 1)
            panel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        return panel;
    }

    internal static void Rows(TableLayoutPanel panel, params RowStyle[] styles)
    {
        foreach (var style in styles)
            panel.RowStyles.Add(style);
    }

    internal static FlowLayoutPanel Flow()
    {
        return new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            WrapContents = false,
            AutoSize = true,
            AutoSizeMode = AutoSizeMode.GrowAndShrink,
            Margin = Padding.Empty,
            Padding = new Padding(0, 3, 0, 0)
        };
    }

    internal static Label Label(string text)
    {
        return new Label
        {
            Text = text,
            Dock = DockStyle.Fill,
            AutoSize = true,
            Padding = new Padding(0, 4, 0, 4),
            TextAlign = ContentAlignment.MiddleLeft,
            AutoEllipsis = true
        };
    }

    internal static Button Button(string text, EventHandler action, int width = 90)
    {
        var b = new Button
        {
            Text = text,
            Width = width,
            Height = 34,
            AutoSize = true,
            AutoSizeMode = AutoSizeMode.GrowAndShrink,
            MinimumSize = new Size(width, 34),
            Anchor = AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Top,
            Margin = new Padding(3, 3, 3, 3),
            FlatStyle = FlatStyle.System
        };
        b.Click += action;
        return b;
    }

    internal static ListView CreateIconList(bool folderColumn)
    {
        var view = new ListView
        {
            Dock = DockStyle.Fill,
            View = View.Details,
            FullRowSelect = true,
            HideSelection = false,
            MultiSelect = true,
            ShowItemToolTips = true
        };
        view.Columns.Add("程序名称", 200);
        view.Columns.Add("悬停名称", 340);
        if (folderColumn)
        {
            view.Columns.Add("所在文件夹", 140);
            view.Columns.Add("可填写的规则", 190);
        }

        view.ClientSizeChanged += delegate { FitIconColumns(view); };
        view.FontChanged += delegate { FitIconColumns(view); };
        view.HandleCreated += delegate { FitIconColumns(view); };
        return view;
    }

    internal static void FitIconColumns(ListView view)
    {
        int count = view.Columns.Count;
        if (count < 2 || view.ClientSize.Width <= 0)
            return;
        int[] weights = count == 2 ? new[] { 34, 66 } : count == 3 ? new[] { 28, 48, 24 } : new[] { 22, 38, 16, 24 };
        int available = Math.Max(1, view.ClientSize.Width - SystemInformation.VerticalScrollBarWidth - 4);
        int used = 0;
        for (int i = 0; i < count; i++)
        {
            int minimum = TextRenderer.MeasureText(view.Columns[i].Text, view.Font).Width + DisplayLayout.Units(view, 24);
            int width = Math.Max(minimum, i == count - 1 ? available - used : available * weights[i] / 100);
            view.Columns[i].Width = width;
            used += width;
        }
    }

    internal static List<TrayIconInfo> SelectedIcons(ListView view)
    {
        return view.SelectedItems.Cast<ListViewItem>().Select(x => (TrayIconInfo)x.Tag).ToList();
    }
}
