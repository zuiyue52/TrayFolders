// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Windows.Forms;

[assembly: System.Runtime.Versioning.TargetFramework(".NETFramework,Version=v4.8")]

internal static class TestProgram
{
    [STAThread]
    static int Main(string[] args)
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        if (args.Length != 1)
            return 2;
        return SelfTests.Run(args[0]);
    }
}
