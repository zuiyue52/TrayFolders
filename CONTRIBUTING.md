# 参与维护 / Contributing

维护者 / Maintainer: **c_zh**（[@zuiyue52](https://github.com/zuiyue52)）。欢迎通过 [问题与建议 / Issues](https://github.com/zuiyue52/TrayFolders/issues/new/choose) 反馈问题，或提交 [Pull Requests](https://github.com/zuiyue52/TrayFolders/pulls)。 / Please use the repository Issues and Pull Requests for reports and contributions.

反馈问题请包含：Windows build、架构、组件版本、重现步骤、预期和实际表现、缩放/分辨率，以及去除个人信息后的日志。 / Include OS build, architecture, component version, reproduction steps, expected/actual behavior, scaling/resolution and sanitized logs.

修改请聚焦单个问题，保留上游署名，运行 [BUILD.md](docs/BUILD.md) 中的测试并说明结果。原生改动需递增 DLL 文件名，并实际验证启用、展开、拖放、停用及重连。 / Keep changes focused, retain attribution and run the documented tests. Native changes require a new DLL filename and real Explorer regression.

不要提交个人 INI、图标名单、PDB、适配缓存、日志或生成的二进制。仅跟踪 `config/defaults` 中的默认配置。 / Do not commit personal INIs, inventory, PDBs, caches, logs or generated binaries. Track neutral defaults only.
