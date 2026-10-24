// SPDX-License-Identifier: GPL-3.0-only
// Wire values must match src/native/lifecycle.inc.
internal enum EngineCommand
{
    Start = 1,
    ReloadSettings = 2,
    Stop = 3,
    Query = 4,
    RefreshInventory = 5,
    CheckCompatibility = 6
}
