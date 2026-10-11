-- RE4 RhyLive Face v0.2.0. All 52 ARKit channels in the UI (grouped), per-channel gain, settings saved.
-- Receiver, pose solve and drive/release logic unchanged from 0.1.3.
if reframework:get_game_name():lower()~='re4' then return end
local VERSION='0.2.0'
local mathface=require('RhyLiveFace.face_math')
local defaults=require('RhyLiveFace.default_profile')
local language=require('RhyLiveFace.localization')
local imgui=language.wrap(imgui)
local native=rhylive_native
local profile,order,bone_names
local base,target_id={},nil
local drive,manual=false,false
local pending=nil
local snapshot={running=false,sequence=0,age_ms=2147483647,values={}}
local filtered,last_seq={},0
local status='Start receiver, enter gameplay, then capture neutral.'
local settings={port=9000,bind_ip='0.0.0.0',timeout_ms=500,smoothing=0.35,position_scale=1,gains={}}
local GROUPS={
    {'Brows',{'browDownLeft','browDownRight','browInnerUp','browOuterUpLeft','browOuterUpRight'}},
    {'Eyes',{'eyeBlinkLeft','eyeBlinkRight','eyeSquintLeft','eyeSquintRight','eyeWideLeft','eyeWideRight'}},
    {'Gaze',{'eyeLookUpLeft','eyeLookUpRight','eyeLookDownLeft','eyeLookDownRight','eyeLookInLeft','eyeLookInRight','eyeLookOutLeft','eyeLookOutRight'}},
    {'Cheeks / nose',{'cheekPuff','cheekSquintLeft','cheekSquintRight','noseSneerLeft','noseSneerRight'}},
    {'Jaw',{'jawOpen','jawForward','jawLeft','jawRight'}},
    {'Mouth',{'mouthClose','mouthFunnel','mouthPucker','mouthLeft','mouthRight','mouthSmileLeft','mouthSmileRight',
        'mouthFrownLeft','mouthFrownRight','mouthDimpleLeft','mouthDimpleRight','mouthStretchLeft','mouthStretchRight',
        'mouthRollLower','mouthRollUpper','mouthShrugLower','mouthShrugUpper','mouthPressLeft','mouthPressRight',
        'mouthLowerDownLeft','mouthLowerDownRight','mouthUpperUpLeft','mouthUpperUpRight'}},
    {'Tongue',{'tongueOut'}},
}
local tests={}
for _,g in ipairs(GROUPS) do for _,name in ipairs(g[2]) do tests[name]=0 end end
local selected=1
local selection={'eyeBlinkLeft','eyeBlinkRight','jawOpen'}
local controlled_last=false
local receiver_status='Receiver stopped.'
local poll_ticks,pose_ticks=0,0
local first_packet_reported=false
-- settings persistence: saved shortly after the last change, so dragging a slider does not write every frame
local SETTINGS_FILE='RhyLiveFace_settings.json'
local save_due=nil
local draw_tick=0
local function load_settings()
    local ok,s=pcall(json.load_file,SETTINGS_FILE)
    if not ok or type(s)~='table' then return end
    for _,k in ipairs({'port','timeout_ms','smoothing','position_scale'}) do
        if mathface.finite(s[k]) then settings[k]=s[k] end
    end
    if type(s.bind_ip)=='string' then settings.bind_ip=s.bind_ip end
    if type(s.gains)=='table' then
        for name,v in pairs(s.gains) do
            if defaults.channel_names[name] and mathface.finite(v) then settings.gains[name]=mathface.clamp(v,0,3) end
        end
    end
end
local function mark_dirty() save_due=draw_tick+60 end
local function flush_settings(force)
    if not save_due or (not force and draw_tick<save_due) then return end
    save_due=nil
    local ok,err=pcall(json.dump_file,SETTINGS_FILE,settings)
    if not ok then log.error('[RhyLiveFace] Settings save: '..tostring(err)) end
end
local function dump_diagnostics(reason)
    local ok,err=pcall(json.dump_file,'RhyLiveFace_input.json',{
        version=VERSION,reason=reason,receiver=snapshot,settings=settings,
        poll_ticks=poll_ticks,pose_ticks=pose_ticks,drive=drive,manual=manual,
        receiver_status=receiver_status,face_status=status
    })
    if not ok then log.error('[RhyLiveFace] Diagnostics: '..tostring(err)) end
end
local function poll_receiver()
    if not native then return end
    snapshot=native.poll()
    poll_ticks=poll_ticks+1
    if snapshot.sequence~=last_seq then
        for name in pairs(defaults.channel_names) do
            local v=snapshot.values[name] or 0
            filtered[name]=(filtered[name] or v)+(v-(filtered[name] or v))*settings.smoothing
        end
        last_seq=snapshot.sequence
    end
    if not snapshot.running then
        receiver_status=(snapshot.error or 0)~=0 and ('Receiver stopped; Winsock code '..tostring(snapshot.error)) or 'Receiver stopped.'
    elseif (snapshot.sequence or 0)==0 then
        receiver_status='Receiver listening; waiting for /Face data.'
    elseif snapshot.age_ms>settings.timeout_ms then
        receiver_status='Receiver listening; face stream timed out.'
    else
        receiver_status='Receiver listening; face stream active.'
    end
    if snapshot.running and (snapshot.packets or 0)>0 and not first_packet_reported then
        first_packet_reported=true
        dump_diagnostics('first_packet')
    end
end
local function safe_poll_receiver()
    local ok,err=pcall(poll_receiver)
    if not ok then
        receiver_status='Receiver polling error: '..tostring(err)
        drive=false
        snapshot={running=false,sequence=0,age_ms=2147483647,values={}}
    end
end
-- Network input must advance even in menus, with drive disabled or without a model.
-- The frame tick also flushes pending settings, so a change is saved even after the menu is closed.
re.on_frame(function() safe_poll_receiver();draw_tick=draw_tick+1;flush_settings(false) end)
local function load_profile()
    local p=json.load_file('RhyLiveFace_profile.json') or defaults.profile
    local ok,a,b=pcall(mathface.validate,p,defaults.channel_names,defaults.allowed_bones)
    if not ok then status='Profile error: '..tostring(a); profile=nil; drive=false; return end
    profile,order,bone_names=p,a,b
end
local function call(obj,name,...)
    if not obj then return nil end
    return obj:call(name,...)
end
local function address(obj) return obj and tostring(obj:get_address()) end
local function resolve()
    local cm=sdk.get_managed_singleton('chainsaw.CharacterManager')
    local ctx=call(cm,'getPlayerContextRef()')
    local body=call(ctx,'get_BodyUpdater')
    local hm=call(body,'get_HeadMotion')
    local candidates={call(hm,'get_GameObject'),call(ctx,'get_HeadGameObject'),call(ctx,'get_BodyGameObject')}
    for i=1,3 do
        local go=candidates[i]
        if go then
            local xf=call(go,'get_Transform')
            local joints={}; local count=0
            for name in pairs(bone_names or {}) do
                local j=call(xf,'getJointByName',name)
                if j then joints[name]=j;count=count+1 end
            end
            if joints.L_U_Eyelid3 and joints.R_U_Eyelid3 and joints.C_Jaw then
                return joints,address(ctx)..':'..address(xf),count
            end
        end
    end
    return nil
end
local function read_pose(joints)
    local pose={}
    for name,j in pairs(joints) do
        local p=call(j,'get_LocalPosition'); local q=call(j,'get_LocalRotation')
        if p and q then pose[name]={p={p.x,p.y,p.z},q=mathface.normalize({q.x,q.y,q.z,q.w})} end
    end
    return pose
end
local function write_pose(joints,pose)
    for name,v in pairs(pose) do
        local j=joints[name]
        if j then
            call(j,'set_LocalPosition',Vector3f.new(v.p[1],v.p[2],v.p[3]))
            call(j,'set_LocalRotation',Quaternion.new(v.q[4],v.q[1],v.q[2],v.q[3]))
        end
    end
end
local function clear_target(message)
    base={};target_id=nil;drive=false;pending=nil;controlled_last=false
    status=message
end
local function frame()
    pose_ticks=pose_ticks+1
    if not profile then return end
    if not pending and not drive and not controlled_last then return end
    local joints,id,count=resolve()
    if not joints then clear_target('Face joints unavailable; enter gameplay with a facial-bone model.'); return end
    if target_id and target_id~=id then clear_target('Player/head object changed. Capture neutral again.');return end
    if pending then
        if controlled_last then write_pose(joints,base);controlled_last=false;return end
        local action=pending;pending=nil
        if action=='neutral' then
            base=read_pose(joints);target_id=id
            status='Neutral captured: '..tostring(count)..' face bones. Enable drive to test.'
        else
            if not target_id then status='Capture neutral first.';return end
            local pose=read_pose(joints);local entries={}
            for _,entry in ipairs(profile.channels[action] or {}) do
                local name=entry.bone;local b=base[name]
                local v=pose[name]
                if v and b then
                    entries[#entries+1]={bone=name,pos_delta={v.p[1]-b.p[1],v.p[2]-b.p[2],v.p[3]-b.p[3]},
                        rot_delta=mathface.mul(mathface.inverse(b.q),v.q)}
                end
            end
            local proposed={schema=1,channels={}}
            for k,v in pairs(profile.channels) do proposed.channels[k]=v end
            proposed.channels[action]=entries
            local ok,a,b=pcall(mathface.validate,proposed,defaults.channel_names,defaults.allowed_bones)
            if ok then profile,order,bone_names=proposed,a,b;status='Endpoint captured: '..action
            else status='Endpoint rejected: '..tostring(a) end
        end
        return
    end
    local live=snapshot.running and snapshot.sequence>0 and snapshot.age_ms<=settings.timeout_ms
    if drive and target_id and (manual or live) then
        controlled_last=true
        write_pose(joints,mathface.solve(profile,order,base,manual and tests or filtered,settings.gains,settings.position_scale))
        status=manual and 'Manual continuous test' or 'RhyLive driving face'
    elseif controlled_last then
        -- One release write clears our last pose. Later frames leave native animation alone.
        write_pose(joints,base);controlled_last=false
        status=drive and 'Input lost; face control released.' or 'Drive disabled; face control released.'
    end
end
re.on_application_entry('PrepareRendering',function()
    local ok,err=pcall(frame)
    if not ok then
        local good,joints,id=pcall(resolve)
        if good and joints and id==target_id then
            for name,v in pairs(base) do pcall(write_pose,joints,{[name]=v}) end
        end
        clear_target('Runtime error: '..tostring(err));log.error('[RhyLiveFace] '..status)
    end
end)
local function mapped(name) return profile and profile.channels[name] and #profile.channels[name]>0 end
local function channel_rows(names)
    local changed
    for _,name in ipairs(names) do
        if not mapped(name) then
            imgui.text(name..': not mapped')
        else
            if manual then changed,tests[name]=imgui.slider_float(name..' test',tests[name],0,1)
            else imgui.text(string.format('%s: %.3f',name,filtered[name] or 0)) end
            changed,settings.gains[name]=imgui.slider_float(name..' gain',settings.gains[name] or 1,0,3)
            if changed then mark_dirty() end
        end
    end
end
re.on_draw_ui(function()
    if not imgui.tree_node('RhyLive Face '..VERSION..' (RE4)') then flush_settings(false) return end
    if imgui.button('Language / 语言') then language.toggle() end
    safe_poll_receiver()
    if not native then imgui.text('Native receiver missing: install RhyLiveFace.dll and restart game.') end
    local changed,c2
    changed,settings.port=imgui.drag_int('UDP port',settings.port,1,1,65535)
    c2,settings.bind_ip=imgui.input_text('Bind IPv4',settings.bind_ip)
    if changed or c2 then mark_dirty() end
    if native and imgui.button('Start / restart receiver') then
        local ok,err=native.start(settings.port,settings.bind_ip)
        filtered={};last_seq=0;first_packet_reported=false
        safe_poll_receiver()
        if not ok then receiver_status='UDP start failed, Winsock code '..tostring(err) end
        log.info('[RhyLiveFace] Start '..settings.bind_ip..':'..tostring(settings.port)..' result '..tostring(ok)..' code '..tostring(err))
        dump_diagnostics('start')
    end
    if native and imgui.button('Stop receiver') then
        native.stop();manual=false;drive=false;filtered={};last_seq=0
        safe_poll_receiver()
        status='Drive disabled.'
        log.info('[RhyLiveFace] Stop; running='..tostring(snapshot.running))
        dump_diagnostics('stop')
    end
    imgui.text(receiver_status)
    imgui.text(string.format('Listening: %s  Active port: %d  Error: %d',snapshot.running and 'YES' or 'NO',snapshot.port or 0,snapshot.error or 0))
    imgui.text(string.format('Packets %d  Face frames %d  Rejected %d  Age %d ms',snapshot.packets or 0,snapshot.sequence or 0,snapshot.rejected or 0,snapshot.age_ms or 0))
    imgui.text(string.format('Input polls %d  Pose callbacks %d',poll_ticks,pose_ticks))
    imgui.text(status)
    if imgui.button('Capture neutral (idle, eyes open, mouth closed)') then
        drive=false;pending='neutral'
    end
    changed,drive=imgui.checkbox('Drive face',drive)
    if drive and not target_id then drive=false;status='Capture neutral first.' end
    changed,manual=imgui.checkbox('Manual test (ignores phone)',manual)
    changed,settings.smoothing=imgui.slider_float('Input response',settings.smoothing,0.05,1)
    if changed then mark_dirty() end
    changed,settings.timeout_ms=imgui.drag_int('Disconnect timeout ms',settings.timeout_ms,10,100,2000)
    if changed then mark_dirty() end
    changed,settings.position_scale=imgui.slider_float('Position delta scale',settings.position_scale,0.1,2)
    if changed then mark_dirty() end
    if imgui.tree_node('Expression channels') then
        local n=0
        for _,g in ipairs(GROUPS) do for _,name in ipairs(g[2]) do if mapped(name) then n=n+1 end end end
        imgui.text(string.format('Mapped channels: %d / 52',n))
        if imgui.button('Reset all gains to 1') then settings.gains={};mark_dirty() end
        if manual and imgui.button('Reset all test sliders') then for k in pairs(tests) do tests[k]=0 end end
        for _,g in ipairs(GROUPS) do
            if imgui.tree_node(g[1]) then channel_rows(g[2]);imgui.tree_pop() end
        end
        imgui.tree_pop()
    end
    if imgui.tree_node('Model calibration / profile') then
        imgui.text('Stop drive, pose bones with EMV, then capture endpoint. Same head object required.')
        changed,selected=imgui.drag_int('Endpoint: 1 left blink / 2 right blink / 3 jaw',selected,1,1,3)
        if imgui.button('Capture selected endpoint from current pose') then
            drive=false;pending=selection[selected]
        end
        if imgui.button('Save calibrated profile') and profile then json.dump_file('RhyLiveFace_profile.json',profile);status='Profile saved.' end
        if imgui.button('Reload profile') then drive=false;load_profile();clear_target('Profile reloaded; capture neutral again.') end
        if imgui.button('Dump input diagnostics') then safe_poll_receiver();dump_diagnostics('manual_dump') end
        imgui.tree_pop()
    end
    imgui.text('Player facial joints only. Mapping: RhyLiveFace_profile.json, or the built-in blink/jaw default.')
    imgui.tree_pop()
    flush_settings(false)
end)
re.on_script_reset(function()
    flush_settings(true)
    if controlled_last then
        local ok,joints,id=pcall(resolve)
        if ok and joints and id==target_id then pcall(write_pose,joints,base) end
    end
    if native then native.stop() end
end)
load_settings()
load_profile()
log.info('[RhyLiveFace] '..VERSION..' loaded; 52-channel UI, saved settings; receiver and drive start disabled')
