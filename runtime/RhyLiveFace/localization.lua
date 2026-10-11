-- Local UI adapter: stable ImGui IDs across Chinese/English changes.
local M={language='zh'}
local ok,saved=pcall(json.load_file,'RhyLiveFace_language.json')
if ok and type(saved)=='table' and (saved.language=='zh' or saved.language=='en') then M.language=saved.language end
local zh={
 ['RhyLive Face 0.2.0 (RE4)']='RhyLive 面捕 0.2.0 (RE4)',
 ['Expression channels']='表情通道', ['Brows']='眉毛', ['Eyes']='眼睛', ['Gaze']='眼球注视',
 ['Cheeks / nose']='脸颊 / 鼻子', ['Jaw']='下颌', ['Mouth']='嘴', ['Tongue']='舌头',
 ['Reset all gains to 1']='全部幅度重置为 1', ['Reset all test sliders']='全部测试滑条归零',
 ['Player facial joints only. Mapping: RhyLiveFace_profile.json, or the built-in blink/jaw default.']='仅驱动当前玩家面部骨骼；映射来自 RhyLiveFace_profile.json，没有时使用内置的眨眼/张嘴默认映射。',
 ['UDP port']='UDP 端口', ['Bind IPv4']='监听 IPv4',
 ['Start / restart receiver']='启动 / 重启接收', ['Stop receiver']='停止接收',
 ['Capture neutral (idle, eyes open, mouth closed)']='采集中性姿态（待机、睁眼、闭嘴）',
 ['Drive face']='启用面部驱动', ['Manual test (ignores phone)']='手动测试（忽略手机输入）',
 ['Input response']='响应速度（越大越快）', ['Disconnect timeout ms']='断流超时（毫秒）',
 ['Position delta scale']='位置变化倍率', ['Model calibration / profile']='模型校准 / 映射配置',
 ['Endpoint: 1 left blink / 2 right blink / 3 jaw']='终点：1 左眨眼 / 2 右眨眼 / 3 张嘴',
 ['Capture selected endpoint from current pose']='从当前姿态采集所选终点',
 ['Save calibrated profile']='保存校准映射', ['Reload profile']='重新加载映射',
 ['Dump input diagnostics']='导出输入诊断',
 ['Native receiver missing: install RhyLiveFace.dll and restart game.']='缺少接收模块：请安装 RhyLiveFace.dll 并重启游戏。',
 ['Stop drive, pose bones with EMV, then capture endpoint. Same head object required.']='先关闭驱动，用 EMV 调整面骨姿态，再采集终点；需使用同一个头部对象。',
 ['Start receiver, enter gameplay, then capture neutral.']='请启动接收、进入游戏场景，再采集中性姿态。',
 ['Receiver stopped.']='接收已停止。',
 ['Receiver listening; waiting for /Face data.']='正在监听，等待手机面部数据。',
 ['Receiver listening; face stream timed out.']='正在监听，面部输入已中断。',
 ['Receiver listening; face stream active.']='正在监听，面部数据正常。',
 ['Face joints unavailable; enter gameplay with a facial-bone model.']='未找到面部骨骼，请进入游戏并使用带面骨的模型。',
 ['Player/head object changed. Capture neutral again.']='角色或头部对象已变化，请重新采集中性姿态。',
 ['Capture neutral first.']='请先采集中性姿态。',
 ['Manual continuous test']='正在手动测试', ['RhyLive driving face']='RhyLive 正在驱动面部',
 ['Input lost; face control released.']='输入中断，已释放面部控制。',
 ['Drive disabled; face control released.']='驱动已关闭，已释放面部控制。',
 ['Drive disabled.']='驱动已关闭。', ['Profile saved.']='映射配置已保存。',
 ['Profile reloaded; capture neutral again.']='映射已重载，请重新采集中性姿态。',
 ['Player facial joints only. Default mapping: left blink, right blink, jaw open.']='仅驱动当前玩家面部骨骼；默认映射：左眨眼、右眨眼、张嘴。',
}
local CH={browDownLeft='左皱眉',browDownRight='右皱眉',browInnerUp='眉头上扬',browOuterUpLeft='左眉尾上扬',browOuterUpRight='右眉尾上扬',
 eyeBlinkLeft='左眨眼',eyeBlinkRight='右眨眼',eyeSquintLeft='左眯眼',eyeSquintRight='右眯眼',eyeWideLeft='左睁大',eyeWideRight='右睁大',
 eyeLookUpLeft='左眼上看',eyeLookUpRight='右眼上看',eyeLookDownLeft='左眼下看',eyeLookDownRight='右眼下看',
 eyeLookInLeft='左眼内看',eyeLookInRight='右眼内看',eyeLookOutLeft='左眼外看',eyeLookOutRight='右眼外看',
 cheekPuff='鼓腮',cheekSquintLeft='左颊上提',cheekSquintRight='右颊上提',noseSneerLeft='左皱鼻',noseSneerRight='右皱鼻',
 jawOpen='张嘴',jawForward='下颌前伸',jawLeft='下颌左移',jawRight='下颌右移',
 mouthClose='闭唇',mouthFunnel='O 型嘴',mouthPucker='噘嘴',mouthLeft='嘴左移',mouthRight='嘴右移',
 mouthSmileLeft='左微笑',mouthSmileRight='右微笑',mouthFrownLeft='左嘴角下压',mouthFrownRight='右嘴角下压',
 mouthDimpleLeft='左嘴角后收',mouthDimpleRight='右嘴角后收',mouthStretchLeft='左嘴角横拉',mouthStretchRight='右嘴角横拉',
 mouthRollLower='下唇内卷',mouthRollUpper='上唇内卷',mouthShrugLower='下唇上耸',mouthShrugUpper='上唇上耸',
 mouthPressLeft='左抿唇',mouthPressRight='右抿唇',mouthLowerDownLeft='左下唇下拉',mouthLowerDownRight='右下唇下拉',
 mouthUpperUpLeft='左上唇上提',mouthUpperUpRight='右上唇上提',tongueOut='伸舌'}local prefixes={
 {'Receiver stopped; Winsock code ','接收已停止，Winsock 错误码 '},
 {'UDP start failed, Winsock code ','启动失败，Winsock 错误码 '},
 {'Receiver polling error: ','输入轮询错误：'}, {'Profile error: ','映射配置错误：'},
 {'Endpoint rejected: ','终点采集被拒绝：'}, {'Endpoint captured: ','已采集终点：'},
 {'Runtime error: ','运行错误：'},
}
function M.text(value)
 if value=='Language / 语言' then return M.language=='zh' and '切换为 English' or 'Switch to 中文' end
 if M.language=='en' then return value end
 if zh[value] then return zh[value] end
 for _,p in ipairs(prefixes) do if value:sub(1,#p[1])==p[1] then return p[2]..value:sub(#p[1]+1) end end
 local n=value:match('^Neutral captured: (%d+) face bones%. Enable drive to test%.$')
 if n then return '已采集 '..n..' 根面骨的中性姿态，可开启驱动。' end
 local running,port,err=value:match('^Listening: (%u+)  Active port: (%d+)  Error: (%d+)$')
 if running then return string.format('监听：%s  当前端口：%s  错误码：%s',running=='YES' and '是' or '否',port,err) end
 local packets,frames,rejected,age=value:match('^Packets (%d+)  Face frames (%d+)  Rejected (%d+)  Age (%d+) ms$')
 if packets then return string.format('收包 %s  面部帧 %s  拒绝 %s  距上次有效帧 %s 毫秒',packets,frames,rejected,age) end
 local polls,poses=value:match('^Input polls (%d+)  Pose callbacks (%d+)$')
 if polls then return string.format('输入轮询 %s  面部回调 %s',polls,poses) end
 local mapped=value:match('^Mapped channels: (%d+) / 52$')
 if mapped then return '已映射通道：'..mapped..' / 52' end
 local ch,kind=value:match('^(%w+) (%a+)$')
 if ch and CH[ch] and (kind=='test' or kind=='gain') then return CH[ch]..(kind=='test' and ' 测试' or ' 幅度') end
 ch=value:match('^(%w+): not mapped$')
 if ch and CH[ch] then return CH[ch]..'：未映射' end
 local rest
 ch,rest=value:match('^(%w+): (.*)$')
 if ch and CH[ch] then return CH[ch]..' ('..ch..')：'..rest end
 return value
end
function M.label(value) return M.text(value)..'###RhyLiveFace_'..value end
function M.toggle()
 M.language=M.language=='zh' and 'en' or 'zh'
 local success,err=pcall(json.dump_file,'RhyLiveFace_language.json',{language=M.language})
 if not success then log.error('[RhyLiveFace] Language save: '..tostring(err)) end
end
function M.wrap(host)
 local ui=setmetatable({}, {__index=host})
 for _,key in ipairs({'tree_node','button','checkbox','slider_float','drag_int','input_text'}) do
  local fn=host[key]
  ui[key]=function(label,...) return fn(M.label(label),...) end
 end
 ui.text=function(value,...)return host.text(M.text(value),...)end
 return ui
end
return M
