Maintenance release: **0.4.2-beta.1** — pre-release with optional nine-input model mapping and safe runtime upgrades.

维护版本：**0.4.2-beta.1** — 预发布：可选九输入映射与安全运行时升级，保留用户数据。

# RE4R ShapeKey Studio

>

[English](#english) | [中文](#中文)

---

## English

### What it is

A Windows desktop workbench for the custom shape keys of a Resident Evil 4 Remake character mod. It talks to the game through a small REFramework bridge DLL (named pipe on the local machine) and the in-game panel [RE4R-ShapeKey-System](https://github.com/Funnyguysmz/RE4R-ShapeKey-System). It can also run the RhyLive receiver so a phone can drive expressions (ARKit 52).

Studio 0.4 features:

- Left navigation **generated from the character mod's profile**: built-in pages for expression, face shaping, body and outfit, hair and ears, plus **an automatic extra page for every other region** the mod declares (outfit variants, eye details, mouth details, nails, or any custom region). Empty pages are hidden. See the region table in [PROFILE_FORMAT.md](https://github.com/Funnyguysmz/RE4R-ShapeKey-System/blob/main/docs/PROFILE_FORMAT.md).
- Sliders per key with row reset, page reset, copy / paste of values, presets (save / load), neutral comparison, per-key inspection sweep.
- Per-mesh weight-array status in the diagnostics page; body and hair meshes are allocated automatically (uses the panel's `allocate_weights`).
- **Face-capture page**: face frame rate and packet rate, a 60-second frame-rate curve, a status pill (not started / waiting for phone / receiving / interrupted), packet age, and a **responsiveness** control (0.05-1; larger follows faster, smaller is smoother) backed by the panel's `set_cap_smooth` (needs ShapeKey System 0.4.4 and the matching bridge update).
- Dark / light theme, Chinese / English, an always-on-top pin button (Ctrl+T).
- GUI installer: validates the game folder, installs or updates the panel, the RhyLive receiver and the bridge. Existing scripts are kept (only the adaptation block is updated), backups go to `backups/` beside the program, the log is `installer.log`.

### How the three repositories fit together

```
 phone (RhyLive app, ARKit 52, UDP)
        |
        v
 RE4R-RhyLive-Face      receiver DLL (rhylive_native) + face-bone Lua panel
        |  rhylive_native.poll()
        v
 RE4R-ShapeKey-System   in-game panel, writes shape-key weights
        ^
        |  RE4R_ShapeKeySystem_api  (Lua)  <->  ShapeKeyBridge.dll  <->  named pipe
 RE4R-ShapeKey-Studio   <- THIS REPO: desktop app + bridge DLL + installer
```

The desktop app talks to the game over a local named pipe (no network port). The phone talks to the PC over UDP (default port 9000).

### Requirements

- Resident Evil 4 Remake with a REFramework build that works with your game version.
- A character mod that ships custom shape keys **and** the profile `reframework/data/RE4R_ShapeKeySystem_profile.json` (this tool contains no models; model authoring uses the exporter toolchain described below; this repository does not bundle model assets).
- Windows 10 / 11 x64 and the **Microsoft Edge WebView2 Runtime** (included with Windows 11 and with Edge; 0.4 only). **No .NET and no Visual C++ redistributable are needed.**

### Install and first run (players)

1. Unpack the Studio package into a writable folder and run `RE4R_ShapeKeyStudio.exe`.
2. On the setup page choose the folder that directly contains `re4.exe` and click Verify. The control pages stay locked until the folder passes.
3. **Quit the game**, then click "Install / update bridge and face capture". The shape-key panel, the RhyLive plugin and the bridge are installed; existing scripts are kept, backups go to `backups/`.
4. Start the game, load a save, then in Studio open the workbench and click "Start control" (the model must be connected).
5. For face capture: on the phone enter the PC's LAN IPv4 shown on the setup page and port 9000; start the receiver in Studio, confirm the frame counter grows, choose source "RhyLive + fine tuning" or "capture / native fall-back". This route turns off the old bone driver to avoid conflicts.

The game folder is stored in `Studio.settings.ini` beside the program and re-verified at start; no Steam path is hard-coded. Keys whose module is not enabled in the character mod (body / hair add-ons) are disabled. `RE4R_ShapeKeyStudio.exe --demo` shows the interface offline: nothing is written to the game and nothing is installed.

Troubleshooting: if installing fails, quit the game and read `installer.log`; if sliders are grey, check that control has been started and that the mod (including body / hair add-ons) is enabled in your mod manager.

### Build from source

Windows x64 with Windows PowerShell 5.1+, [Zig](https://ziglang.org/), the Lua 5.4 C sources, and Microsoft's `Microsoft.Web.WebView2` **1.0.4258.31** NuGet SDK (download the `.nupkg` and extract it; see [third_party/webview2/README.md](third_party/webview2/README.md)). Nothing is downloaded by the build.

```powershell
.\build_v04.ps1 -Zig 'C:\tools\zig\zig.exe' -LuaSourceDirectory 'C:\tools\lua-5.4\src' -WebView2Directory 'C:\tools\webview2-1.0.4258.31'
```

Output: `build/v04/package` (`RE4R_ShapeKeyStudio.exe`, `WebView2Loader.dll`, `ui/`, bridge DLL, runtime scripts, installer, license files, `VERSION.txt`). Debug symbols go to `build/v04/symbols`. Before publishing a zip, run `.\scripts\Check-Release.ps1 -ZipPath <zip>` (whitelist / license / version check).

The native Win32 **0.3** UI is kept as a fallback: `.\Build.ps1 -Zig ... -LuaSourceDirectory ...` → `build/package`.

Development flags of the 0.4 host: `--demo` (offline), `--selftest` (loads the UI and waits for its ready message); environment variable `STUDIO_DEVTOOLS=1` enables WebView2 developer tools. The 0.4 interface can also be opened in a normal browser with `?demo=1` for design work.

### Testing

```powershell
.\scripts\Test.ps1              # 0.4 (default)
.\scripts\Test.ps1 -UiVersion '0.3'
```

For 0.4 it also loads the WebView2 UI in demo and host mode and checks the ready handshake (needs the WebView2 Runtime). Runs the Lua adapter and receiver tests (stub-based), the native pipe test (bridge DLL against a desktop client on a private pipe), the GUI setup smoke test, and an installer test in an isolated folder (repeat install keeps existing scripts, incompatible scripts are rejected before writing). Passing means "offline tests pass"; **it does not prove behaviour in the real game.**

### Verified status and limitations

| Item | Status |
|---|---|
| 0.3.0 offline tests (compile, bridge / Lua / pipe, multi-mesh limits, model-change and disconnect recovery, setup-folder gate, port / IP validation, installer re-install) | passed locally |
| 0.3.0 in real RE4 (GUI link-up, capture, taskbar icon) | **not confirmed** |
| 0.4 WebView2 interface | public build passes on Windows PowerShell 5.1; `Test.ps1` passes |
| 0.4 in real RE4: link-up, sliders, auto-generated tabs, body / hair weight-array auto allocation, capture rate / state display, responsiveness control, always-on-top | **confirmed in game by the user** with an example character mod |
| In-game effect of a shape-key model | needs separate verification; the GUI's static thumbnails are not a live preview |
| Body / hair keys | need the character mod's matching modules; unbound keys stay disabled |

### License

License: MIT. See [LICENSE](LICENSE). Third-party components retain their original licenses.

### Third-party notices

- REFramework `API.h` (MIT, praydog) — `vendor/REFramework_LICENSE`.
- RhyLive-related code — `vendor/RhyLive_LICENSE`.
- Lua 5.4 (MIT, Lua.org, PUC-Rio) is compiled into the DLLs; the build writes `Lua_LICENSE.txt` into the package.
- Studio 0.4 additionally redistributes Microsoft's `WebView2Loader.dll` (Microsoft Edge WebView2 SDK).

Directory layout: `src/` desktop and bridge code, `runtime/` in-game Lua and capture modules, `tests/` stub tests, `assets/` icons and offline channel data.

---

## 中文

### 这是什么

生化危机4重制版角色 MOD 自定义形态键的 Windows 桌面工作台。它通过一个小型 REFramework 桥接 DLL（本机命名管道）和游戏内面板 [RE4R-ShapeKey-System](https://github.com/Funnyguysmz/RE4R-ShapeKey-System) 与游戏通信，也可以运行 RhyLive 接收端，用手机（ARKit 52）驱动表情。

Studio 0.4 功能：

- 左侧导航**由角色 MOD 的配置自动生成**：内置表情、捏脸、身体与服装、头发与耳朵四类页面，MOD 声明的**其他每个区域会自动各成一页**（服装变体、眼部细节、口部细节、指甲或任何自定义区域）。没有键的页面自动隐藏。区域对照表见 [PROFILE_FORMAT.md](https://github.com/Funnyguysmz/RE4R-ShapeKey-System/blob/main/docs/PROFILE_FORMAT.md)。
- 每个键一条滑条，单行重置、整页重置、复制 / 粘贴参数、预设保存与载入、中性对比、逐键巡检。
- 诊断页显示每个网格的权重数组状态；身体和头发网格自动分配（调用面板的 `allocate_weights`）。
- **面捕页**：面部帧率和数据包速率、最近 60 秒帧率曲线、状态胶囊（未启动 / 等待手机数据 / 接收中 / 已中断）、数据间隔，以及基于面板 `set_cap_smooth` 的**实时度**调节（0.05–1，越大越跟手，越小越平滑；需要 ShapeKey System 0.4.4 和对应的桥接更新）。
- 深色 / 浅色主题、中文 / English、窗口置顶图钉按钮（Ctrl+T）。
- GUI 安装器：验证游戏目录，安装或更新面板、RhyLive 接收插件和桥接。已有脚本保留（只更新适配段），备份在程序旁 `backups/`，日志为 `installer.log`。

### 三个仓库如何配合

```
 手机（RhyLive，ARKit 52，UDP）
        |
        v
 RE4R-RhyLive-Face      接收 DLL（rhylive_native）+ 面部骨骼 Lua 面板
        |  rhylive_native.poll()
        v
 RE4R-ShapeKey-System   游戏内面板，写入形态键权重
        ^
        |  RE4R_ShapeKeySystem_api（Lua） <-> ShapeKeyBridge.dll <-> 命名管道
 RE4R-ShapeKey-Studio   <- 本仓库：桌面程序 + 桥接 DLL + 安装器
```

桌面程序与游戏之间用本机命名管道（不开网络端口）；手机与电脑之间用 UDP（默认端口 9000）。

### 需求

- 与你的游戏版本匹配的 REFramework。
- 带自定义形态键、并附带 `reframework/data/RE4R_ShapeKeySystem_profile.json` 的角色 MOD（本工具不含任何模型；模型制作需要下述导出器工具链，本仓库不附带模型资产）。
- Windows 10 / 11 x64 与 **Microsoft Edge WebView2 运行时**（Windows 11 和 Edge 自带；仅 0.4 需要）。**不需要 .NET，也不需要 Visual C++ 运行库。**

### 玩家安装与首次使用

1. 把 Studio 包解压到可写文件夹，运行 `RE4R_ShapeKeyStudio.exe`。
2. 在初始化页选择直接包含 `re4.exe` 的文件夹并点击验证。验证通过前控制页保持锁定。
3. **退出游戏**，点击“安装 / 更新桥接与面捕”。会安装形态键面板、RhyLive 插件和桥接；已有脚本保留，备份在 `backups/`。
4. 启动游戏、读取存档，在 Studio 进入工作台，点“开始控制”（需要模型已连接）。
5. 面捕：手机里填初始化页显示的电脑局域网 IPv4 和端口 9000；在 Studio 启动接收，确认帧计数在增长，来源选“RhyLive + 微调”或“面捕 / 原生回落”。此路线会关闭旧的骨骼驱动以避免冲突。

游戏目录保存在程序旁的 `Studio.settings.ini`，启动时重新验证，不硬编码任何 Steam 路径。角色 MOD 里没启用对应模块（身体 / 头发 add-on）的键会被禁用。`RE4R_ShapeKeyStudio.exe --demo` 可离线查看界面，不写游戏也不安装任何东西。

排查：安装失败时先退出游戏，查看 `installer.log`；滑条灰色时确认已“开始控制”，并且角色 MOD（含身体 / 头发 add-on）在你的 MOD 管理器里已启用。

### 从源码构建

Windows x64 + Windows PowerShell 5.1 及以上、[Zig](https://ziglang.org/)、Lua 5.4 C 源码，以及微软 `Microsoft.Web.WebView2` **1.0.4258.31** NuGet SDK（下载 `.nupkg` 解压即可，见 [third_party/webview2/README.md](third_party/webview2/README.md)）。构建过程不联网下载。

```powershell
.\build_v04.ps1 -Zig 'C:\tools\zig\zig.exe' -LuaSourceDirectory 'C:\tools\lua-5.4\src' -WebView2Directory 'C:\tools\webview2-1.0.4258.31'
```

输出 `build/v04/package`（`RE4R_ShapeKeyStudio.exe`、`WebView2Loader.dll`、`ui/`、桥接 DLL、运行脚本、安装器、许可文件、`VERSION.txt`），调试符号在 `build/v04/symbols`。发布 zip 前运行 `.\scripts\Check-Release.ps1 -ZipPath <zip>` 做白名单 / 许可 / 版本检查。

原生 Win32 的 **0.3** 界面保留为回退：`.\Build.ps1 -Zig ... -LuaSourceDirectory ...` → `build/package`。

0.4 外壳的开发参数：`--demo`（离线）、`--selftest`（加载界面并等待其 ready 消息）；环境变量 `STUDIO_DEVTOOLS=1` 开启 WebView2 开发者工具。0.4 界面也可以在普通浏览器里用 `?demo=1` 打开做设计调试。

### 测试

```powershell
.\scripts\Test.ps1              # 0.4（默认）
.\scripts\Test.ps1 -UiVersion '0.3'
```

0.4 还会在演示模式和 host 模式下加载 WebView2 界面并检查 ready 握手（需要 WebView2 Runtime）。依次运行 Lua 适配层与接收器测试（基于桩）、原生管道测试（桥接 DLL 对桌面客户端，使用独立管道）、GUI 初始化冒烟测试，以及隔离目录里的安装器测试（重复安装保留已有脚本，不兼容脚本在写入前被拒绝）。通过只代表“离线测试通过”，**不能证明真实游戏里的行为。**

### 验证状态与限制

| 项目 | 状态 |
|---|---|
| 0.3.0 离线测试（编译、桥接 / Lua / 管道、多网格上限、模型变化与断连恢复、初始化目录门禁、端口 / IP 校验、安装器重复安装） | 本地通过 |
| 0.3.0 在真实 RE4 中（GUI 联动、面捕、任务栏图标） | **未确认** |
| 0.4 WebView2 界面 | 公开源码在 Windows PowerShell 5.1 下构建通过；`Test.ps1` 通过 |
| 0.4 在真实 RE4 中：联动、滑条、自动分区、身体 / 头发权重数组自动分配、面捕速率 / 状态显示、实时度调节、窗口置顶 | 用户已用示例角色 MOD **实机确认** |
| 带形态键模型的游戏内效果 | 需要另行验证；GUI 的静态缩略图不是实时预览 |
| 身体 / 头发键 | 需要角色 MOD 的对应模块；未绑定的键保持禁用 |

### 许可

License: MIT。见 [LICENSE](LICENSE)；第三方组件保留各自原许可证。

### 第三方声明

- REFramework `API.h`（MIT，praydog）— `vendor/REFramework_LICENSE`。
- RhyLive 相关代码 — `vendor/RhyLive_LICENSE`。
- Lua 5.4（MIT，Lua.org, PUC-Rio）编译进 DLL；构建会把 `Lua_LICENSE.txt` 写入包内。
- Studio 0.4 还会再分发微软的 `WebView2Loader.dll`（Microsoft Edge WebView2 SDK）。

目录：`src/` 桌面与桥接代码，`runtime/` 游戏内 Lua 与面捕模块，`tests/` 桩测试，`assets/` 图标与离线通道数据。

Native animation conversion: the package includes `reframework/data/RE4R_ShapeKeySystem_native_leon.json`. Clean installations install it; an existing file is preserved with a message. `json.load_file` in REFramework reads this filename under `reframework/data`.

原生动画换算：包内包含 `reframework/data/RE4R_ShapeKeySystem_native_leon.json`；干净安装会写入，已有文件会提示并保留。REFramework 的 `json.load_file` 从 `reframework/data` 读取此文件名。



### 0.4.2-beta.1 toolchain and upgrade / 工具链与升级

The public [GPLv3 exporter v0.70.6-re4r.1](https://github.com/Funnyguysmz/RE-Mesh-Editor-BlendShape/releases/tag/v0.70.6-re4r.1) provides optional nine zero-delta inputs and packed/base-normal/no-normalRecalc output. Base cylindrical normals, Head binding and a model-specific profile still require authoring. Toon normals are not necessarily overwritten; per-expression normal mixing and the integrated94-channel combination are not universally verified. See [toolchain and recovery guide](docs/TOOLCHAIN.md).

九输入与保留基础三渲二法线的导出路线已有公开预发布；圆柱基础法线、Head绑定和模型profile仍需制作。现有91/NoTap与用户数据保留，普通安装保留脚本主体，显式升级会备份；详见[工具链与回退说明](docs/TOOLCHAIN.md)。新集成94通道组合仍待独立实机反馈。

Bundled runtime: [System 0.4.5-rc.1](https://github.com/Funnyguysmz/RE4R-ShapeKey-System/releases/tag/v0.4.5-rc.1), with Studio's bridge appended by the build. The synthetic profile is documentation/test data only and is never installed as the active model profile.

随包运行时为System 0.4.5-rc.1，构建时追加Studio桥接；合成profile仅作合同示例，不安装为活动配置。
