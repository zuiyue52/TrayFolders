// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

internal sealed partial class MainForm : Form
{
    readonly SettingsStore store;
    readonly InventoryReader inventoryReader;
    readonly EngineController engine;
    readonly StartupRetry startup;
    readonly bool interactive;
    readonly List<Folder> folders;
    List<Folder> appliedFolders;
    List<TrayIconInfo> inventory = new List<TrayIconInfo>();
    Folder selected;
    bool filling, exitRequested;
    readonly Timer timer = new Timer();
    NotifyIcon tray;
    internal MainForm(SettingsStore settings, bool live, bool autoStart = true, IStartupPreferencesStore startupPreferences = null)
    {
        SuspendLayout();
        store = settings;
        interactive = live;
        folders = store.Load();
        appliedFolders = store.Load();
        inventoryReader = new InventoryReader(store);
        engine = new EngineController(new NativeEngineTransport());
        startup = new StartupRetry(autoStart);
        InitializeLayout();
        InitializeStartupSettings(startupPreferences ?? new RegistryStartupPreferencesStore(Application.ExecutablePath));
        InitializeCompatibility();
        DisplayLayout.FitToScreen(this, new System.Drawing.Size(640, 420));
        ResumeLayout(true);
        folderList.SelectedIndexChanged += delegate
        {
            if (filling)
                return;
            CommitSelected();
            selected = folderList.SelectedItem as Folder;
            LoadSelected();
        };
        RefreshFolders(folders.FirstOrDefault());
        RefreshInventory(true);
        UpdateState();
        Tell("把托盘图标拖入文件夹后，列表会自动更新。关闭窗口后仍在托盘运行。");
        if (interactive)
            InitializeTray();
    }

    void InitializeTray()
    {
        tray = new NotifyIcon
        {
            Icon = Icon,
            Text = "托盘文件夹",
            Visible = true
        };
        var menu = new ContextMenuStrip();
        menu.Items.Add("打开设置", null, delegate
        {
            ShowWindow();
        });
        menu.Items.Add("刷新图标", null, delegate
        {
            RefreshNow();
        });
        menu.Items.Add("停用分组", null, delegate
        {
            StopEngine();
        });
        menu.Items.Add("退出并恢复托盘", null, delegate
        {
            exitRequested = true;
            Close();
        });
        tray.ContextMenuStrip = menu;
        tray.DoubleClick += delegate
        {
            ShowWindow();
        };
        timer.Interval = 750;
        timer.Tick += delegate
        {
            RefreshInventory(false);
            if (!preparing && startup.IsDue(DateTime.UtcNow))
                BeginStartEngine(true);
            else if (!preparing && engine.PrepareReconnect())
                StartEngine();
            CheckRuntimeCompatibility();
        };
        timer.Start();
        FormClosing += OnClosing;
    }

    void Tell(string text)
    {
        message.Text = text;
    }

    void StartEngine()
    {
        startup.Cancel();
        BeginStartEngine(false);
    }

    bool StopEngine()
    {
        if (preparation != null)
            preparation.Cancel();
        runtimePending = false;
        startup.Cancel();
        try
        {
            int result = engine.Stop();
            if (result < 0)
            {
                Tell(EngineController.ErrorText(result));
                return false;
            }

            UpdateState();
            Tell("分组已停用。");
            return true;
        }
        catch (Exception error)
        {
            Tell("停用失败：" + error.Message);
            return false;
        }
    }

    void UpdateState()
    {
        start.Enabled = !engine.IsEnabled && !preparing;
        stop.Enabled = engine.IsEnabled || preparing;
        if (detect != null)
            detect.Enabled = !preparing;
    }

    void RequestInventory(bool report)
    {
        if (!interactive)
            return;
        if (!engine.IsEnabled)
        {
            if (report)
                Tell("当前分组已停用；启用后可重新读取托盘图标。");
            return;
        }

        int result = engine.RefreshInventory();
        if (report)
            Tell(result == 1 ? "已刷新当前托盘图标。" : result == 2 ? "请先展开一次右下角隐藏图标，再点击刷新图标。" : EngineController.ErrorText(result));
    }

    void ShowWindow()
    {
        Show();
        WindowState = FormWindowState.Normal;
        Activate();
        RefreshInventory(true);
        startupSettings.RefreshState(false);
    }

    void OnClosing(object sender, FormClosingEventArgs e)
    {
        if (!exitRequested && e.CloseReason == CloseReason.UserClosing)
        {
            e.Cancel = true;
            Hide();
            return;
        }

        if (engine.IsEnabled && !StopEngine() && e.CloseReason == CloseReason.UserClosing)
        {
            e.Cancel = true;
            exitRequested = false;
            return;
        }

        if (preparation != null)
            preparation.Cancel();
        timer.Stop();
        startup.Cancel();
        if (tray != null)
        {
            tray.Visible = false;
            tray.Dispose();
        }

        engine.Dispose();
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            if (preparation != null)
                preparation.Cancel();
            timer.Dispose();
            if (tray != null)
                tray.Dispose();
            engine.Dispose();
        }

        base.Dispose(disposing);
    }
}
