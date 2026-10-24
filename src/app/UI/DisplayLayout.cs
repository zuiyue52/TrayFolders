// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Drawing;
using System.Windows.Forms;
using Microsoft.Win32;

internal static class DisplayLayout
{
    // Always derive minimum sizes from logical units, avoiding cumulative rounding.
    internal static int Units(Control control, int logical)
    {
        var form = control.FindForm();
        return (int)Math.Ceiling(logical * (form == null ? control.DeviceDpi : form.DeviceDpi) / 96.0);
    }

    internal static void FitToScreen(Form form, Size logicalMinimum)
    {
        Rectangle lastWorkArea = Rectangle.Empty;
        bool pending = false;
        Action fit = delegate
        {
            pending = false;
            if (form.IsDisposed || form.Disposing) return;
            Rectangle work = Screen.FromControl(form).WorkingArea;
            lastWorkArea = work;
            form.MinimumSize = new Size(Math.Min(Units(form, logicalMinimum.Width), work.Width),
                Math.Min(Units(form, logicalMinimum.Height), work.Height));
            if (form.WindowState != FormWindowState.Normal)
                return;
            // Manual positions are used by the off-screen UI tests.
            if (form.StartPosition == FormStartPosition.Manual && form.Left < -10000)
                return;
            form.Bounds = ConstrainBounds(form.Bounds, work);
        };
        Action schedule = delegate
        {
            if (pending || !form.IsHandleCreated || form.IsDisposed || form.Disposing) return;
            pending = true;
            try { form.BeginInvoke(fit); }
            catch (InvalidOperationException) { pending = false; }
        };
        form.Load += delegate { fit(); };
        form.DpiChanged += delegate { schedule(); };
        form.ResizeEnd += delegate { fit(); };
        form.LocationChanged += delegate
        {
            if (Screen.FromControl(form).WorkingArea != lastWorkArea) schedule();
        };
        EventHandler displayChanged = delegate { schedule(); };
        SystemEvents.DisplaySettingsChanged += displayChanged;
        form.Disposed += delegate { SystemEvents.DisplaySettingsChanged -= displayChanged; };
    }

    internal static Rectangle ConstrainBounds(Rectangle bounds, Rectangle work)
    {
        Size size = new Size(Math.Min(bounds.Width, work.Width), Math.Min(bounds.Height, work.Height));
        return new Rectangle(new Point(Math.Max(work.Left, Math.Min(bounds.Left, work.Right - size.Width)),
            Math.Max(work.Top, Math.Min(bounds.Top, work.Bottom - size.Height))), size);
    }
}

// WinForms updates inherited fonts on a DPI change; explicitly styled headings
// need to follow the form's new font too, including pages opened later.
internal sealed class HeadingLabel : Label
{
    readonly float multiplier;
    Form owner;
    Font headingFont;

    internal HeadingLabel(string text, float multiplier)
    {
        this.multiplier = multiplier;
        Text = text;
        AutoSize = true;
        Dock = DockStyle.Fill;
        TextAlign = ContentAlignment.MiddleLeft;
    }

    protected override void OnHandleCreated(EventArgs e)
    {
        base.OnHandleCreated(e);
        owner = FindForm();
        if (owner != null)
        {
            owner.FontChanged += UpdateFont;
            UpdateFont(this, EventArgs.Empty);
        }
    }

    void UpdateFont(object sender, EventArgs e)
    {
        Font previous = headingFont;
        headingFont = new Font(owner.Font.FontFamily, owner.Font.SizeInPoints * multiplier, FontStyle.Bold);
        Font = headingFont;
        if (previous != null) previous.Dispose();
    }

    protected override void OnHandleDestroyed(EventArgs e)
    {
        if (owner != null) owner.FontChanged -= UpdateFont;
        owner = null;
        base.OnHandleDestroyed(e);
    }

    protected override void Dispose(bool disposing)
    {
        base.Dispose(disposing);
        if (disposing && headingFont != null) headingFont.Dispose();
    }
}

// Short displays scroll the page instead of compressing action rows or hiding Save.
internal sealed class PageViewport : Panel
{
    readonly Control content;
    readonly Size logicalMinimum;
    bool arranging;

    internal PageViewport(Control content, int minimumWidth, int minimumHeight)
    {
        this.content = content;
        logicalMinimum = new Size(minimumWidth, minimumHeight);
        Dock = DockStyle.Fill;
        Margin = Padding.Empty;
        AutoScroll = true;
        content.Dock = DockStyle.None;
        Controls.Add(content);
    }

    protected override void OnLayout(LayoutEventArgs e)
    {
        if (arranging || content == null)
            return;
        arranging = true;
        try
        {
            AutoScrollMinSize = new Size(DisplayLayout.Units(this, logicalMinimum.Width),
                DisplayLayout.Units(this, logicalMinimum.Height));
            // Clear obsolete scrollbars before expanding to the available area.
            // Otherwise an old, wider canvas can keep a horizontal scrollbar alive.
            content.Bounds = new Rectangle(AutoScrollPosition, AutoScrollMinSize);
            base.OnLayout(e);
            content.Bounds = new Rectangle(AutoScrollPosition,
                new Size(Math.Max(ClientSize.Width, AutoScrollMinSize.Width),
                    Math.Max(ClientSize.Height, AutoScrollMinSize.Height)));
        }
        finally { arranging = false; }
    }
}
