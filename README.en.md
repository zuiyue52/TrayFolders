# TrayFolders / 托盘文件夹

[简体中文](README.md) · [Downloads](https://github.com/zuiyue52/TrayFolders-Public/releases/tag/v0.4.2-preview.1) · **0.4.2-preview.1** · Windows 11 x64  
Maintainer: **c_zh** · Project code license: **GPL-3.0-only**

Create folders inside the Windows 11 **Show hidden icons** flyout. Organize tray icons by dragging, selecting from the current inventory, or matching their names. TrayFolders runs independently of Windhawk.

This community preview uses internal Explorer interfaces. It is not an official Windows feature. Dynamic symbols help with some component changes, but do not guarantee compatibility with every Windows 11 version.

![Folder management with synthetic test data](docs/images/folders.png)

## Features

- Create, rename, delete and reorder folders; use member previews or a custom glyph.
- Select current tray icons or match names with `*` and `?`.
- Drag icons onto folders to save membership; drag onto the back tile to remove them.
- Automatically update inventory and counts after dragging, with manual refresh available.
- Configure current-user login startup and silent launch.
- Adapt to display DPI, with scrollable pages at smaller sizes.

## Packages and requirements

| File | Contents |
| --- | --- |
| `TrayFolders-0.4.2-preview.1-win-x64.zip` | Complete portable application |
| `TrayFolders-0.4.2-preview.1-source.zip` | Corresponding source, tests, build scripts and vendored code |
| `SHA256SUMS.txt` | SHA256 hashes for both archives |

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

## Development and origins

- [Build, tests and architecture](docs/BUILD.md)
- [Compatibility and troubleshooting](docs/COMPATIBILITY.md)
- [Changelog](CHANGELOG.md)
- [Third-party notices](THIRD-PARTY-NOTICES.md)
- [Contributing](CONTRIBUTING.md)

The core derives from **Ris Peng's** [System Tray Folders 1.0.0](https://github.com/ramensoftware/windhawk-mods/blob/a1190b6c1b631a725f6d628189886d410b3760f6/mods/system-tray-folders.wh.cpp), retaining its credits to **m417z's** tray mods. Hooking uses [MinHook](https://github.com/TsudaKageyu/minhook). This project adds the standalone controller, settings UI, persistent drag membership, startup preferences, dynamic adaptation and display layout handling. It is not an official release of those projects.

Project code is [GPL-3.0-only](LICENSE); third-party terms are retained. Distribute matching source and notices alongside the portable archive. Source: [zuiyue52/TrayFolders-Public](https://github.com/zuiyue52/TrayFolders-Public). Report problems through [Issues](https://github.com/zuiyue52/TrayFolders-Public/issues).
