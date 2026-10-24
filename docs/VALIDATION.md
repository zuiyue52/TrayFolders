# 发布验证 / Release validation

版本 / Release: **0.4.2-preview.1**  
日期 / Date: **2026-10-24**

- C# 主程序、原生 DLL 与符号解析器从源码完整构建成功。 / Full build of the managed host, native DLL and symbol resolver succeeded.
- 独立功能测试通过：配置、匹配与手动归属、文件夹操作、刷新保留编辑、启动设置、静默启动和嵌入图标。 / Isolated functional tests passed for configuration, membership, folder actions, refresh/edit preservation, startup, silent launch and embedded icon.
- 77 项布局场景检查通过，包括四个页面与选择弹窗、多种模拟尺寸、100%～200% 缩放及缩放往返。 / 77 layout checks passed across pages, picker, simulated sizes, 100%–200% scaling and DPI round trips.
- 完整便携包原生加载检查通过；刻意缺失 DLL 和无效二进制能返回明确错误；缺失 DLL 的命令行清理不再再次抛异常。 / Native-load preflight passed; missing/invalid DLL cases produced diagnostics and CLI cleanup completed normally.
- 更新测试保留个人配置并拒绝同名不同内容的原生文件；两个 ZIP 的文件清单、默认配置与哈希验证通过。 / Update tests preserved settings and rejected conflicting same-name native files. Archive contents, neutral defaults and manifests passed.
- 重新构建的原生 DLL 的代码段与之前验证的 v5 一致，整体文件哈希可因链接元数据不同而变化。 / The rebuilt DLL code section matches the previously validated v5; whole-file hashes may differ due to linker metadata.
- 本轮未替换正在使用的应用，也未新增其他 Windows 版本的实际托盘验证。已知范围见 [COMPATIBILITY.md](COMPATIBILITY.md)。 / This release preparation did not replace the running installation or add full-system validation on other Windows versions.

文档截图使用隔离测试数据，并非个人托盘名单。运行日志与原始测试输出留在构建目录，不包含在公开包中。 / Screenshots use isolated fixture data. Raw logs and test outputs remain in build directories and are excluded from public packages.
