# 项目来源与第三方声明 / Origins and third-party notices

项目维护者 / Maintainer: **c_zh**。项目代码按 GPL-3.0-only 发布；第三方部分保留原有版权和条款。 / Project code is GPL-3.0-only. Third-party copyrights and terms remain in effect.

## System Tray Folders — Ris Peng

核心代码改编自 [System Tray Folders 1.0.0](https://github.com/ramensoftware/windhawk-mods/blob/a1190b6c1b631a725f6d628189886d410b3760f6/mods/system-tray-folders.wh.cpp)，作者 [Ris Peng / RisPNG](https://github.com/RisPNG)，许可证 GPL-3.0-only。参考版本提交为 `a1190b6c1b631a725f6d628189886d410b3760f6`；原始下载文件 SHA256 为 `F0A5CEDD68E4351699FB179CC1C911C84CABF16EBC0103B3879BCEAEB2818E73`。本地初始快照在忽略换行编码后与该版本文本一致。

The grouping core derives from Ris Peng's System Tray Folders 1.0.0, GPL-3.0-only, at the commit and hash above. The initial source snapshot matches this version after line-ending normalization.

改编包括：移除 Windhawk 运行依赖，增加独立控制与配置、原生生命周期、拖动归属持久化、动态符号解析及界面验证。原模组的归属来自规则，本项目新增拖入文件夹并保存归属。

Changes include removing the Windhawk runtime dependency and adding standalone control/configuration, native lifecycle management, persistent drag assignments, dynamic symbol resolution and UI validation.

## m417z 的托盘模组 / m417z tray mods

原始模组说明部分组件加载与窗口线程辅助代码来自 [m417z](https://github.com/m417z) 的 **Taskbar tray icon spacing and grid**、**Taskbar tray system icon tweaks**，同为 GPL-3.0。保留此间接来源署名；详见核心源码头部。 / The upstream module credits m417z's two named tray mods for module-loading and window-thread helpers. That attribution is retained in the native source header.

## MinHook

[Tsuda Kageyu / MinHook](https://github.com/TsudaKageyu/minhook) 提供原生函数挂钩。源码在 `vendor/minhook`；其 BSD 风格条款及 Vyacheslav Patkov 的 HDE 版权声明见 [MinHook.txt](licenses/MinHook.txt)。 / MinHook provides native hooks. Vendored source and the complete copyright and license notices are included.

## C++/WinRT、LLVM 与 MinGW / Toolchain components

- [Microsoft C++/WinRT](https://github.com/microsoft/cppwinrt)：生成的头文件与支持代码，MIT；[完整条款](licenses/CppWinRT.txt)。 / Generated headers and support code, MIT.
- [LLVM](https://github.com/llvm/llvm-project)：编译器及静态运行库的相关条款见 [LLVM.txt](licenses/LLVM.txt)。 / Compiler/runtime notices.
- [MinGW-w64](https://www.mingw-w64.org/)：运行库及所包含部分的声明见 [MinGW-runtime.txt](licenses/MinGW-runtime.txt)。 / Runtime and incorporated-component notices.

构建工具本身没有随程序打包。Windows、.NET Framework、系统托盘 DLL 和微软 PDB 也不随包分发；它们属于各自提供方。 / Toolchain binaries, Windows/.NET Framework components, system tray DLLs and Microsoft PDBs are not bundled.

## 应用图标 / Application icon

沿用原应用的 Windows/.NET Framework 默认彩色窗口图标，由 `scripts/Generate-Icon.ps1` 从标准 WinForms 窗口提取；不是本项目原创商标。图标来源属于 Microsoft 平台资源，项目 GPL 声明不改变其原有权利。 / The existing colored window icon is extracted from the standard Windows/.NET Framework WinForms window by the included script. It is a platform asset, not an original TrayFolders trademark; the project GPL declaration does not relicense third-party artwork.

## 许可证与源码 / License and source

[LICENSE](LICENSE) 与 [licenses/GPL-3.0.txt](licenses/GPL-3.0.txt) 为 GPL v3 正文；项目采用 GPL-3.0-only，不是“或更高版本”。修改保留署名与许可证。便携包与同版本 `-source.zip` 一起分发，后者包含对应源码和构建脚本；第三方例外按相应声明处理。

The project uses GPL-3.0-only, not “or later.” Preserve attribution and notices. Distribute the portable archive with the matching source archive containing the corresponding source and build scripts. Third-party portions retain the terms identified above.

本项目不是 Microsoft、Windhawk 或上游作者发布的官方版本。 / This is not an official Microsoft, Windhawk or upstream-author release.
