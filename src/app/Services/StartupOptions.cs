// SPDX-License-Identifier: GPL-3.0-only
internal sealed class StartupOptions
{
    internal bool Silent { get; private set; }
    internal bool AutoStart { get; private set; }

    internal static bool TryParse(string[] args, out StartupOptions options)
    {
        options = new StartupOptions
        {
            AutoStart = true
        };
        foreach (string arg in args)
        {
            switch (arg)
            {
                case "--silent":
                    options.Silent = true;
                    break;
                case "--no-start":
                    options.AutoStart = false;
                    break;
                default:
                    options = null;
                    return false;
            }
        }

        return true;
    }
}
