<div align="center">

<img src="assets/tray-folders.png" width="40" height="40" alt="TrayFolders 图标">

# 托盘文件夹 · TrayFolders

**为 Windows 11 隐藏托盘图标添加文件夹。**

拖动归类 · 选择加入 · 名称规则 · 独立运行

[**下载 Windows x64 便携包**](https://github.com/zuiyue52/TrayFolders/releases/download/v0.4.2-preview.1/TrayFolders-0.4.2-preview.1-win-x64.zip) · [English](README.en.md) · [使用文档](docs/README.md) · [反馈问题](https://github.com/zuiyue52/TrayFolders/issues/new/choose)

[![版本：0.4.2-preview.1](https://img.shields.io/badge/version-0.4.2--preview.1-orange)](https://github.com/zuiyue52/TrayFolders/releases/tag/v0.4.2-preview.1) [![平台：Windows 11 x64](https://img.shields.io/badge/platform-Windows_11_x64-0078D4)](docs/COMPATIBILITY.md) [![代码许可证：GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue)](LICENSE)

</div>

TrayFolders 在右下角 **“显示隐藏的图标”** 面板中整理程序图标。新建文件夹后，可以直接拖入图标，也可以从名单选择或填写匹配规则。程序独立运行，无需安装 Windhawk。

## 收纳效果

| 未分组：图标逐个排列 | 收纳后：文件夹显示成员预览 |
| :---: | :---: |
| <img src="docs/images/tray-ungrouped.png" width="293" alt="未分组的 Windows 隐藏图标面板，微信、ChatGPT 和 Everything 等图标单独排列"> | <img src="docs/images/tray-grouped.png" width="293" alt="收纳后的 Windows 隐藏图标面板，左下角文件夹预览微信、ChatGPT 和 Everything 三个成员图标"> |

**右图左下角就是收纳后的文件夹**，可看到微信、ChatGPT 和 Everything 的缩略图标。点击文件夹可展开成员；把其他图标拖到文件夹上即可加入。

*以上为早期实机测试截图，拍摄时间不同，应用数量、主题和程序图标可能与当前版本不同。右图红框是当时的程序图标标记。*

## 下载

| 下载内容 | 适合谁 |
| --- | --- |
| [**Windows x64 便携包**](https://github.com/zuiyue52/TrayFolders/releases/download/v0.4.2-preview.1/TrayFolders-0.4.2-preview.1-win-x64.zip) | 直接使用；完整解压后运行 `TrayFolders.exe` |
| [对应版本源码包](https://github.com/zuiyue52/TrayFolders/releases/download/v0.4.2-preview.1/TrayFolders-0.4.2-preview.1-source.zip) | 查看源码、修改或自行编译 |
| [SHA256 校验文件](https://github.com/zuiyue52/TrayFolders/releases/download/v0.4.2-preview.1/SHA256SUMS.txt) | 校验下载文件的完整性 |

当前版本：**0.4.2-preview.1（预览版）**。[查看发布说明](https://github.com/zuiyue52/TrayFolders/releases/tag/v0.4.2-preview.1)

需要 **Windows 11 x64、.NET Framework 4.8** 和可写目录。首次适配可能需要联网获取微软符号。应用界面为中文，主要使用及开发文档提供中英双语。

## 快速开始

1. **完整解压便携包**，运行 `TrayFolders.exe`，保留同目录内的 DLL、配置和辅助程序。
2. 在“文件夹管理”中点击 **＋ 新建**，填写名称并 **保存并应用**。
3. 点击 **启用分组**，等待准备完成，再展开一次 Windows 隐藏图标面板。
4. 将图标拖到文件夹上，或用 **＋ 添加图标…** 选择加入。

新安装从空白配置开始。若正在使用旧版，请先从托盘菜单退出；同一时间只运行一份分组程序。关闭设置窗口后仍会在托盘运行，彻底退出请使用 **“退出并恢复托盘”**。

[完整使用手册](docs/USAGE.zh-CN.md) · [自启动、迁移与卸载](docs/USAGE.zh-CN.md#自启动升级与卸载) · [常见问题](docs/COMPATIBILITY.md)

## 功能一览

| 功能 | 使用方式 |
| --- | --- |
| 文件夹管理 | 新建、重命名、排序或删除，支持成员预览与自定义符号 |
| 拖动归类 | 拖到文件夹即可加入；在文件夹内拖到返回箭头可移出 |
| 图标选择与规则 | 从当前图标名单多选，或使用 `*`、`?` 匹配名称 |
| 刷新与同步 | 拖动后自动更新名单和数量，也可手动刷新 |
| 启动与显示 | 可选登录自启动和静默运行，支持 DPI 缩放及小窗口滚动 |

### 设置界面

<p align="center">
  <img src="docs/images/folders.png" width="760" alt="托盘文件夹设置界面：左侧管理文件夹，右侧选择成员并填写匹配规则">
</p>

*截图使用测试数据。实际图标名单由当前系统读取。*

## 兼容范围

本项目使用 Explorer 内部接口。符号定位可适应部分组件变化，但**不保证兼容所有 Windows 11 版本**。当前为预览版；ARM64、32 位系统和 Windows 10 不在支持范围内。

已验证的组件、尚未完成验证的版本及 DLL 错误处理见[兼容性说明](docs/COMPATIBILITY.md)，测试范围见[验证记录](docs/VALIDATION.md)。

## 文档与开发

| 想了解什么 | 入口 |
| --- | --- |
| 使用方法、图标规则、数据位置 | [中文使用手册](docs/USAGE.zh-CN.md) · [English guide](docs/USAGE.en.md) |
| 所有文档 | [文档导航](docs/README.md) |
| 构建、测试与代码结构 | [开发指南](docs/BUILD.md) |
| 历次变化 | [版本记录](CHANGELOG.md) |
| 报告问题、提出建议或贡献代码 | [反馈入口](https://github.com/zuiyue52/TrayFolders/issues/new/choose) · [参与维护](CONTRIBUTING.md) |

## 来源与许可证

分组核心改编自 **[Ris Peng / RisPNG](https://github.com/RisPNG)** 的 [System Tray Folders 1.0.0](https://github.com/ramensoftware/windhawk-mods/blob/a1190b6c1b631a725f6d628189886d410b3760f6/mods/system-tray-folders.wh.cpp)，保留原项目对 **[m417z](https://github.com/m417z)** 托盘模组的署名；原生挂钩使用 **[MinHook](https://github.com/TsudaKageyu/minhook)**。

本项目扩展了独立控制程序、配置界面、拖动归属持久化、自启动和动态适配。它不是 Microsoft、Windhawk 或上游作者发布的官方版本。

项目代码采用 **[GPL-3.0-only](LICENSE)**，第三方部分保留各自条款。分发程序时请同时提供对应源码和声明。[完整来源与第三方声明](THIRD-PARTY-NOTICES.md)

维护者：**c_zh** · [@zuiyue52](https://github.com/zuiyue52)
