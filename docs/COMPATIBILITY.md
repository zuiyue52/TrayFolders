# 兼容性与常见问题 / Compatibility and FAQ

## 验证范围 / Validation scope

目标为 Windows 11 x64。托盘组件版本不同于 Windows 显示版本，不能仅凭“24H2”等名称判定兼容。 / Target: Windows 11 x64. Component versions are distinct from Windows release names.

| 组件 / Component | 证据 / Evidence |
| --- | --- |
| Taskbar.View.dll 2126.8901.10.0 | 开发环境验证分组、拖动、恢复及运行校验。 / Grouping, drag, restoration and runtime checks exercised in the development environment. |
| Taskbar.View.dll 2125.30601.0.0 | 仅独立解析、内存映射与缓存测试；无完整系统 UI 回归。 / Resolver, mapped-image and cache checks only; no full-system UI regression. |
| SystemTray.dll 2606.29002.0.0 | 用户日志中组件/地址检查通过，随后 DLL 加载失败；不算完整兼容验证。 / User log passed component/address checks, then failed native loading; not an end-to-end success. |
| 其他版本 / Other versions | 未逐一验证，以运行检查为准。 / Not individually tested. |

流程：文件检查 → 识别组件 → 匹配符号 → 解析函数 → 选择适配 → 校验地址 → 首次展开时校验界面结构。下载符号成功不等于界面兼容，内部结构变化仍可能需要代码更新。

Flow: check files, identify the component, match symbols, resolve functions, select the adapter, validate addresses and finally validate the UI on first expansion. Successful symbol download does not prove UI compatibility.

## DLL 错误 / DLL errors

`TrayFolders.Native.v5.dll` 是必要核心。`0x8007007E` 可能是该文件缺失，也可能是依赖未能加载，不能只凭错误码判断。 / The native DLL is essential. Error `0x8007007E` may mean a missing file or an unresolved dependency; the code alone does not identify the cause.

在完整解压后的程序目录运行 / Run from the extracted application folder:

```powershell
Start-Process .\TrayFolders.exe -ArgumentList '--check-files' -Wait
Get-Content .\command-result.txt
```

检查会加载本机 DLL 并验证导出，不挂钩 Explorer、不下载符号。缺文件时列出名称，加载失败时报告 Win32 错误。通过只说明文件可加载，不代表该系统的分组已兼容。

This loads the local DLL and checks exports without Explorer hooks or downloads. Missing files are named; load failures include Win32 error details. Passing does not establish tray compatibility.

若失败，重新完整解压同版本便携包，对照 SHA256，确认 x64 系统，并查看安全软件的隔离记录。不要从不明来源下载替代 DLL。 / Re-extract the matching archive, verify hashes, check architecture and review quarantine history. Avoid arbitrary replacement DLL downloads.

## 名单与刷新 / Inventory and refresh

先创建文件夹、启用分组，展开隐藏图标面板再刷新。手动加入和拖动优先于文字规则；同名程序图标共享手动归属。刷新仅更新快照、名单和数量，不清空规则、不重启 Explorer，也不代表重新下载符号。

Create a folder, enable grouping and open the flyout before refreshing. Explicit assignments override patterns and are shared by icons with the same app name. Refresh updates the snapshot, inventory and counts; it does not clear rules, restart Explorer or redownload symbols.

## 屏幕与缩放 / Displays and scaling

PerMonitorV2 配合滚动保持控件可用。测试覆盖 100%、125%、150%、200% 缩放及多种模拟分辨率/窗口尺寸。模拟布局不等于所有物理显示器、远程桌面或混合 DPI 环境都已验证。

PerMonitorV2 and scrolling keep controls usable. Layout tests simulate 100%, 125%, 150% and 200% scaling and several resolutions/sizes. These do not replace physical multi-monitor or remote-desktop testing.

## 数据位置 / Data locations

| 位置 / Location | 内容 / Contents |
| --- | --- |
| 程序目录 / Application folder | `settings.ini`：规则 / rules；`assignments.ini`：归属 / assignments；`icons.tsv`：名单 / inventory；`compatibility.bin`：适配数据 / profile；`command-result.txt`：诊断 / diagnostics |
| `%LOCALAPPDATA%\TrayFolders\Symbols` | PDB 和解析缓存 / PDB and resolver cache |
| `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` 下的 `TrayFolders` | 当前用户自启动 / Current-user startup |
| `HKCU\Software\TrayFolders` | 静默偏好 / Silent-start preference |

## 诊断与恢复 / Diagnosis and recovery

`--probe` 只使用已有缓存；`--prepare` 必要时下载符号，两者不启用分组。`--status`、`--compat-status`、`--refresh`、`--stop` 会连接 Explorer。 / Probe is offline; prepare may download symbols. Neither enables grouping. Status, compatibility-status, refresh and stop commands connect to Explorer.

异常时先停用分组或选择“退出并恢复托盘”。若 Explorer 已无响应，保存工作后可在任务管理器重启 Windows 资源管理器。反馈请提供系统 build、组件版本、缩放比例和脱敏日志。 / First disable grouping or exit through the tray menu. If Explorer is unresponsive, save work before restarting Windows Explorer in Task Manager. Include OS build, component version, scaling and sanitized logs in reports.
