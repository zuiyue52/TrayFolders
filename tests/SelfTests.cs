// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

internal static class SelfTests
{
    [DllImport("user32.dll")]
    static extern bool PrintWindow(IntPtr window, IntPtr deviceContext, uint flags);
    static void Require(bool condition, string message)
    {
        if (!condition)
            throw new Exception(message);
    }

    internal static IEnumerable<Control> Descendants(Control root)
    {
        foreach (Control child in root.Controls)
        {
            yield return child;
            foreach (var nested in Descendants(child))
                yield return nested;
        }
    }

    static void Click(MainForm form, string text)
    {
        var button = Descendants(form).OfType<Button>().Single(x => x.Text == text);
        typeof(Button).GetMethod("OnClick", BindingFlags.Instance | BindingFlags.NonPublic).Invoke(button, new object[] { EventArgs.Empty });
    }

    internal static void Render(Form form, string path)
    {
        if (!form.Visible)
        {
            form.ShowInTaskbar = false;
            form.StartPosition = FormStartPosition.Manual;
            form.Location = new Point(-32000, -32000);
            form.Show();
        }

        form.PerformLayout();
        Application.DoEvents();
        RenderSurface(form, path);

        var title = Descendants(form).OfType<Label>().FirstOrDefault(x => x.Text == "托盘文件夹");
        if (title != null)
        {
            Require(title.Parent.ClientRectangle.Contains(title.Bounds), "Title extends outside header: " + title.Bounds + " parent " + title.Parent.ClientSize);
            Require(title.Height >= TextRenderer.MeasureText(title.Text, title.Font).Height, "Title text is clipped");
            Require(title.Width >= TextRenderer.MeasureText(title.Text, title.Font).Width, "Title wraps unexpectedly");
            Require(Math.Abs(title.Font.SizeInPoints / form.Font.SizeInPoints - 20F / 9F) < 0.05F, "Heading font failed to follow DPI");
        }
    }

    internal static void RenderSurface(Control surface, string path)
    {
        surface.PerformLayout();
        Application.DoEvents();
        using (var bitmap = new Bitmap(surface.Width, surface.Height))
        {
            if (!(surface is Form))
            {
                // A large content canvas extends beyond the host window. Its pages
                // are unscrolled, so DrawToBitmap can render the complete layout.
                surface.DrawToBitmap(bitmap, new Rectangle(Point.Empty, surface.Size));
                bitmap.Save(path, ImageFormat.Png);
                return;
            }
            // DrawToBitmap renders scrolled child HWNDs outside their parents.
            // PrintWindow respects native clipping and does not capture the desktop.
            using (var graphics = Graphics.FromImage(bitmap))
            {
                IntPtr dc = graphics.GetHdc();
                try { Require(PrintWindow(surface.Handle, dc, 2), "Could not render test window"); }
                finally { graphics.ReleaseHdc(dc); }
            }
            bitmap.Save(path, ImageFormat.Png);
        }

    }

    internal static void CheckButtons(Control parent)
    {
        foreach (var button in Descendants(parent).OfType<Button>())
        {
            Require(button.Width >= 65 && button.Height >= 28, "Button too small: " + button.Text);
            Size text = TextRenderer.MeasureText(button.Text, button.Font, Size.Empty, TextFormatFlags.SingleLine);
            Require(button.ClientSize.Width >= text.Width + 8 && button.ClientSize.Height >= text.Height + 6,
                "Button text clipped: " + button.Text + " button " + button.ClientSize + " text " + text);
            Require(button.Left >= 0 && button.Top >= 0 && button.Right <= button.Parent.ClientSize.Width && button.Bottom <= button.Parent.ClientSize.Height, "Clipped button: " + button.Text + " " + button.Bounds + " parent " + button.Parent.ClientSize);
        }
    }

    internal static int Run(string output)
    {
        Directory.CreateDirectory(output);
        try
        {
            EngineControllerTests.Run();
            CompatibilityTests.Run();
            ReleaseFilesTests.Run(output);
            StartupSettingsTests.Run();
            Require(typeof(MainForm).Assembly.GetType("SelfTests") == null, "Tests leaked into the release application");
            var store = new SettingsStore(Path.Combine(output, "fixture-" + Guid.NewGuid().ToString("N")));
            Directory.CreateDirectory(store.Root);
            var first = new Folder
            {
                Id = "first",
                Name = "others",
                Patterns = new List<string>
                {
                    "*微信*",
                    "*ChatGPT*"
                }
            };
            var second = new Folder
            {
                Id = "second",
                Name = "工具"
            };
            var folders = new List<Folder>
            {
                first,
                second
            };
            store.Save(folders, true, true);
            StartupTests.Run(store);
            var search = new TrayIconInfo
            {
                AppName = "Everything",
                ToolTip = "Everything",
                AccessibleName = "Everything",
                FolderId = ""
            };
            var chat = new TrayIconInfo
            {
                AppName = "Codex",
                ToolTip = "ChatGPT",
                AccessibleName = "ChatGPT",
                FolderId = "first"
            };
            store.Assign(new[] { search }, first.Id);
            store.Assign(new[] { chat }, second.Id);
            Require(store.ResolveFolder(chat, folders) == "second", "Selection must override wildcard rule");
            store.Assign(new[] { chat }, null);
            Require(store.ResolveFolder(chat, folders) == "", "Moving out must override wildcard rule");
            store.Assign(new[] { chat }, first.Id);
            store.Save(new List<Folder> { second, first }, true, true);
            Require(store.ResolveFolder(search, store.Load()) == "first", "Reordering must preserve assignment by ID");
            store.Save(folders, true, true);
            File.WriteAllText(store.InventoryPath, "Everything\tEverything\tEverything\t0\tfirst\nCodex\tChatGPT\tChatGPT\t0\tfirst\nWeixin\t微信\t微信\t0\tfirst\nSnipaste\tSnipaste\tSnipaste\t-1\t\n", Encoding.UTF8);
            Require(new InventoryReader(store).Read(new List<Folder> { second, first })[0].FolderId == "first", "Inventory must use stable folder ID");
            using (var form = new MainForm(store, false, true, new FakeStartupPreferencesStore()))
            {
                Render(form, Path.Combine(output, "window-default.png"));
                CheckButtons(Descendants(form).OfType<TabControl>().Single().TabPages[0]);
                // File-watcher refresh must not erase a name the user is typing.
                var name = Descendants(form).OfType<TextBox>().First(x => x.Text == "others");
                name.Text = "尚未保存的名称";
                store.Assign(new[] { search }, second.Id);
                form.RefreshInventory(false);
                Require(name.Text == "尚未保存的名称", "Refresh erased an unsaved edit");
                name.Text = "others";
                Require(Descendants(form).OfType<FolderListBox>().Single().Items.Cast<Folder>().Single(x => x.Id == "first").IconCount == 2, "Drag membership count did not refresh");
                Click(form, "＋ 新建");
                Require(store.Load().Count == 3, "New folder action did not persist");
                Click(form, "删除");
                Require(store.Load().Count == 2, "Delete action did not persist");
                form.Size = form.MinimumSize;
                Render(form, Path.Combine(output, "window-minimum.png"));
                CheckButtons(Descendants(form).OfType<TabControl>().Single().TabPages[0]);
                form.ClientSize = new Size(1040, 760);
                Descendants(form).OfType<TabControl>().Single().SelectedIndex = 1;
                Render(form, Path.Combine(output, "window-icons.png"));
                CheckButtons(Descendants(form).OfType<TabControl>().Single().TabPages[1]);
                var all = Descendants(Descendants(form).OfType<TabControl>().Single().TabPages[1]).OfType<ListView>().Single();
                var snip = all.Items.Cast<ListViewItem>().Single(x => x.Text == "Snipaste");
                snip.Selected = true;
                Click(form, "加入选中图标");
                Require(store.Assignment((TrayIconInfo)snip.Tag) == "first", "Add selected icons action failed");
                var tabs = Descendants(form).OfType<TabControl>().Single();
                tabs.SelectedIndex = 2;
                Render(form, Path.Combine(output, "window-settings.png"));
                CheckButtons(tabs.TabPages[2]);
                tabs.SelectedIndex = 3;
                Render(form, Path.Combine(output, "window-compatibility.png"));
                CheckButtons(tabs.TabPages[3]);
                tabs.SelectedIndex = 2;
                form.Size = form.MinimumSize;
                Render(form, Path.Combine(output, "window-settings-minimum.png"));
                CheckButtons(tabs.TabPages[2]);
                form.ClientSize = new Size(1040, 760);
            }

            using (var icon = AppAssets.LoadIcon())
            using (var picker = new IconPickerDialog(new InventoryReader(store).Read(folders), folders, "second", icon))
            {
                Render(picker, Path.Combine(output, "icon-picker.png"));
                CheckButtons(picker);
            }

            DisplayLayoutTests.Run(store, folders, output);
            using (var ico = Icon.ExtractAssociatedIcon(typeof(MainForm).Assembly.Location))
            {
                Require(ico != null, "EXE icon missing");
                using (var bitmap = ico.ToBitmap())
                    bitmap.Save(Path.Combine(output, "exe-icon.png"));
            }

            File.WriteAllText(Path.Combine(output, "result.txt"), "PASS: startup settings UI and isolated registry roundtrip, silent startup, settings, assignment precedence, stable IDs, folder actions, automatic refresh, unsaved edits, layouts, embedded icon.", Encoding.UTF8);
            return 0;
        }
        catch (Exception e)
        {
            File.WriteAllText(Path.Combine(output, "result.txt"), e.ToString(), Encoding.UTF8);
            return 1;
        }
    }
}
