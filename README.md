# 托盘文件夹 / TrayFolders

[English](README.en.md) · [下载 / Downloads](https://github.com/zuiyue52/TrayFolders-Public/releases/tag/v0.4.2-preview.1) · **0.4.2-preview.1** · Windows 11 x64  
维护者：**c_zh** · 项目代码许可证：**GPL-3.0-only**

在 Windows 11 右下角“显示隐藏的图标”面板中创建文件夹，整理后台程序图标。可以拖动加入文件夹、从当前图标名单中选择，或填写名称匹配规则。程序独立运行，不需要安装 Windhawk。

这是社区维护的预览版，使用 Explorer 内部接口，不是微软官方功能。动态符号定位可以适应部分组件变化，但不保证兼容所有 Windows 11 版本。

![文件夹管理界面，使用测试数据](docs/images/folders.png)

## 功能

- 新建、删除、重命名和排序文件夹，显示图标预览或自定义符号。
- 从当前图标名单中多选加入，或用 `*`、`?` 编写名称规则。
- 拖入文件夹保存归属，拖到文件夹里的返回箭头可移出。
- 拖动后自动更新设置页名单与数量，也可手动刷新。
- 在设置页配置当前用户登录自启动和静默启动。
- 随显示器 DPI 缩放；小窗口可滚动，四个页面及图标选择窗口均有布局检查。

## 使用哪个包

| 文件 | 用途 |
| --- | --- |
| `TrayFolders-0.4.2-preview.1-win-x64.zip` | 完整便携包，解压后运行 |
| `TrayFolders-0.4.2-preview.1-source.zip` | 对应源码、测试、构建脚本和第三方源码 |
| `SHA256SUMS.txt` | 两个 ZIP 的 SHA256 校验值 |

需要 **Windows 11 x64、.NET Framework 4.8** 和可写的程序目录。首次适配可能需要访问微软符号服务器。当前应用界面为中文，主要文档提供中英双语。ARM64、32 位系统和 Windows 10 不在本预览版支持范围内。

**完整解压便携包后运行 `TrayFolders.exe`。不要从 ZIP 内直接运行，也不要只复制 EXE。**

```text
TrayFolders.exe
TrayFolders.exe.config
TrayFolders.Native.v5.dll
TrayFolders.Symbols.exe
settings.ini
assignments.ini
```

前四项是必要运行文件，必须保持在同一目录。两个 INI 保存文件夹和手动归属；发布包提供空白配置。首次使用不会自动添加个人规则，也不会自动注册开机启动。

## 第一次使用

1. 在“文件夹管理”点击“＋ 新建”，填写名称，点击“保存并应用”。
2. 点击“启用分组”，等待“系统适配”准备完成，然后展开一次 Windows 隐藏图标面板。
3. 选中文件夹，点击“＋ 添加图标…”多选加入；也可直接在隐藏面板里拖入。
4. 手动规则每行一条，例如 `*Everything*`、`*微信*`，修改后保存并应用。
5. “刷新图标”重新读取托盘快照，更新名单和数量，不会重启程序或清空规则。名单为空时先展开隐藏面板再刷新。

手动选择或拖动的归属优先于文字规则；同名程序的多个图标会共享手动归属。规则忽略大小写，按文件夹顺序分配。更多例子见[匹配规则示例](docs/匹配规则示例.md)。

“显示符号”留空时显示成员图标预览；可填写短文字、emoji 或 `U+E7FC` 这样的字符码，显示效果取决于字体。

关闭设置窗口后仍在托盘运行，双击彩色方块图标重新打开。使用托盘菜单“退出并恢复托盘”退出。使用前停用 Windhawk 的同类分组模组，同一时间只运行一份 TrayFolders。

## 自启动、升级与卸载

在“设置”页勾选“开机自启动（登录 Windows 后）”及“自启动时静默运行”，点击“保存启动设置”。静默模式不显示主窗口，自启动仅针对当前 Windows 用户。

升级时退出旧版，将新包解压到新目录，按需复制旧目录的 `settings.ini` 和 `assignments.ini`。不要复制旧 DLL、`compatibility.bin` 或临时文件。移动位置后重新保存启动设置，更新路径。

卸载前取消自启动并保存，通过托盘菜单退出，再删除程序目录。符号缓存 `%LOCALAPPDATA%\TrayFolders\Symbols` 可在退出后手动删除，下次使用可能需要重新下载。

## 数据与联网

配置、图标名单与适配数据保存在程序目录。解析器通过 HTTPS 从微软符号服务下载匹配的 PDB，并使用本地缓存。代码没有分析统计或上传托盘名单的功能。日志和截图可能包含程序名、悬停内容或个人路径，分享前请检查。

## 开发与来源

- [构建、测试和架构（中英）](docs/BUILD.md)
- [兼容性、DLL 错误与常见问题（中英）](docs/COMPATIBILITY.md)
- [版本记录（中英）](CHANGELOG.md)
- [参考项目与第三方声明（中英）](THIRD-PARTY-NOTICES.md)
- [参与维护（中英）](CONTRIBUTING.md)

分组核心改编自 **Ris Peng** 的 [System Tray Folders 1.0.0](https://github.com/ramensoftware/windhawk-mods/blob/a1190b6c1b631a725f6d628189886d410b3760f6/mods/system-tray-folders.wh.cpp)，保留原项目对 **m417z** 托盘模组的署名。挂钩使用 [MinHook](https://github.com/TsudaKageyu/minhook)。本项目扩展了独立控制程序、可视化配置、拖动归属、自启动、动态适配和屏幕布局处理，与上游没有官方隶属关系。

代码按 [GPL-3.0-only](LICENSE) 提供，第三方部分保留其许可证。分发便携包时同时提供同版本源码包和声明。源码仓库：[https://github.com/zuiyue52/TrayFolders-Public](https://github.com/zuiyue52/TrayFolders-Public)；反馈问题请使用 [Issues](https://github.com/zuiyue52/TrayFolders-Public/issues)。
