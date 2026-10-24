// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

internal sealed partial class MainForm : Form
{
    readonly FolderListBox folderList = new FolderListBox();
    readonly TextBox folderName = new TextBox(), glyph = new TextBox(), patterns = new TextBox();
    readonly CheckBox atEnd = new CheckBox(), showEmpty = new CheckBox();
    readonly Label message = new Label(), inventoryStatus = new Label();
    Button start, stop, addIcons, removeIcons;
    readonly ComboBox destination = new ComboBox();
    readonly ListView members = UiFactory.CreateIconList(false), allIcons = UiFactory.CreateIconList(true);
    readonly TabControl tabs = new TabControl();
    void InitializeLayout()
    {
        Font = new Font("Microsoft YaHei UI", 9F);
        AutoScaleMode = AutoScaleMode.Dpi;
        AutoScaleDimensions = new SizeF(96, 96);
        Text = "托盘文件夹";
        Icon = AppAssets.LoadIcon();
        ClientSize = new Size(1040, 760);
        MinimumSize = new Size(640, 420);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Color.FromArgb(246, 248, 251);
        var root = UiFactory.Table(1, 3);
        root.Padding = new Padding(18, 12, 18, 10);
        UiFactory.Rows(root, new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.Percent, 100), new RowStyle(SizeType.AutoSize));
        Controls.Add(root);
        var header = UiFactory.Table(2, 1);
        header.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        header.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        header.AutoSize = true;
        header.MinimumSize = new Size(0, 62);
        header.Controls.Add(new HeadingLabel("托盘文件夹", 20F / 9F), 0, 0);
        var headerActions = UiFactory.Flow();
        headerActions.FlowDirection = FlowDirection.RightToLeft;
        stop = UiFactory.Button("停用分组", delegate
        {
            StopEngine();
        }, 108);
        start = UiFactory.Button("启用分组", delegate
        {
            StartEngine();
        }, 108);
        headerActions.Controls.Add(stop);
        headerActions.Controls.Add(start);
        headerActions.Controls.Add(UiFactory.Button("刷新图标", delegate
        {
            RefreshNow();
        }, 108));
        header.Controls.Add(headerActions, 1, 0);
        root.Controls.Add(header, 0, 0);
        tabs.Dock = DockStyle.Fill;
        var settingsPage = new TabPage("文件夹管理");
        var iconsPage = new TabPage("当前图标");
        tabs.TabPages.Add(settingsPage);
        tabs.TabPages.Add(iconsPage);
        root.Controls.Add(tabs, 0, 1);
        message.Dock = DockStyle.Fill;
        message.TextAlign = ContentAlignment.MiddleLeft;
        message.ForeColor = Color.DimGray;
        message.AutoEllipsis = true;
        message.MinimumSize = new Size(0, 30);
        root.Controls.Add(message, 0, 2);
        var split = UiFactory.Table(2, 1);
        split.Padding = new Padding(10);
        split.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 208));
        split.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        settingsPage.Controls.Add(new PageViewport(split, 820, 550));
        split.SizeChanged += delegate
        {
            split.ColumnStyles[0].Width = Math.Max(DisplayLayout.Units(split, 208), Math.Min(DisplayLayout.Units(split, 280), split.ClientSize.Width * 0.22F));
        };
        var left = UiFactory.Table(1, 4);
        left.Padding = new Padding(0, 0, 10, 0);
        UiFactory.Rows(left, new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.Percent, 100), new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.AutoSize));
        left.Controls.Add(UiFactory.Label("文件夹"), 0, 0);
        folderList.Dock = DockStyle.Fill;
        folderList.IntegralHeight = false;
        left.Controls.Add(folderList, 0, 1);
        var folderActions = UiFactory.Table(2, 1);
        folderActions.AutoSize = true;
        folderActions.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
        folderActions.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
        folderActions.Controls.Add(UiFactory.Button("＋ 新建", delegate
        {
            NewFolder();
        }), 0, 0);
        folderActions.Controls.Add(UiFactory.Button("删除", delegate
        {
            DeleteFolder();
        }), 1, 0);
        left.Controls.Add(folderActions, 0, 2);
        var orderActions = UiFactory.Table(2, 1);
        orderActions.AutoSize = true;
        orderActions.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
        orderActions.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50));
        orderActions.Controls.Add(UiFactory.Button("上移", delegate
        {
            MoveFolder(-1);
        }), 0, 0);
        orderActions.Controls.Add(UiFactory.Button("下移", delegate
        {
            MoveFolder(1);
        }), 1, 0);
        left.Controls.Add(orderActions, 0, 3);
        split.Controls.Add(left, 0, 0);
        var editor = UiFactory.Table(1, 8);
        UiFactory.Rows(editor, new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.Percent, 65), new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.Percent, 35), new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.AutoSize));
        split.Controls.Add(editor, 1, 0);
        var metadata = UiFactory.Table(2, 2);
        metadata.AutoSize = true;
        metadata.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 68));
        metadata.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 32));
        UiFactory.Rows(metadata, new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.AutoSize));
        metadata.Controls.Add(UiFactory.Label("文件夹名称"), 0, 0);
        metadata.Controls.Add(UiFactory.Label("显示符号（可留空）"), 1, 0);
        folderName.Dock = glyph.Dock = DockStyle.Top;
        metadata.Controls.Add(folderName, 0, 1);
        metadata.Controls.Add(glyph, 1, 1);
        editor.Controls.Add(metadata, 0, 0);
        editor.Controls.Add(UiFactory.Label("文件夹中的图标（支持多选）"), 0, 1);
        editor.Controls.Add(members, 0, 2);
        var memberActions = UiFactory.Flow();
        addIcons = UiFactory.Button("＋ 添加图标…", delegate
        {
            ChooseIcons();
        }, 140);
        removeIcons = UiFactory.Button("移出选中图标", delegate
        {
            AssignIcons(UiFactory.SelectedIcons(members), null);
        }, 140);
        memberActions.Controls.Add(addIcons);
        memberActions.Controls.Add(removeIcons);
        editor.Controls.Add(memberActions, 0, 3);
        editor.Controls.Add(UiFactory.Label("手动匹配规则：每行一条，例如 *微信*；* 可匹配任意文字"), 0, 4);
        patterns.Multiline = true;
        patterns.AcceptsReturn = true;
        patterns.ScrollBars = ScrollBars.Vertical;
        patterns.Dock = DockStyle.Fill;
        editor.Controls.Add(patterns, 0, 5);
        var options = UiFactory.Flow();
        atEnd.Text = "文件夹放在最后";
        showEmpty.Text = "显示空文件夹";
        atEnd.AutoSize = showEmpty.AutoSize = true;
        atEnd.Checked = store.Number("General", "FoldersAtEnd", 1) != 0;
        showEmpty.Checked = store.Number("General", "ShowEmptyFolders", 1) != 0;
        options.Controls.Add(atEnd);
        options.Controls.Add(showEmpty);
        editor.Controls.Add(options, 0, 6);
        var saveActions = UiFactory.Flow();
        saveActions.FlowDirection = FlowDirection.RightToLeft;
        saveActions.Controls.Add(UiFactory.Button("保存并应用", delegate
        {
            if (SaveAndApply())
                Tell("设置已保存。");
        }, 140));
        editor.Controls.Add(saveActions, 0, 7);
        var inventoryLayout = UiFactory.Table(1, 3);
        inventoryLayout.Padding = new Padding(10);
        UiFactory.Rows(inventoryLayout, new RowStyle(SizeType.AutoSize), new RowStyle(SizeType.Percent, 100), new RowStyle(SizeType.AutoSize));
        iconsPage.Controls.Add(new PageViewport(inventoryLayout, 740, 300));
        inventoryStatus.Dock = DockStyle.Fill;
        inventoryStatus.AutoSize = true;
        inventoryStatus.Padding = new Padding(0, 5, 0, 5);
        inventoryStatus.TextAlign = ContentAlignment.MiddleLeft;
        inventoryLayout.Controls.Add(inventoryStatus, 0, 0);
        inventoryLayout.Controls.Add(allIcons, 0, 1);
        var inventoryActions = UiFactory.Flow();
        inventoryActions.Controls.Add(new Label { Text = "加入文件夹：", AutoSize = true, Margin = new Padding(3, 10, 3, 0) });
        destination.DropDownStyle = ComboBoxStyle.DropDownList;
        destination.Width = 190;
        destination.Margin = new Padding(3, 7, 8, 0);
        inventoryActions.Controls.Add(destination);
        inventoryActions.Controls.Add(UiFactory.Button("加入选中图标", delegate
        {
            var f = destination.SelectedItem as Folder;
            if (f != null)
                AssignIcons(UiFactory.SelectedIcons(allIcons), f.Id);
        }, 140));
        inventoryActions.Controls.Add(UiFactory.Button("复制选中规则", delegate
        {
            var rows = UiFactory.SelectedIcons(allIcons);
            if (rows.Count > 0)
                Clipboard.SetText(string.Join(Environment.NewLine, rows.Select(x => x.Pattern).Distinct()));
        }, 140));
        inventoryLayout.Controls.Add(inventoryActions, 0, 2);
    }
}
