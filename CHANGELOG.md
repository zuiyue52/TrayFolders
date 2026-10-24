# 版本记录 / Changelog

## 0.4.2-preview.1 — 2026-10-24

- 首个整理后的公开预览版；源码和便携包分别打包，附带校验值。 / First prepared public preview; separate source and portable archives with checksums.
- 中英双语使用、构建和兼容说明，补齐来源与许可证。 / Bilingual usage, build and compatibility documentation, with attribution and licenses.
- 空白默认配置，不带个人图标名单、日志或机器缓存。 / Neutral defaults; no private inventory, logs or machine caches.
- 启用前检查必要文件和 DLL 加载；新增 `--check-files`。修复加载失败后连接清理再次抛异常的问题。 / Check runtime files and DLL loading before activation; add `--check-files`; make cleanup safe after failed loading.
- 沿用 v5 原生接口；未宣称额外 Windows 版本已通过实机测试。 / Retain the v5 interface; no additional Windows versions claimed as fully tested.

## 0.4.1 — 开发版 / Development build

统一四个页面与图标选择窗的 DPI、按钮大小、列宽及滚动布局。 / DPI, text-sized buttons, adaptive columns and scrolling across all pages and the picker.

## 0.4.0 — 开发版 / Development build

识别托盘组件、解析微软符号、选择适配方案并验证运行时结构。 / Component identification, Microsoft symbol resolution, adapter selection and runtime validation.

## 0.3.x 及以前 / Earlier development

独立程序、拖放归属、图标选择、自启动设置与源码分层。 / Standalone controller, drag assignments, inventory selection, startup settings and source organization.
