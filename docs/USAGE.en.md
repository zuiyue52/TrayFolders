# User guide

[Project home](../README.en.md) · [简体中文](USAGE.zh-CN.md) · [Documentation](README.md)

## Packages and requirements

| File | Contents |
| --- | --- |
| [TrayFolders-0.4.2-preview.1-win-x64.zip](https://github.com/zuiyue52/TrayFolders/releases/download/v0.4.2-preview.1/TrayFolders-0.4.2-preview.1-win-x64.zip) | Complete portable application |
| [TrayFolders-0.4.2-preview.1-source.zip](https://github.com/zuiyue52/TrayFolders/releases/download/v0.4.2-preview.1/TrayFolders-0.4.2-preview.1-source.zip) | Corresponding source, tests, build scripts and vendored code |
| [SHA256SUMS.txt](https://github.com/zuiyue52/TrayFolders/releases/download/v0.4.2-preview.1/SHA256SUMS.txt) | SHA256 hashes for both archives |

Requires **Windows 11 x64, .NET Framework 4.8** and a writable application folder. Initial preparation may need access to Microsoft's symbol server. The UI is currently Chinese; main documentation is bilingual. ARM64, 32-bit systems and Windows 10 are outside this preview's scope.

**Extract the whole portable ZIP and run `TrayFolders.exe`. Do not run from inside the archive or copy only the EXE.** Keep these files together:

```text
TrayFolders.exe
TrayFolders.exe.config
TrayFolders.Native.v5.dll
TrayFolders.Symbols.exe
settings.ini
assignments.ini
```

The first four are required runtime components. The INIs hold folders and explicit assignments. Clean defaults are included; a new installation has no personal rules and does not register login startup automatically.

## Getting started

1. Under **文件夹管理** (Folder management), click **＋ 新建** (New), name the folder and click **保存并应用** (Save and apply).
2. Click **启用分组** (Enable grouping). Wait for preparation in **系统适配** (System compatibility), then open the Windows hidden-icons flyout once.
3. Select a folder and click **＋ 添加图标…** (Add icons), or drag an icon onto the folder in the flyout.
4. Enter one name rule per line, such as `*Everything*` or `*Weixin*`, and save.
5. **刷新图标** (Refresh icons) rereads the tray snapshot and updates lists and counts. It does not restart applications or clear rules. Open the flyout first if the list is empty.

Explicit selections and drag assignments override text rules. Icons sharing an application name share explicit membership. Patterns match application, tooltip or accessible names, ignoring case; the first matching folder wins.

Leave **显示符号** (Display glyph) blank for member previews, or enter short text, emoji or a code point such as `U+E7FC`. Rendering depends on system fonts.

Closing settings leaves the app running. Double-click the colored-square tray icon to reopen it. Use **退出并恢复托盘** (Exit and restore tray) to exit. Disable equivalent Windhawk grouping mods first and run only one TrayFolders controller.

## Startup, upgrades and removal

In **设置** (Settings), select **开机自启动（登录 Windows 后）** (Start at login) and optionally **自启动时静默运行** (Silent startup), then click **保存启动设置** (Save startup settings). Silent launch hides the settings window. Registration applies only to the current user.

For upgrades, exit the old app, extract the new package into a new directory and copy only `settings.ini` and `assignments.ini` if desired. Do not carry over old DLLs, `compatibility.bin` or temporary files. Save startup settings again after moving the app.

To remove it, disable login startup and save, exit through the tray menu, then delete the application folder. The optional cache at `%LOCALAPPDATA%\TrayFolders\Symbols` can be deleted after exit.

## Data and networking

Configuration, inventory and compatibility data are stored beside the executable. The resolver downloads matching PDBs from Microsoft's HTTPS symbol service and uses a local cache. The code does not contain analytics or inventory uploads. Review logs and screenshots for names, tooltip text and personal paths before sharing them.
