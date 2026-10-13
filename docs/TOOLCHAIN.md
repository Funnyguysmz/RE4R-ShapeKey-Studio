# Toolchain and upgrade guide / 工具链与升级

Studio 0.4.2-beta.1 is a pre-release for the optional nine-input model route.
Studio drives an authored model's custom channels; it cannot add shape keys to an
ordinary model or create its weights, base normals, Head binding or model profile.

## Model authoring / 模型制作

The public [exporter fork](https://github.com/Funnyguysmz/RE-Mesh-Editor-BlendShape)
has the verified [v0.70.6-re4r.1 pre-release](https://github.com/Funnyguysmz/RE-Mesh-Editor-BlendShape/releases/tag/v0.70.6-re4r.1).
It is a separate GPLv3 Blender add-on, not bundled or relicensed under Studio's MIT license.

Its two RE4 options default to off:

- Add nine named, zero-displacement native input channels. A same-named channel
  with nonzero displacement is rejected. The runtime reads these inputs; it does
  not write native weights.
- Use packed displacements with int8 base normals and no normalRecalc. This route
  can retain the final authored toon base normals and does not require adding fins.
  It uses the final base normals for each key; it does not automatically generate
  cylindrical normals, bind weights, or remove existing fins.

Therefore shape keys do not necessarily overwrite toon shading normals. Authors
still need to prepare the appropriate cylindrical/base normals, Head binding,
weights, shape keys and a matching System profile with actual names and hashes.
Changing only an export switch does not create those prerequisites.

导出器的两个开关默认关闭：添加九个指定零位移原生接收口；packed位移＋int8基础法线且不输出normalRecalc。
后一路线可保留已制作的三渲二基础法线，无需增加fins；不会自动造圆柱法线、绑权重或删除已有fins。
因此不能再笼统说“形态键必然覆盖三渲二法线”。基础法线、Head绑定、权重和模型profile仍需制作。

## Optional mapping / 可选映射

The package carries the traditional Leon mapping and an independent mouth9 map.
`native_map_file` selects a mapping within a model's own profile; omitting it keeps
the old mapping. Legacy91-input and NoTap profiles are not globally converted.
See [instructions](../examples/mouth9/README.md). A synthetic example, if supplied,
is only for documentation/tests, never an active profile or a model dependency.

通用映射不等于角色profile；九输入是可选模型方案。安装器不覆盖已有profile、settings或映射。
已有同名表会提示并保留，不会为所有模型强行换成九口表。

## Upgrade and recovery / 升级与回退

Quit RE4 before installation. Normal GUI installation preserves existing script
bodies and refreshes the Studio adapter. To explicitly replace the System body
with this package's version, use `Install.ps1 -GameDirectory <game root>
-UpdateShapeKeyRuntime`. Review personal script modifications first. The installer
backs up the original Studio-adapted script and records a manifest. Receiver
script bodies, user profiles/settings and maps remain intact.

Restore with the game closed: copy the original script from the relevant backup
to its recorded game path. Restore any model/profile you deliberately changed
separately. Use the Studio-adapted backup, not an unadapted System baseline.
Installation failures roll back new writes. Do not run two scripts that write
the same custom channels simultaneously.

普通GUI安装保留System主体；明确采用新版主体需显式参数，原脚本先备份。
回退按manifest恢复原Studio适配脚本；不能用缺GUI桥接的裸基础Lua覆盖它。

## Evidence / 验证边界

Offline authoring evidence covers 85/94 channel combinations, import/export round
trips, batch export and System profile name/hash matching. The user reported the
standalone85-channel normal experiment was fine. The integrated94-channel plus
nine-input combination has not received separate game feedback. Signal binding,
blinking, mouth opening/closing and idle closed-mouth remain pending. Different
per-expression normal mixing and all models are not universally validated.

已有0.4桌面功能的用户实机确认与新模型组合的验证分开记录。85通道独立实验用户反馈“没问题”，
不能外推为94通道组合或所有模型逐表情法线混合均通过。本轮不新增代表性过场要求。
