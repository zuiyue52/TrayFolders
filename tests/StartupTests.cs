// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Drawing;
using System.Windows.Forms;

internal static class StartupTests
{
    static void Check(bool condition, string message)
    {
        if (!condition)
            throw new Exception(message);
    }

    internal static void Run(SettingsStore store)
    {
        StartupOptions options;
        Check(StartupOptions.TryParse(new[] { "--silent" }, out options) && options.Silent && options.AutoStart, "Silent startup options incorrect");
        Check(StartupOptions.TryParse(new[] { "--silent", "--no-start" }, out options) && !options.AutoStart, "No-start option ignored");
        Check(!StartupOptions.TryParse(new[] { "--status" }, out options), "Diagnostic command routed to GUI");
        var now = new DateTime(2026, 1, 1, 0, 0, 0, DateTimeKind.Utc);
        var retry = new StartupRetry(true);
        Check(retry.IsDue(now), "Startup not scheduled");
        retry.RecordResult(-1, now);
        Check(!retry.IsDue(now.AddSeconds(2)) && retry.IsDue(now.AddSeconds(3)), "Shell retry interval incorrect");
        retry.RecordResult(-2, now.AddSeconds(3));
        Check(retry.IsDue(now.AddSeconds(6)), "Taskbar module startup is not retried");
        retry.RecordResult(1, now.AddSeconds(6));
        Check(!retry.IsDue(now.AddHours(1)), "Successful startup retries again");
        retry = new StartupRetry(true);
        retry.RecordResult(-10, now);
        Check(!retry.IsDue(now.AddHours(1)), "Incompatible Windows should not retry");
        retry = new StartupRetry(true);
        for (int i = 0; i < 20; i++)
            retry.RecordResult(-1, now.AddSeconds(i * 3));
        Check(!retry.IsDue(now.AddHours(1)), "Retries never stop");
        retry = new StartupRetry(true);
        retry.Cancel();
        Check(!retry.IsDue(now), "Manual stop failed to cancel startup");
        VerifyHiddenWindow(store, false);
        VerifyHiddenWindow(store, true);
    }

    static void VerifyHiddenWindow(SettingsStore store, bool openLater)
    {
        using (var form = new MainForm(store, false, false, new FakeStartupPreferencesStore()))
        {
            form.StartPosition = FormStartPosition.Manual;
            form.Location = new Point(-32000, -32000);
            int shown = 0;
            form.Shown += delegate
            {
                shown++;
            };
            using (var context = new TrayApplicationContext(form, true))
            using (var timer = new Timer
            {
                Interval = 50
            }

            )
            {
                Check(form.IsHandleCreated && !form.Visible && shown == 0, "Silent startup displayed the window");
                bool firstTick = true;
                timer.Tick += delegate
                {
                    if (firstTick)
                    {
                        firstTick = false;
                        Check(!form.Visible && shown == 0, "Message loop displayed a silent window");
                        if (openLater)
                        {
                            form.Show();
                            return;
                        }
                    }

                    timer.Stop();
                    form.Close();
                };
                timer.Start();
                Application.Run(context);
                Check(shown == (openLater ? 1 : 0), "Silent window cannot be opened on demand or closed");
            }
        }
    }
}
