// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Threading;
using System.Windows.Forms;

internal static class Program
{
    [STAThread]
    static int Main(string[] args)
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        string directory = AppDomain.CurrentDomain.BaseDirectory;
        int exitCode;
        if (CommandLine.TryRun(args, directory, out exitCode))
            return exitCode;
        StartupOptions options;
        StartupOptions.TryParse(args, out options);
        bool created;
        using (var mutex = new Mutex(true, "Local\\TrayFolders.Standalone.Controller", out created))
        {
            if (!created)
            {
                if (!options.Silent)
                    MessageBox.Show("程序已运行，请双击托盘图标打开设置。", "托盘文件夹");
                return 0;
            }

            using (var context = new TrayApplicationContext(new MainForm(new SettingsStore(directory), true, options.AutoStart), options.Silent))
                Application.Run(context);
        }

        return 0;
    }
}
