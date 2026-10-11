local adapter=assert(io.open(arg[1],'rb')):read('*a')
local prelude=[[
local settings={port=9000,bind_ip='0.0.0.0',gains={jawOpen=1.7}}
local drive,manual=true,true
local filtered,last_seq,first_packet_reported={},3,true
local status,receiver_status='','active'
local snapshot={running=false,packets=4,sequence=2,age_ms=5,rejected=1,error=0}
local started,stopped,saved=0,0,0
local native={start=function(p,ip)started=started+1;snapshot.running=true;return true,0 end,stop=function()stopped=stopped+1;snapshot.running=false end}
local function safe_poll_receiver()end
local function mark_dirty()end
local function flush_settings()saved=saved+1 end
re={on_script_reset=function()end}
]]
local checks=[[
local api=rhylive_desktop_api
assert(api.execute('receiver_config','9001','192.168.1.2'))
assert(settings.port==9001 and settings.bind_ip=='192.168.1.2' and settings.gains.jawOpen==1.7)
for _,p in ipairs({'nan','0','65536','1.1'}) do assert(not api.execute('receiver_start',p,'0.0.0.0')) end
for _,ip in ipairs({'256.1.2.3','0.0.0.0x','-1.0.0.0','1.2.3','localhost'}) do assert(not api.execute('receiver_start',9000,ip)) end
assert(started==0 and saved==1)
assert(api.execute('receiver_start',9000,'0.0.0.0'))
assert(started==1 and not drive and not manual and last_seq==0)
local s=api.snapshot();assert(s.available and s.running and s.packets==4 and s.sequence==2 and s.age_ms==5)
assert(api.execute('receiver_stop'));assert(stopped==1 and not snapshot.running)
assert(not api.execute('receiver_unknown',9000,'0.0.0.0'))
native=nil;assert(not api.execute('receiver_start',9000,'0.0.0.0'))
print('RECEIVER_CONFIG_START_STOP_STATS_AND_BONE_CONFLICT_GUARD_PASS')
]]
assert(load(prelude..adapter..checks,'receiver-test'))()
