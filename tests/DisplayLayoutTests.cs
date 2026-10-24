// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows.Forms;

internal static class DisplayLayoutTests
{
    [StructLayout(LayoutKind.Sequential)]
    struct Rect { internal int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")]
    static extern IntPtr SendMessage(IntPtr window, int message, IntPtr wParam, ref Rect lParam);
    [DllImport("user32.dll")]
    static extern IntPtr GetWindowDpiAwarenessContext(IntPtr window);
    [DllImport("user32.dll")]
    static extern bool AreDpiAwarenessContextsEqual(IntPtr left, IntPtr right);

    static void Require(bool condition, string message)
    {
        if (!condition) throw new Exception(message);
    }

    static void ChangeDpi(Form form, int dpi)
    {
        // Deliver the same WM_DPICHANGED message used when moving between screens.
        // This tests WinForms scaling and font changes without changing desktop settings.
        var bounds = new Rect { Left = -32000, Top = -32000, Right = -32000 + form.Width, Bottom = -32000 + form.Height };
        SendMessage(form.Handle, 0x02E0, new IntPtr(dpi | (dpi << 16)), ref bounds);
        Application.DoEvents();
        Require(form.DeviceDpi == dpi, "DPI change was not applied: " + form.DeviceDpi);
    }

    static void CheckLists(Control root)
    {
        foreach (var view in SelfTests.Descendants(root).OfType<ListView>())
        {
            UiFactory.FitIconColumns(view);
            int width = view.Columns.Cast<ColumnHeader>().Sum(x => x.Width);
            Require(width >= view.ClientSize.Width - SystemInformation.VerticalScrollBarWidth - 5,
                "List columns leave unused width: " + view.ClientSize + " columns " + width);
            foreach (ColumnHeader column in view.Columns)
                Require(column.Width >= TextRenderer.MeasureText(column.Text, view.Font).Width + 8, "Column heading clipped");
        }
    }

    internal static void Run(SettingsStore store, List<Folder> folders, string output)
    {
        var log = new List<string>();
        using (var form = new MainForm(store, false, false, new FakeStartupPreferencesStore()))
        {
            form.MaximumSize = new Size(6000, 4000);
            SelfTests.Render(form, Path.Combine(output, "dpi-initial.png"));
            Require(AreDpiAwarenessContextsEqual(GetWindowDpiAwarenessContext(form.Handle), new IntPtr(-4)),
                "Application did not opt into PerMonitorV2 awareness");
            var tabs = SelfTests.Descendants(form).OfType<TabControl>().Single();
            // Include a return to 100% to catch cumulative scaling drift.
            foreach (int dpi in new[] { 96, 120, 144, 192, 96 })
            {
                ChangeDpi(form, dpi);
                foreach (Size logical in new[] { new Size(1040, 760), new Size(900, 540) })
                {
                    form.ClientSize = new Size(logical.Width * dpi / 96, logical.Height * dpi / 96);
                    for (int page = 0; page < tabs.TabPages.Count; page++)
                    {
                        tabs.SelectedIndex = page;
                        string name = string.Format("dpi-{0}-{1}x{2}-page-{3}", dpi, logical.Width, logical.Height, page);
                        SelfTests.Render(form, Path.Combine(output, name + ".png"));
                        SelfTests.CheckButtons(tabs.TabPages[page]);
                        foreach (var button in SelfTests.Descendants(form).OfType<Button>().Where(x => x.Visible))
                        {
                            Size text = TextRenderer.MeasureText(button.Text, button.Font, Size.Empty, TextFormatFlags.SingleLine);
                            Require(button.Width >= text.Width + 8 && button.Height >= text.Height + 6, name + ": clipped " + button.Text);
                        }
                        CheckLists(tabs.TabPages[page]);
                        log.Add("PASS " + name);
                    }
                }
            }
            foreach (var screen in new[] {
                new[] { 1366, 768, 96 }, new[] { 1366, 768, 144 }, new[] { 1366, 768, 192 },
                new[] { 1920, 1080, 96 }, new[] { 1920, 1080, 144 },
                new[] { 2560, 1440, 144 }, new[] { 3840, 2160, 192 },
                new[] { 3042, 1763, 120 } })
            {
                int dpi = screen[2];
                ChangeDpi(form, dpi);
                var work = new Rectangle(-screen[0], 0, screen[0], screen[1] - 48 * dpi / 96);
                var fitted = DisplayLayout.ConstrainBounds(new Rectangle(-5000, -5000, 5000, 4000), work);
                Require(work.Contains(fitted) && fitted.Size == work.Size, "Window must fit smaller/left-hand monitor");
                // Use the simulated work area rather than the real monitor's minimum.
                // Size includes the caption, matching the space left above the taskbar.
                form.MinimumSize = Size.Empty;
                form.Size = work.Size;
                bool oversized = form.Size != work.Size;
                Control canvas = form.Controls[0];
                if (oversized)
                {
                    // Windows caps top-level windows at this machine's tracking size.
                    // Exercise the actual root layout at the requested pixel size.
                    canvas.Dock = DockStyle.None;
                    canvas.Size = new Size(work.Width, work.Height - 32 * dpi / 96);
                    Require(canvas.Width == work.Width, "Large resolution canvas was clamped");
                }
                for (int page = 0; page < tabs.TabPages.Count; page++)
                {
                    tabs.SelectedIndex = page;
                    var pageViewport = SelfTests.Descendants(tabs.TabPages[page]).OfType<PageViewport>().Single();
                    pageViewport.AutoScrollPosition = Point.Empty;
                    string name = string.Format("resolution-{0}x{1}-dpi-{2}-page-{3}", screen[0], screen[1], dpi, page);
                    if (oversized)
                        SelfTests.RenderSurface(canvas, Path.Combine(output, name + ".png"));
                    else
                        SelfTests.Render(form, Path.Combine(output, name + ".png"));
                    SelfTests.CheckButtons(tabs.TabPages[page]);
                    CheckLists(tabs.TabPages[page]);
                    foreach (var button in SelfTests.Descendants(tabs.TabPages[page]).OfType<Button>())
                    {
                        var viewport = SelfTests.Descendants(tabs.TabPages[page]).OfType<PageViewport>().Single();
                        viewport.ScrollControlIntoView(button);
                        Application.DoEvents();
                        Require(viewport.ClientRectangle.Contains(viewport.RectangleToClient(button.RectangleToScreen(button.ClientRectangle))),
                            name + ": cannot reach " + button.Text);
                    }
                    log.Add("PASS " + name + (oversized ? " (content canvas)" : " (window)"));
                }
                canvas.Dock = DockStyle.Fill;
            }
            ChangeDpi(form, 96);
            form.ClientSize = new Size(640, 420);
            tabs.SelectedIndex = 0;
            Application.DoEvents();
            var compactViewport = SelfTests.Descendants(tabs.TabPages[0]).OfType<PageViewport>().Single();
            var save = SelfTests.Descendants(compactViewport).OfType<Button>().Single(x => x.Text == "保存并应用");
            compactViewport.ScrollControlIntoView(save);
            Application.DoEvents();
            Rectangle visibleSave = compactViewport.RectangleToClient(save.RectangleToScreen(save.ClientRectangle));
            Require(compactViewport.ClientRectangle.Contains(visibleSave), "Save cannot be reached by scrolling on a small screen");
            SelfTests.Render(form, Path.Combine(output, "compact-scrolled-save.png"));
        }
        using (var icon = AppAssets.LoadIcon())
        using (var picker = new IconPickerDialog(new InventoryReader(store).Read(folders), folders, "second", icon))
        {
            SelfTests.Render(picker, Path.Combine(output, "picker-initial.png"));
            foreach (int dpi in new[] { 96, 120, 144, 192, 96 })
            {
                ChangeDpi(picker, dpi);
                picker.ClientSize = new Size(680 * dpi / 96, 400 * dpi / 96);
                SelfTests.Render(picker, Path.Combine(output, "picker-dpi-" + dpi + ".png"));
                SelfTests.CheckButtons(picker);
                CheckLists(picker);
                log.Add("PASS picker dpi " + dpi);
            }
        }
        File.WriteAllLines(Path.Combine(output, "display-checks.txt"), log);
    }
}
