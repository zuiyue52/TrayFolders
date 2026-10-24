// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Windows.Forms;

// Do not set ApplicationContext.MainForm: WinForms would show it automatically
// when the message loop starts. A hidden handle still receives close/session events.
internal sealed class TrayApplicationContext : ApplicationContext
{
    readonly MainForm settingsWindow;
    internal TrayApplicationContext(MainForm form, bool silent)
    {
        settingsWindow = form;
        settingsWindow.FormClosed += OnFormClosed;
        if (silent)
        {
            IntPtr handle = settingsWindow.Handle;
        }
        else
        {
            settingsWindow.Show();
        }
    }

    void OnFormClosed(object sender, FormClosedEventArgs e)
    {
        ExitThread();
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            settingsWindow.FormClosed -= OnFormClosed;
            settingsWindow.Dispose();
        }

        base.Dispose(disposing);
    }
}
