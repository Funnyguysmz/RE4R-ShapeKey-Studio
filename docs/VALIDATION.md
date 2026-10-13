# Validation / 验证状态 — 0.4.2-beta.1

## English

Offline (Windows x64, Windows PowerShell 5.1, public checkout):
- `build_v04.ps1` builds the WebView2 host, bridge DLL and package; `Build.ps1` still builds the 0.3 fallback.
- `scripts/Test.ps1` (0.4): WebView2 UI in demo and host mode with ready handshake; adapter tests (multi-mesh read-back, weight-array allocation, `cap_smooth` command and `C` row, finite-value checks, old-API fallback); receiver tests; bridge named-pipe round trip and disconnect; isolated installer runs (repeat install keeps existing scripts, incompatible scripts rejected before writing).
- Bundled ShapeKey System panel 0.4.4 regression: 60/60.
- `scripts/Check-Release.ps1` rejects zips with non-whitelisted files (model previews, build artefacts) and missing license / version files.

In the real game (user-confirmed, 2026-10-12, with an example character mod):
- Confirmed: desktop ↔ game link-up, sliders and read-back, auto-generated region tabs, body / hair weight-array auto allocation without opening the debug page, capture page rate / state display and responsiveness control with a live phone stream, always-on-top toggle.

No game files are installed or changed by the tests.

## 中文

离线（Windows x64、Windows PowerShell 5.1、公开源码）：
- `build_v04.ps1` 构建 WebView2 外壳、桥接 DLL 和完整包；`Build.ps1` 仍可构建 0.3 回退版。
- `scripts/Test.ps1`（0.4）：演示模式与 host 模式加载界面并完成 ready 握手；适配层测试（多网格回读、权重数组分配、`cap_smooth` 命令与 `C` 行、有限数值检查、旧 API 回退）；接收器测试；桥接命名管道往返与断连；隔离目录安装测试（重复安装保留已有脚本，不兼容脚本写入前拒绝）。
- 随包 ShapeKey System 面板 0.4.4 回归 60/60。
- `scripts/Check-Release.ps1` 会拒绝包含非白名单文件（模型预览图、编译中间产物）或缺少许可 / 版本文件的 zip。

真实游戏（用户确认，2026-10-12，示例角色 MOD）：
- 已确认：桌面与游戏联动、滑条与回读、按分区自动生成 tab、身体 / 头发权重数组自动分配（无需进调试页）、真实手机推流下的面捕速率 / 状态显示与实时度调节、窗口置顶。

测试不会安装或修改游戏文件。

Native conversion table maintenance: isolated clean-install hash check, existing configuration preservation, and rollback after an injected write failure are covered by `tests/install_test.ps1`. This maintenance does not install into the real game.

原生换算表维护：隔离测试覆盖干净安装文件哈希、已有配置保留、注入写入失败后的回滚；本轮不安装到真实游戏。

Historical local candidate, before this public pre-release (2026-10-13): final System 0.4.5-mouth9-test copied from the completed System handoff, with the Studio adapter appended. ZIP-extracted combined script passed legacy60 and mouth9 29 regressions; Studio bridge/UI tests, optional-map/profile preservation, explicit runtime backup and installation rollback passed. Legacy default mapping hash unchanged; old-runtime-plus-new-map negative candidate rejected. No game installation or live mouth9 validation, commit, push or publication was performed.

## Public pre-release 0.4.2-beta.1

Bundled System: 0.4.5-rc.1, source commit 55fe33499a91065a309d1626073e34610826148b. Studio appends its own bridge block. Mapping contracts remain48 rules for legacy Leon and19 for mouth9. Both public mappings are sanitized; existing installed maps are preserved.

The public test entry includes System's synthetic legacy60/mouth9 29 tests against the composed runtime, Studio adapter/receiver/pipe/UI tests, clean installation, profiles/settings/maps preservation, explicit runtime upgrade backups, actual restoration from backup, and failure rollback. Synthetic profiles are never activated by the installer. Final ZIP is read back and checked against source.

Evidence scope: standalone85-channel normal experiment accepted by user; integrated94-channel/nine-input signal binding and eye/mouth behaviour still await separate game feedback. No universal model or expression-normal-mixing acceptance is claimed. Exporter v0.70.6-re4r.1 is separately GPLv3, verified public; Studio remains MIT.
