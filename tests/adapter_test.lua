local adapter=assert(io.open(arg[1],'rb')):read('*a')
local prelude=[[
local S={enabled=false,source=3,values={old=0.2},strength=1,region={}}
local profile={model='mock'}
local keys={{id='jaw',ch=0,label='张嘴',region='mouth'}}
local key_by_id={jaw=keys[1]}
local bind={go_id=10,mesh_id=20,limit=1,mesh={}}
local cap={status='mock input'}
local status,last_error='mock ready',nil
local sweep,compare_neutral
local valid=true
local function finite(n)return type(n)=='number' and n==n and n~=math.huge and n~=-math.huge end
local function copy(t)local o={} for k,v in pairs(t)do o[k]=type(v)=='table' and copy(v) or v end return o end
local function clamp(n,a,b)return math.max(a,math.min(b,n))end
local function touch()end
local saved,zeroed=0,0
local function save_settings()saved=saved+1 end
local function still_valid()return valid end
local function zero_written()zeroed=zeroed+1 end
local function active()return S.enabled and bind~=nil end
local function unbind()bind=nil end
local function call(_,name)if name=='get_BlendShapeChannelWeightCount' then return 1 end return 0.75 end
local callback,reset
re={on_frame=function(f)callback=f end,on_script_reset=function(f)reset=f end}
local queue,published,connected='',nil,true
shapekey_desktop={poll=function()local s=queue queue='' return s end,publish=function(s)published=s end,connected=function()return connected end}
]]
local checks=[[
local function run(cmd)queue=cmd or '';callback() end
local t=desktop_identity()
run('C\t'..t..'\tstart\t\t\nC\t'..t..'\tset\tjaw\t0.75\n')
assert(S.enabled and S.values.jaw==0.75 and desktop_owned)
run('C\tBAD\tset\tjaw\t0.1\n');assert(S.values.jaw==0.75)
run('C\t'..t..'\tset\tunknown\t0.3\nC\t'..t..'\tset\tjaw\tnan\n');assert(S.values.jaw==0.75)
run('C\t'..t..'\tset\tjaw\t5\n');assert(S.values.jaw==1)
run('C\t'..t..'\tsource\t1\t\n');assert(S.source==1)
run();assert(published:find('K\tjaw\t张嘴\tmouth\t1.0000\t0.7500\t1',1,true))
connected=false;run();assert(not S.enabled and S.values.old==0.2 and S.values.jaw==nil and zeroed==1 and saved==1)
connected=true;run('C\t'..t..'\tstart\t\t\n');assert(desktop_owned)
bind.mesh_id=99;run();assert(not desktop_owned and zeroed==1) -- must not zero a newly bound mesh
run('C\t'..t..'\tstart\t\t\n');assert(not desktop_owned)
run('C\t'..desktop_identity()..'\tstart\t\t\n');valid=false;run();assert(not desktop_owned and zeroed==1)
-- Multi-mesh readback must use a body's array/limit rather than the head's.
valid=true
local body_count=20
local head_mesh,body_mesh={},{}
bind={go_id=10,mesh_id=20,mesh=head_mesh,limit=1,meshes={
    {go_id=10,mesh_id=20,mesh=head_mesh,limit=1},
    {go_id=11,mesh_id=30,mesh=body_mesh,limit=20}}}
keys[1].mi=1
keys[2]={id='body',ch=7,mi=2,label='身体',region='body'};key_by_id.body=keys[2]
call=function(mesh,name)
    if name=='get_BlendShapeChannelWeightCount' then return mesh==body_mesh and body_count or 1 end
    return mesh==body_mesh and 0.4 or 0.75
end
run();t=desktop_identity()
run('C\t'..t..'\tstart\t\t\nC\t'..t..'\tset\tbody\t0.2\n')
assert(S.values.body==0.2)
for i=1,6 do run() end
assert(published:find('K\tbody\t身体\tbody\t0.2000\t0.4000\t1\t2',1,true))
body_count=0;run('C\t'..t..'\tset\tbody\t0.9\n');assert(S.values.body==0.2)
for i=1,6 do run() end
assert(published:find('K\tbody\t身体\tbody\t0.2000\t0.0000\t0\t2',1,true))
bind.meshes[2].mesh_id=31;run();assert(not desktop_owned)
body_count=20
bind.meshes[2].go={current=31}
local function component(go)return {id=go.current} end
local function addr(object)return object.id end
-- Make these locally resolved names visible to the compiled adapter via globals.
_G.component=component;_G.addr=addr
t=desktop_identity();run('C\t'..t..'\tstart\t\t\n');assert(desktop_owned)
bind.meshes[2].go.current=32
assert(not active() and bind==nil and not desktop_owned)
bind={go_id=10,mesh_id=20,mesh=head_mesh,limit=1};run();local old=desktop_identity()
bind={go_id=10,mesh_id=20,mesh=head_mesh,limit=1};assert(desktop_identity()~=old)
print('ADAPTER_TOKEN_DISCONNECT_FINITE_READBACK_PASS')
print('ADAPTER_MULTI_MESH_READBACK_AND_ALLOCATION_GUARD_PASS')
bind=nil
local last_receiver
rhylive_desktop_api={execute=function(op,port,ip)last_receiver={op,port,ip};return true end,
    snapshot=function()return {available=true,running=true,port=9000,bind_ip='0.0.0.0',packets=12,sequence=8,age_ms=4,rejected=0,error=0,status='active'}end}
run('C\tWAIT\treceiver_start\t9000\t0.0.0.0\n')
assert(last_receiver and last_receiver[1]=='receiver_start' and last_receiver[2]=='9000')
for i=1,6 do run() end
assert(published:find('R\t1\t1\t9000\t0.0.0.0\t12\t8\t4',1,true))
print('RECEIVER_COMMAND_BEFORE_MODEL_BIND_AND_SNAPSHOT_PASS')
]]
assert(load(prelude..adapter..checks,'adapter-test'))()
