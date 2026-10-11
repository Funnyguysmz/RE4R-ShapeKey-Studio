#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "API.h"
#include "lua.h"
#include "lauxlib.h"
#define EXPORT __declspec(dllexport)
#ifdef STUDIO_TEST_PIPE
#define PIPE_NAME L"\\\\.\\pipe\\RE4R.ShapeKey.Desktop.Test.v1"
#else
#define PIPE_NAME L"\\\\.\\pipe\\RE4R.ShapeKey.Desktop.v1"
#endif
#define CAP 262144
static SRWLOCK gate=SRWLOCK_INIT;
static char commands[CAP], snapshot[CAP]="S\tWAIT\t0\t3\t1\tWaiting for ShapeKey runtime\t\n";
static size_t command_len;
static ULONGLONG contact;
static HANDLE worker;
/* This thread handles bytes only. All Lua and engine access stays on the game thread. */
static DWORD WINAPI serve(void *unused) {
    (void)unused;
    char *in=HeapAlloc(GetProcessHeap(),0,CAP), *out=HeapAlloc(GetProcessHeap(),0,CAP);
    if(!in || !out) return 1;
    for(;;) {
        HANDLE p=CreateNamedPipeW(PIPE_NAME,PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,CAP,CAP,100,NULL);
        if(p==INVALID_HANDLE_VALUE) {Sleep(500);continue;}
        if(ConnectNamedPipe(p,NULL) || GetLastError()==ERROR_PIPE_CONNECTED) {
            DWORD n=0,w=0;
            if(ReadFile(p,in,CAP-1,&n,NULL) && n<CAP) {
                in[n]=0;
                AcquireSRWLockExclusive(&gate);
                /* Never silently drop a partial command. On overflow, reject the entire batch. */
                if(n>0 && in[n-1]=='\n' && command_len+n<CAP) {
                    memcpy(commands+command_len,in,n);command_len+=n;commands[command_len]=0;
                }
                contact=GetTickCount64();
                size_t len=strlen(snapshot);memcpy(out,snapshot,len+1);
                ReleaseSRWLockExclusive(&gate);
                if(WriteFile(p,out,(DWORD)len,&w,NULL)) FlushFileBuffers(p);
            }
        }
        DisconnectNamedPipe(p);CloseHandle(p);
    }
}
static int poll(lua_State *L) {
    AcquireSRWLockExclusive(&gate);
    lua_pushlstring(L,commands,command_len);command_len=0;commands[0]=0;
    ReleaseSRWLockExclusive(&gate);return 1;
}
static int publish(lua_State *L) {
    size_t n;const char *s=luaL_checklstring(L,1,&n);
    if(n>=CAP) return luaL_error(L,"snapshot exceeds bridge limit");
    AcquireSRWLockExclusive(&gate);memcpy(snapshot,s,n);snapshot[n]=0;ReleaseSRWLockExclusive(&gate);return 0;
}
static int connected(lua_State *L) {
    AcquireSRWLockShared(&gate);ULONGLONG t=contact;ReleaseSRWLockShared(&gate);
    lua_pushboolean(L,t && GetTickCount64()-t<2000);return 1;
}
static void bind_lua(lua_State *L) {
    static const luaL_Reg f[]={{"poll",poll},{"publish",publish},{"connected",connected},{NULL,NULL}};
    luaL_newlib(L,f);lua_setglobal(L,"shapekey_desktop");
}
EXPORT void reframework_plugin_required_version(REFrameworkPluginVersion *v) {
    v->major=1;v->minor=15;v->patch=0;v->game_name="RE4";
}
EXPORT bool reframework_plugin_initialize(const REFrameworkPluginInitializeParam *p) {
    if(!p || !p->functions || !p->functions->on_lua_state_created) return false;
    if(!p->functions->on_lua_state_created(bind_lua)) return false;
    worker=CreateThread(NULL,0,serve,NULL,0,NULL);
    return worker!=NULL;
}
