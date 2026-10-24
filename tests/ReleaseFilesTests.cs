// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.IO;
using System.Linq;

internal static class ReleaseFilesTests
{
    internal static void Run(string output)
    {
        string root = Path.Combine(output, "release-files-fixture-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        if (ReleaseFiles.Missing(root).Length != 4 || new SettingsStore(root).Load().Count != 0)
            throw new Exception("Fresh installation must be empty and report missing runtime files");
        foreach (string name in ReleaseFiles.RequiredNames)
            File.WriteAllText(Path.Combine(root, name), "invalid test file");
        string native = ReleaseFiles.RequiredNames.Single(x => x.EndsWith(".dll"));
        File.Delete(Path.Combine(root, native));
        try { ReleaseFiles.EnsureReady(root); throw new Exception("Missing native library accepted"); }
        catch (FileNotFoundException e)
        {
            if (!e.Message.Contains(native) || !e.Message.Contains(root))
                throw new Exception("Missing DLL message omitted file or directory");
        }
        File.WriteAllText(Path.Combine(root, native), "invalid native binary");
        bool rejected = false;
        try { ReleaseFiles.EnsureReady(root); }
        catch (InvalidOperationException e) { rejected = e.Message.Contains("Win32") && e.Message.Contains(native); }
        if (!rejected) throw new Exception("Invalid native file did not produce an actionable error");
        new NativeEngineTransport().Disconnect();
    }
}
