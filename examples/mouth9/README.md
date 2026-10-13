# Optional nine-input model mapping / 可选九输入方案

The mapping is optional. Profiles without native_map_file keep the traditional Leon mapping; existing 91-input and NoTap model paths remain supported. This package does not contain a character profile, mesh, recordings or private settings.

九输入是可选模型方案：旧profile不写native_map_file时仍使用传统表；不把91通道或NoTap模型一律切换。这里不附角色专用profile、网格、录制或私人设置。

For a correctly authored nine-input model, add the following field to its own matching profile:

    "native_map_file": "RE4R_ShapeKeySystem_native_leon_mouth9.json"

This is a field fragment, not a complete installable profile. The model author must supply actual keys, full channel names/hashes, head binding and nine native_taps matching the exported mesh. Do not replace an existing model profile with a generic example.

这是字段片段，不是可安装profile。制作者仍需提供自定义键、实际通道名字/hash、Head绑定和匹配九口的native_taps，不能用通用示例覆盖旧模型配置。

## Upgrade and rollback / 升级与回退

Quit RE4 first. Ordinary GUI installation preserves existing System and RhyLive script bodies and profiles/settings/maps. Missing mapping files are added individually; existing files are kept with a message.

To explicitly adopt the packaged System runtime, after inspecting any personal script edits:

    powershell -ExecutionPolicy Bypass -File .\Install.ps1 -GameDirectory 'C:\Games\RE4' -UpdateShapeKeyRuntime

The game path is an example. The previous System script is backed up under backups with a manifest; the receiver script body and user data are preserved. This option replaces the System body with the new packaged version and appends the Studio bridge once. If the previous runtime is retained, nine-input mapping requires its own compatible update.

升级前退出游戏。普通GUI安装仅更新桥接段，保留已有脚本主体和profile/settings/maps；缺失映射分别补齐。显式参数会备份并替换System主体，所以有个人代码修改时先核对差异。不会自动更换模型profile。

Rollback with RE4 closed: restore the backed-up reframework/autorun/RE4R_ShapeKeySystem.lua to the same game path, then restore any profile/model you chose to change separately. Use the original Studio-adapted backup, not a bare System baseline without the GUI bridge. Installation failure rolls back newly written files automatically.

回退时退出游戏，按backup manifest把原Studio适配脚本恢复至同一路径，再恢复手动更换过的profile/模型。不要用不含GUI桥接的裸System旧脚本覆盖。

## Evidence / 验证范围

Offline exporter checks covered 85/94 combinations, import/export round trips, batch export and profile name/hash matching. The user reported the standalone85-channel normal experiment was fine. The integrated94-channel/nine-input combination still lacks its own game feedback. Signal binding, blinking, mouth opening/closing and idle closed-mouth remain to be checked. Different per-expression normal mixing and all models are not universally validated. Reduced pucker mapping is not fully equivalent to the91-input route.

导出和回读通过不等于新九输入组合已实机通过；不增加代表性过场要求。停用NoTap pilot或其他同时写同一自定义键的脚本后再测。