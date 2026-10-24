# 使用手册

[项目首页](../README.md) · [English](USAGE.en.md) · [文档导航](README.md)

## 使用哪个包

| 文件 | 用途 |
| --- | --- |
| [TrayFolders-0.4.2-preview.1-win-x64.zip](https://github.com/zuiyue52/TrayFolders-Public/releases/download/v0.4.2-preview.1/TrayFolders-0.4.2-preview.1-win-x64.zip) | 完整便携包，解压后运行 |
| [TrayFolders-0.4.2-preview.1-source.zip](https://github.com/zuiyue52/TrayFolders-Public/releases/download/v0.4.2-preview.1/TrayFolders-0.4.2-preview.1-source.zip) | 对应源码、测试、构建脚本和第三方源码 |
| [SHA256SUMS.txt](https://github.com/zuiyue52/TrayFolders-Public/releases/download/v0.4.2-preview.1/SHA256SUMS.txt) | 两个 ZIP 的 SHA256 校验值 |

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

手动选择或拖动的归属优先于文字规则；同名程序的多个图标会共享手动归属。规则忽略大小写，按文件夹顺序分配。更多例子见[匹配规则示例](匹配规则示例.md)。

“显示符号”留空时显示成员图标预览；可填写短文字、emoji 或 `U+E7FC` 这样的字符码，显示效果取决于字体。

关闭设置窗口后仍在托盘运行，双击彩色方块图标重新打开。使用托盘菜单“退出并恢复托盘”退出。使用前停用 Windhawk 的同类分组模组，同一时间只运行一份 TrayFolders。

## 自启动、升级与卸载

在“设置”页勾选“开机自启动（登录 Windows 后）”及“自启动时静默运行”，点击“保存启动设置”。静默模式不显示主窗口，自启动仅针对当前 Windows 用户。

升级时退出旧版，将新包解压到新目录，按需复制旧目录的 `settings.ini` 和 `assignments.ini`。不要复制旧 DLL、`compatibility.bin` 或临时文件。移动位置后重新保存启动设置，更新路径。

卸载前取消自启动并保存，通过托盘菜单退出，再删除程序目录。符号缓存 `%LOCALAPPDATA%\TrayFolders\Symbols` 可在退出后手动删除，下次使用可能需要重新下载。

## 数据与联网

配置、图标名单与适配数据保存在程序目录。解析器通过 HTTPS 从微软符号服务下载匹配的 PDB，并使用本地缓存。代码没有分析统计或上传托盘名单的功能。日志和截图可能包含程序名、悬停内容或个人路径，分享前请检查。
