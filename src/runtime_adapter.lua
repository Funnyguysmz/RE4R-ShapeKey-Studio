-- RE4R_DESKTOP_BRIDGE_BEGIN
-- Appended by build.ps1 to a COPY of the current ShapeKey runtime.
-- The upstream source remains untouched. This block shares its local variables.
local desktop_owned, desktop_before, desktop_ticks = false, nil, 0
local desktop_token, desktop_error = '', ''
local desktop_epoch, desktop_binding_ref=0,nil
local function desktop_clean(v) return tostring(v or ''):gsub('[\t\r\n]', ' ') end
local function desktop_identity()
    if not bind then return 'WAIT' end
    if bind~=desktop_binding_ref then desktop_epoch=desktop_epoch+1;desktop_binding_ref=bind end
    local parts={tostring(bind.go_id),tostring(bind.mesh_id),tostring(profile.model or profile.title or ''),tostring(#keys)}
    for i,e in ipairs(bind.meshes or {}) do parts[#parts+1]=tostring(i)..'='..tostring(e.go_id)..','..tostring(e.mesh_id) end
    parts[#parts+1]='g'..desktop_epoch
    return table.concat(parts,':')
end
local function desktop_binding_valid()
    if not bind or not still_valid() then return false end
    for _,e in ipairs(bind.meshes or {}) do
        if e.go and type(component)=='function' then
            local current=component(e.go,'via.render.Mesh')
            if not current or addr(current)~=e.mesh_id then return false end
        end
    end
    return true
end
local function desktop_mesh(k)
    return bind and (bind.meshes and bind.meshes[k.mi or 1] or bind)
end
local function desktop_writable(k)
    local e=desktop_mesh(k)
    if type(native_key)=='function' and native_key(k) then return false end
    if type(read_weight)=='function' then
        local value=read_weight(e,k.ch)
        return e and k.ch and finite(value) and k.ch>=0 and k.ch<(e.limit or 0)
    end
    local count=e and tonumber(call(e.mesh,'get_BlendShapeChannelWeightCount'))
    return e and k.ch and finite(count) and k.ch>=0 and k.ch<math.min(count,e.limit or 0)
end
local function desktop_release()
    if desktop_owned and desktop_before then
        if desktop_binding_valid() and desktop_before.token==desktop_identity() then zero_written() end
        S.enabled, S.source, S.values = desktop_before.enabled, desktop_before.source, desktop_before.values
        S.strength, S.region = desktop_before.strength, desktop_before.region
        sweep = nil; compare_neutral = false
        save_settings()
    end
    desktop_owned, desktop_before = false, nil
end
local function desktop_command(line)
    local t, op, arg, value = line:match('^C\t([^\t]+)\t([^\t]+)\t([^\t]*)\t([^\t]*)$')
    if not t then return end
    if op:match('^receiver_') then
        local api=rawget(_G,'rhylive_desktop_api')
        if not api then desktop_error='请安装面捕通信接口并重启游戏';return end
        local ok,err=api.execute(op,arg,value)
        if not ok then desktop_error=tostring(err) end
        return
    end
    if t ~= desktop_identity() or not desktop_binding_valid() then desktop_error='模型已变化，请等待重新绑定'; return end
    if op=='cap_smooth' then
        local api=rawget(_G,'RE4R_ShapeKeySystem_api')
        local n=tonumber(value)
        if not finite(n) then desktop_error='实时度必须为有限数值';return end
        if not api or type(api.set_cap_smooth)~='function' then
            desktop_error='面板版本过旧：缺少实时度接口';return
        end
        local ok,err=api.set_cap_smooth(clamp(n,0.05,1))
        if ok==false then desktop_error=tostring(err or '实时度设置失败') end
        return
    end
    if op=='alloc' then   -- Claude 2026-10-12: panel 0.4.3 API, sizes body / hair weight arrays (works without owning)
        local api=rawget(_G,'RE4R_ShapeKeySystem_api')
        if api then api.allocate_weights(tonumber(arg)) else desktop_error='面板版本过旧：需要 0.4.3 或更新' end
        return
    end
    if op=='start' then
        if not desktop_owned then desktop_before={token=desktop_identity(),enabled=S.enabled,source=S.source,values=copy(S.values),strength=S.strength,region=copy(S.region)} end
        desktop_owned=true; S.enabled=true; compare_neutral=false; sweep=nil
    elseif op=='release' then desktop_release()
    elseif not desktop_owned then desktop_error='请先点开始控制'
    elseif op=='set' then
        local k,w=key_by_id[arg],tonumber(value)
        if k and desktop_writable(k) and finite(w) then S.values[k.id]=clamp(w,0,1);touch() end
    elseif op=='zero' then S.values={};sweep=nil;touch()
    elseif op=='source' then local n=tonumber(arg); if n and n>=1 and n<=5 and n%1==0 then S.source=n end
    elseif op=='gain' then local n=tonumber(arg);if finite(n) then S.strength=clamp(n,0,2) end
    elseif op=='compare' then compare_neutral=arg=='1'
    elseif op=='sweep' then sweep=arg=='1' and {i=1,t0=os.clock(),period=1.2,w=0} or nil
    end
end
-- Guard the reused game writer/LOD callback while the desktop owns controls.
-- The upstream file stays untouched; this closure is only in the installed adapter copy.
if type(active)=='function' then
    local desktop_base_active=active
    active=function()
        if desktop_owned and not desktop_binding_valid() then
            unbind('桌面控制：模型网格已变化，正在重新绑定。');desktop_release()
        end
        return desktop_base_active()
    end
end
re.on_frame(function()
    local dll=rawget(_G,'shapekey_desktop');if not dll then return end
    local ok,err=pcall(function()
        local identity=desktop_identity()
        if desktop_token~=identity then desktop_release();desktop_token=identity end
        if desktop_owned and (not dll.connected() or not desktop_binding_valid()) then desktop_release() end
        local input=dll.poll()
        for line in input:gmatch('[^\n]+') do desktop_command(line) end
        desktop_ticks=desktop_ticks+1
        if desktop_ticks%6~=0 then return end
        local rows={table.concat({'S',desktop_identity(),desktop_owned and '1' or '0',S.source,S.strength,
            desktop_clean(desktop_error~='' and desktop_error or status),desktop_clean(profile and (profile.title or profile.model)),
            desktop_clean(cap.status),desktop_clean(last_error),compare_neutral and '1' or '0',sweep and '1' or '0'},'\t')}
        desktop_error=''
        local receiver_api=rawget(_G,'rhylive_desktop_api')
        if receiver_api then
            local r=receiver_api.snapshot()
            rows[#rows+1]=table.concat({'R',r.available and '1' or '0',r.running and '1' or '0',r.port or 9000,
                desktop_clean(r.bind_ip or '0.0.0.0'),r.packets or 0,r.sequence or 0,r.age_ms or 2147483647,
                r.rejected or 0,r.error or 0,desktop_clean(r.status)},'\t')
        end
        local skapi=rawget(_G,'RE4R_ShapeKeySystem_api')   -- M rows (Claude 2026-10-12): per-mesh weight-array state
        if skapi and type(skapi.get_cap_smooth)=='function' then
            local n=skapi.get_cap_smooth()
            if finite(n) then rows[#rows+1]='C\t'..string.format('%.4f',clamp(n,0.05,1)) end
        end
        if skapi then
            for _,m in ipairs(skapi.mesh_status()) do
                rows[#rows+1]=table.concat({'M',m.index,desktop_clean(m.name),m.channels,tostring(m.weight_count or 0),
                    tostring(m.weight_list or 0),m.limit or 0,m.ready and '1' or '0'},'\t')
            end
        end
        for _,k in ipairs(keys) do
            local e=desktop_mesh(k)
            local actual=desktop_writable(k) and tonumber(call(e.mesh,'getBlendShapeChannelWeightParam',k.ch)) or 0
            if not finite(actual) then actual=0 end
            rows[#rows+1]=table.concat({'K',desktop_clean(k.id),desktop_clean(k.label or k.name or k.id),desktop_clean(k.region),
                string.format('%.4f',S.values[k.id] or 0),string.format('%.4f',actual or 0),
                desktop_writable(k) and '1' or '0',tostring(k.mi or 1),desktop_clean(k.group or k.region)},'\t')
        end
        dll.publish(table.concat(rows,'\n')..'\n')
    end)
    if not ok then desktop_error=tostring(err) end
end)
re.on_script_reset(function() desktop_release() end)
-- RE4R_DESKTOP_BRIDGE_END
