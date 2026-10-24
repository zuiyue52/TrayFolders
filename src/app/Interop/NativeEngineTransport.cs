// SPDX-License-Identifier: GPL-3.0-only
internal sealed class NativeEngineTransport : IEngineTransport
{
    bool loaded;
    public int Send(EngineCommand command)
    {
        try
        {
            int result = NativeMethods.TF_Command((int)command);
            loaded = true;
            return result;
        }
        catch (System.DllNotFoundException error) { throw ReleaseFiles.LoadFailure(error); }
        catch (System.BadImageFormatException error) { throw ReleaseFiles.LoadFailure(error); }
        catch (System.EntryPointNotFoundException error) { throw ReleaseFiles.LoadFailure(error); }
    }

    public void Disconnect()
    {
        if (loaded)
        {
            NativeMethods.TF_Disconnect();
            loaded = false;
        }
    }

    public uint ShellProcessId
    {
        get
        {
            uint pid;
            NativeMethods.GetWindowThreadProcessId(NativeMethods.FindWindow("Shell_TrayWnd", null), out pid);
            return pid;
        }
    }
}
