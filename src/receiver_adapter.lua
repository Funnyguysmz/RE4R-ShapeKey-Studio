-- RE4R_RHYLIVE_DESKTOP_BEGIN
-- Append to the installed RhyLiveFace.lua; preserve its local settings and UI.
rhylive_desktop_api = {}
local function desktop_ipv4(ip)
    if type(ip)~='string' then return false end
    local a,b,c,d=ip:match('^(%d+)%.(%d+)%.(%d+)%.(%d+)$')
    if not a then return false end
    for _,n in ipairs({a,b,c,d}) do if #n>3 or tonumber(n)>255 then return false end end
    return true
end
function rhylive_desktop_api.execute(op,port,ip)
    if op~='receiver_start' and op~='receiver_stop' and op~='receiver_config' then return false,'Unknown receiver operation' end
    if op=='receiver_stop' then
        if native then native.stop() end
        manual=false;drive=false;filtered={};last_seq=0;safe_poll_receiver()
        status='Desktop: receiver stopped, bone drive disabled.'
        return true
    end
    port=tonumber(port)
    if not port or port%1~=0 or port<1 or port>65535 or not desktop_ipv4(ip) then return false,'Invalid UDP port or bind IPv4' end
    settings.port=port;settings.bind_ip=ip
    mark_dirty();flush_settings(true)
    if op=='receiver_start' then
        if not native then return false,'RhyLiveFace.dll not loaded; restart game after installing' end
        -- Shape-key capture must not fight the legacy bone driver.
        drive=false;manual=false;filtered={};last_seq=0;first_packet_reported=false
        local ok,err=native.start(port,ip)
        safe_poll_receiver()
        if not ok then return false,'UDP start failed: '..tostring(err) end
    elseif op~='receiver_config' then return false,'Unknown receiver operation' end
    return true
end
function rhylive_desktop_api.snapshot()
    safe_poll_receiver()
    return {available=native~=nil,running=snapshot.running,port=settings.port,bind_ip=settings.bind_ip,
        packets=snapshot.packets or 0,sequence=snapshot.sequence or 0,age_ms=snapshot.age_ms or 2147483647,
        rejected=snapshot.rejected or 0,error=snapshot.error or 0,status=receiver_status}
end
re.on_script_reset(function() rhylive_desktop_api=nil end)
-- RE4R_RHYLIVE_DESKTOP_END
