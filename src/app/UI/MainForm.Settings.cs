// SPDX-License-Identifier: GPL-3.0-only
using System.Windows.Forms;

internal sealed partial class MainForm : Form
{
    StartupSettingsPanel startupSettings;
    void InitializeStartupSettings(IStartupPreferencesStore preferences)
    {
        var page = new TabPage("设置");
        startupSettings = new StartupSettingsPanel(preferences);
        page.Controls.Add(startupSettings);
        tabs.TabPages.Add(page);
        tabs.SelectedIndexChanged += delegate
        {
            if (tabs.SelectedTab == page)
                startupSettings.RefreshState(false);
        };
    }
}
