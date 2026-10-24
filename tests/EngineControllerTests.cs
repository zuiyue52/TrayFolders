// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;

internal static class EngineControllerTests
{
    sealed class FakeTransport : IEngineTransport
    {
        public uint ShellProcessId { get; set; }

        internal readonly List<EngineCommand> Commands = new List<EngineCommand>();
        internal int Result = 1, Disconnects;
        public int Send(EngineCommand command)
        {
            Commands.Add(command);
            return Result;
        }

        public void Disconnect()
        {
            Disconnects++;
        }
    }

    static void Check(bool value, string description)
    {
        if (!value)
            throw new Exception(description);
    }

    internal static void Run()
    {
        var transport = new FakeTransport
        {
            ShellProcessId = 10
        };
        using (var engine = new EngineController(transport))
        {
            Check(engine.ReloadSettings() == 0 && transport.Commands.Count == 0, "Disabled reload must not contact Explorer");
            Check(engine.Start() == 1 && engine.IsEnabled, "Start state lost");
            transport.Result = 2;
            Check(engine.RefreshInventory() == 2 && engine.IsEnabled, "Unopened panel incorrectly disabled grouping");
            transport.Result = -5;
            Check(engine.Stop() == -5 && engine.IsEnabled && transport.Disconnects == 0, "Failed cleanup must retain the connection");
            transport.ShellProcessId = 0;
            Check(!engine.PrepareReconnect(), "Missing Explorer should not repeatedly reconnect");
            transport.ShellProcessId = 11;
            Check(engine.PrepareReconnect() && !engine.IsEnabled && transport.Disconnects == 1, "Explorer restart not detected");
            transport.Result = 1;
            Check(engine.Start() == 1 && !engine.PrepareReconnect(), "Reconnect state incorrect");
            transport.Result = 0;
            Check(engine.Stop() == 0 && !engine.IsEnabled, "Stop state incorrect");
        }

        Check(transport.Disconnects == 2, "Connection was leaked or disconnected twice");
    }
}
