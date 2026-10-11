#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "API.h"
#include "lua.h"
#include "lauxlib.h"

#define EXPORT __declspec(dllexport)
#define FACE_COUNT 52
static const char *names[FACE_COUNT] = {
 "browDownLeft","browDownRight","browInnerUp","browOuterUpLeft","browOuterUpRight",
 "cheekPuff","cheekSquintLeft","cheekSquintRight","eyeBlinkLeft","eyeBlinkRight",
 "eyeLookDownLeft","eyeLookDownRight","eyeLookInLeft","eyeLookInRight","eyeLookOutLeft","eyeLookOutRight",
 "eyeLookUpLeft","eyeLookUpRight","eyeSquintLeft","eyeSquintRight","eyeWideLeft","eyeWideRight",
 "jawForward","jawLeft","jawOpen","jawRight","mouthClose","mouthDimpleLeft","mouthDimpleRight",
 "mouthFrownLeft","mouthFrownRight","mouthFunnel","mouthLeft","mouthLowerDownLeft","mouthLowerDownRight",
 "mouthPressLeft","mouthPressRight","mouthPucker","mouthRight","mouthRollLower","mouthRollUpper",
 "mouthShrugLower","mouthShrugUpper","mouthSmileLeft","mouthSmileRight","mouthStretchLeft","mouthStretchRight",
 "mouthUpperUpLeft","mouthUpperUpRight","noseSneerLeft","noseSneerRight","tongueOut"
};
typedef struct {
 float values[FACE_COUNT];
 uint64_t sequence, age_ms, packets, rejected;
 int running, port, error;
} RhySnapshot;
static SOCKET sock = INVALID_SOCKET;
static SRWLOCK gate = SRWLOCK_INIT;
static RhySnapshot state;
static ULONGLONG received_at;
static int wsa_started;
static uint32_t be32(const unsigned char *p) {
 return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static int osc_string(const unsigned char *p, size_t n, size_t *off, const char **s) {
 size_t start=*off, end=start;
 while(end<n && p[end]) ++end;
 if(end==n) return 0;
 size_t next=(end+4)&~(size_t)3;
 if(next>n) return 0;
 for(size_t i=end;i<next;++i) if(p[i]) return 0;
 *s=(const char*)p+start; *off=next; return 1;
}
/* Returns -1 malformed, 0 unrelated, 1 face. Last face in a valid bundle wins.
   Timetags are intentionally treated as live arrival, not scheduled playback. */
static int parse(const unsigned char *p,size_t n,float *out,int depth) {
 if(depth>4 || n<4 || (n&3)) return -1;
 if(n>=8 && !memcmp(p,"#bundle\0",8)) {
  if(n<16) return -1;
  size_t off=16; int found=0;
  while(off<n) {
   if(n-off<4) return -1;
   uint32_t len=be32(p+off); off+=4;
   if(!len || len>n-off || (len&3)) return -1;
   int r=parse(p+off,len,out,depth+1); if(r<0) return -1;
   found|=r; off+=len;
  }
  return found;
 }
 size_t off=0; const char *address,*tags;
 if(!osc_string(p,n,&off,&address) || address[0]!='/') return -1;
 if(strcmp(address,"/Face")) return 0;
 if(!osc_string(p,n,&off,&tags) || tags[0]!=',' || strlen(tags)!=FACE_COUNT+1) return -1;
 if(n-off!=FACE_COUNT*4) return -1;
 for(int i=0;i<FACE_COUNT;++i) {
  if(tags[i+1]!='f') return -1;
  uint32_t bits=be32(p+off+i*4); float v; memcpy(&v,&bits,4);
  if(!isfinite(v)) return -1;
  out[i]=v<0?0:(v>1?1:v);
 }
 return 1;
}
EXPORT int rhylive_parse(const void *packet,unsigned int length,float *out) {
 if(!packet || !out || length>65507) return -1;
 float candidate[FACE_COUNT]; int r=parse(packet,length,candidate,0);
 if(r==1) memcpy(out,candidate,sizeof(candidate));
 return r;
}
static void stop_locked(void) {
 if(sock!=INVALID_SOCKET) { closesocket(sock); sock=INVALID_SOCKET; }
 if(wsa_started) { WSACleanup(); wsa_started=0; }
 state.running=0;
}
EXPORT void rhylive_stop(void) {
 AcquireSRWLockExclusive(&gate); stop_locked(); ReleaseSRWLockExclusive(&gate);
}
EXPORT int rhylive_start(int port,const char *bind_ip) {
 AcquireSRWLockExclusive(&gate); stop_locked(); memset(&state,0,sizeof(state)); received_at=0;
 state.port=port;
 if(port<1 || port>65535) { state.error=WSAEINVAL; goto fail; }
 WSADATA data;
 if((state.error=WSAStartup(MAKEWORD(2,2),&data))) goto fail;
 wsa_started=1; sock=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
 if(sock==INVALID_SOCKET) { state.error=WSAGetLastError(); goto fail; }
 BOOL exclusive=TRUE; u_long nonblocking=1;
 if(setsockopt(sock,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(char*)&exclusive,sizeof(exclusive)) ||
    ioctlsocket(sock,FIONBIO,&nonblocking)) { state.error=WSAGetLastError(); goto fail; }
 struct sockaddr_in addr={0}; addr.sin_family=AF_INET; addr.sin_port=htons((u_short)port);
 if(!bind_ip || InetPtonA(AF_INET,bind_ip,&addr.sin_addr)!=1) {state.error=WSAEINVAL; goto fail;}
 if(bind(sock,(struct sockaddr*)&addr,sizeof(addr))) {state.error=WSAGetLastError(); goto fail;}
 state.running=1; state.error=0; ReleaseSRWLockExclusive(&gate); return 0;
fail:
 stop_locked(); int err=state.error; ReleaseSRWLockExclusive(&gate); return err;
}
EXPORT int rhylive_snapshot(RhySnapshot *out) {
 if(!out) return 0;
 AcquireSRWLockExclusive(&gate);
 if(state.running) {
  unsigned char packet[65536];
  /* Bounded, nonblocking main-thread work; backlog is drained over later frames. */
  for(int i=0;i<64;++i) {
   int n=recvfrom(sock,(char*)packet,sizeof(packet),0,NULL,NULL);
   if(n==SOCKET_ERROR) {
    int e=WSAGetLastError();
    if(e==WSAEWOULDBLOCK) break;
    if(e==WSAEMSGSIZE || e==WSAECONNRESET) {++state.rejected; continue;}
    state.error=e; stop_locked(); break;
   }
   ++state.packets; float values[FACE_COUNT]; int r=rhylive_parse(packet,n,values);
   if(r<0) ++state.rejected;
   if(r==1) {memcpy(state.values,values,sizeof(values)); ++state.sequence; received_at=GetTickCount64();}
  }
 }
 state.age_ms=state.sequence?GetTickCount64()-received_at:UINT64_MAX;
 *out=state; ReleaseSRWLockExclusive(&gate); return state.running;
}
static int l_start(lua_State *L) {
 lua_Integer port=luaL_checkinteger(L,1);
 const char *ip=luaL_optstring(L,2,"0.0.0.0");
 int err=rhylive_start((port>=1 && port<=65535)?(int)port:0,ip);
 lua_pushboolean(L,err==0); lua_pushinteger(L,err); return 2;
}
static int l_stop(lua_State *L) {rhylive_stop(); return 0;}
static void integer_field(lua_State *L,const char *key,lua_Integer v) {lua_pushinteger(L,v);lua_setfield(L,-2,key);}
static int l_poll(lua_State *L) {
 RhySnapshot s; rhylive_snapshot(&s); lua_createtable(L,0,9);
 lua_pushboolean(L,s.running);lua_setfield(L,-2,"running");
 integer_field(L,"port",s.port);integer_field(L,"error",s.error);
 integer_field(L,"sequence",s.sequence);integer_field(L,"packets",s.packets);integer_field(L,"rejected",s.rejected);
 integer_field(L,"age_ms",s.sequence?(lua_Integer)s.age_ms:2147483647);
 lua_createtable(L,0,FACE_COUNT);
 for(int i=0;i<FACE_COUNT;++i) {lua_pushnumber(L,s.values[i]);lua_setfield(L,-2,names[i]);}
 lua_setfield(L,-2,"values"); return 1;
}
void rhylive_bind_lua(lua_State *L) {
 static const luaL_Reg funcs[]={{"start",l_start},{"stop",l_stop},{"poll",l_poll},{NULL,NULL}};
 luaL_newlib(L,funcs);lua_pushstring(L,"0.1.0");lua_setfield(L,-2,"version");
 lua_setglobal(L,"rhylive_native");
}
EXPORT void reframework_plugin_required_version(REFrameworkPluginVersion *v) {
 v->major=REFRAMEWORK_PLUGIN_VERSION_MAJOR;v->minor=REFRAMEWORK_PLUGIN_VERSION_MINOR;
 v->patch=REFRAMEWORK_PLUGIN_VERSION_PATCH;v->game_name="RE4";
}
EXPORT bool reframework_plugin_initialize(const REFrameworkPluginInitializeParam *p) {
 if(!p || !p->functions || !p->functions->on_lua_state_created) return false;
 bool ok=p->functions->on_lua_state_created(rhylive_bind_lua);
 if(ok && p->functions->log_info) p->functions->log_info("[RhyLiveFace] 0.1.0 ready; receiver starts from Lua UI");
 return ok;
}
