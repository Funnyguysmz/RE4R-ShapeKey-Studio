local M = {}
function M.finite(v) return type(v)=='number' and v==v and math.abs(v)<math.huge end
function M.clamp(v,a,b) return math.max(a,math.min(b,v)) end
function M.normalize(q)
    local n=math.sqrt(q[1]^2+q[2]^2+q[3]^2+q[4]^2)
    assert(n>1e-8,'zero quaternion')
    return {q[1]/n,q[2]/n,q[3]/n,q[4]/n}
end
function M.mul(a,b)
    local x,y,z,w=table.unpack(a); local X,Y,Z,W=table.unpack(b)
    return M.normalize({w*X+x*W+y*Z-z*Y,w*Y-x*Z+y*W+z*X,w*Z+x*Y-y*X+z*W,w*W-x*X-y*Y-z*Z})
end
function M.inverse(q) return {-q[1],-q[2],-q[3],q[4]} end
function M.slerp_identity(q,t)
    q=M.normalize(q)
    if q[4]<0 then for i=1,4 do q[i]=-q[i] end end
    local angle=math.acos(M.clamp(q[4],-1,1))
    if angle<1e-6 then return M.normalize({q[1]*t,q[2]*t,q[3]*t,1+(q[4]-1)*t}) end
    local k=math.sin(angle*t)/math.sin(angle)
    return {q[1]*k,q[2]*k,q[3]*k,math.cos(angle*t)}
end
local function vector(v,n,limit)
    if type(v)~='table' or #v~=n then return false end
    for i=1,n do if not M.finite(v[i]) or math.abs(v[i])>limit then return false end end
    return true
end
function M.validate(profile,channel_names,allowed)
    assert(type(profile)=='table' and profile.schema==1 and type(profile.channels)=='table','invalid profile schema')
    local order,bones={},{}
    for channel,entries in pairs(profile.channels) do
        assert(channel_names[channel] and type(entries)=='table' and #entries<=256,'unknown channel or invalid entries')
        order[#order+1]=channel
        local seen={}
        for _,entry in ipairs(entries) do
            assert(type(entry)=='table' and allowed[entry.bone] and not seen[entry.bone],'unknown or duplicate face bone')
            seen[entry.bone]=true; bones[entry.bone]=true
            assert(vector(entry.pos_delta,3,0.2),'invalid position delta (max 0.2 game units per axis)')
            assert(vector(entry.rot_delta,4,1.001),'invalid rotation delta')
            entry.rot_delta=M.normalize(entry.rot_delta)
        end
    end
    assert(#order>0,'empty profile'); table.sort(order)
    return order,bones
end
function M.solve(profile,order,base,values,gains,scale)
    local result={}
    for name,b in pairs(base) do result[name]={p={table.unpack(b.p)},q={table.unpack(b.q)}} end
    for _,channel in ipairs(order) do
        local amount=M.clamp((values[channel] or 0)*(gains[channel] or 1),0,1)
        for _,entry in ipairs(profile.channels[channel]) do
            local pose=result[entry.bone]
            if pose then
                for i=1,3 do pose.p[i]=pose.p[i]+entry.pos_delta[i]*amount*scale end
                pose.q=M.mul(pose.q,M.slerp_identity(entry.rot_delta,amount))
            end
        end
    end
    return result
end
return M
