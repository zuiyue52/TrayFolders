# 构建与项目结构 / Build and architecture

## 环境 / Prerequisites

Windows 11 x64、PowerShell 7、.NET Framework 4.8（含系统 C# 编译器）、[LLVM-MinGW](https://github.com/mstorsjo/llvm-mingw) 和 [C++/WinRT](https://github.com/microsoft/cppwinrt) 生成的头文件。

Windows 11 x64, PowerShell 7, .NET Framework 4.8 with the Framework C# compiler, LLVM-MinGW and generated C++/WinRT headers are required.

此包构建使用 Clang 20.1.3、C++/WinRT 2.0.240405.15、Framework64 C# 编译器；编译器来自已有 Windhawk 工具链目录，但程序运行不依赖 Windhawk。可用独立 LLVM-MinGW，并通过 `-WinRtInclude` 指向包含 `winrt/base.h` 的目录。不提供这些编译工具的二进制。

This package was built with Clang 20.1.3, C++/WinRT 2.0.240405.15 and the Framework64 C# compiler. The local compiler distribution came from a Windhawk toolchain; the application does not depend on Windhawk at runtime. A standalone LLVM-MinGW distribution can be used with the generated WinRT include directory. Toolchain binaries are not bundled.

## 命令 / Commands

在源码根目录运行 / Run from the source root:

```powershell
pwsh .\scripts\Build.ps1 -CompilerRoot 'D:\llvm-mingw' -WinRtInclude 'D:\cppwinrt\include'
pwsh .\scripts\Test.ps1 -IncludeReleaseChecks
pwsh .\scripts\Package.ps1
```

构建产物位于 `build/bin`；发布文件位于 `release`，包含两个独立 ZIP 和校验文件。仅修改界面时可用 `Build.ps1 -AppOnly`，但打包仍需完整的原生核心和解析器。构建不会修改个人安装。

Build output goes to `build/bin`; packaging writes two archives and checksums to `release`. `Build.ps1 -AppOnly` builds just the host, but packaging still requires the native library and resolver. Builds do not modify an installed application.

`Test.ps1` 的窗口不显示在任务栏，也不挂钩 Explorer。启动设置测试只使用临时注册表分支。测试包括规则/手动归属、文件夹操作、刷新保留编辑、启动设置、文件缺失错误及 DPI 布局。发布测试检查干净配置、ZIP 内容、哈希和更新保留个人数据。

Tests use off-screen windows and do not install Explorer hooks. Startup tests use an isolated registry prefix. They cover assignments, folder actions, refresh/edit preservation, startup, file failures and DPI layouts; release tests verify archive contents, hashes and configuration preservation.

`Test-Adaptation.ps1` 提供额外原生适配测试；参数见脚本。它需要真实 Windows 托盘组件与匹配缓存，默认不联网。完整运行测试仍需实际展开托盘，验证点击、拖放、停用及恢复。

`Test-Adaptation.ps1` provides additional native checks against real Windows components and matching cached symbols, offline by default. Real tray interaction is still needed to validate clicks, dragging and recovery.

## 结构 / Structure

| 路径 / Path | 职责 / Responsibility |
| --- | --- |
| `src/app/UI` | WinForms 窗口及交互 / UI and interactions |
| `src/app/Services` | 配置、启动、发布文件检查和适配准备 / Configuration, startup, file checks and preparation |
| `src/app/Interop`、`Domain` | Windows 互操作与模型 / Interop and models |
| `src/native` | Explorer 内分组、拖动、适配与符号解析器 / In-process grouping, drag, adapters and resolver |
| `vendor/minhook` | 带许可证的 MinHook 源码 / Vendored MinHook source |
| `tests` | 独立测试 / Separate test executable and release checks |
| `scripts` | 构建、打包和更新 / Build, package and update |
| `config/defaults` | 空白 UTF-16 LE 配置 / Neutral UTF-16 LE defaults |
| `assets`、`licenses`、`docs` | 资源、许可证和说明 / Assets, licenses and documentation |

托管控制程序通过原生 DLL 与 Explorer 通信。符号下载和解析在独立辅助进程中进行。原生实现按职责拆分成 `.inc` 文件，并保持单个编译单元。更详细的中文内部记录见[开发说明](开发说明.md)。

The managed controller communicates with Explorer through the native DLL. Symbol downloads and parsing run in a separate helper process. Native implementation fragments retain one translation unit.

## 修改与发布 / Changes and releases

`project.psd1` 集中定义数字版本、发布标签和 DLL 文件名。Explorer 会固定已加载的原生模块，修改原生代码后须递增 DLL 文件名，不能直接覆盖已加载的同名文件。更新脚本会拒绝同名不同内容的 DLL。

Version, release label and native DLL name live in `project.psd1`. Explorer pins loaded native modules; use a new DLL filename after native changes. The update script rejects replacement with different contents under the same name.

发布脚本使用白名单收集源码，只从构建目录取四个运行组件；INI 只来自默认配置。不打包 PDB、机器缓存、个人数据或 Git 历史。两个 ZIP 各含逐文件 SHA256 清单；外部 `SHA256SUMS.txt` 校验 ZIP 本身。

Packaging uses a source allowlist, four built runtime files and neutral default INIs. It excludes PDBs, machine caches, personal data and Git history. Each ZIP includes a file manifest; the outer checksum file covers both archives.

维护者 / Maintainer: **c_zh**（[@zuiyue52](https://github.com/zuiyue52)）。源码 / Source: [TrayFolders](https://github.com/zuiyue52/TrayFolders).
