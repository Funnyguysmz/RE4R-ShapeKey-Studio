#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "API.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
bool reframework_plugin_initialize(const REFrameworkPluginInitializeParam*);
static REFLuaStateCreatedCb callback;
static bool register_created(REFLuaStateCreatedCb f){callback=f;return true;}
int main(void) {
    REFrameworkPluginFunctions funcs={0};funcs.on_lua_state_created=register_created;
    REFrameworkPluginInitializeParam p={0};p.functions=&funcs;
    if(!reframework_plugin_initialize(&p) || !callback)return 1;
    lua_State *L=luaL_newstate();luaL_openlibs(L);callback(L);
    if(luaL_dostring(L,"assert(shapekey_desktop);shapekey_desktop.publish('S\\tTEST\\t0\\t1\\t1\\tReady\\tMock\\n')"))return 2;
    Sleep(150);
    HANDLE pipe=CreateFileW(L"\\\\.\\pipe\\RE4R.ShapeKey.Desktop.Test.v1",GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    if(pipe==INVALID_HANDLE_VALUE)return 3;
    const char *cmd="C\tTEST\tset\tjawOpen\t0.750\n";DWORD n=0;char b[1024];
    if(!WriteFile(pipe,cmd,(DWORD)strlen(cmd),&n,NULL))return 4;
    if(!ReadFile(pipe,b,sizeof(b)-1,&n,NULL))return 5;b[n]=0;
    if(strncmp(b,"S\tTEST",6))return 6;CloseHandle(pipe);
    if(luaL_dostring(L,"assert(shapekey_desktop.connected()); assert(shapekey_desktop.poll()=='C\\tTEST\\tset\\tjawOpen\\t0.750\\n');assert(shapekey_desktop.poll()=='')"))return 7;
    wchar_t exe[MAX_PATH],child_cmd[2*MAX_PATH];GetModuleFileNameW(NULL,exe,MAX_PATH);
    wchar_t *slash=wcsrchr(exe,L'\\');if(!slash)return 9;*slash=0;slash=wcsrchr(exe,L'\\');if(!slash)return 9;*slash=0;
    swprintf(child_cmd,2*MAX_PATH,L"\"%ls\\package\\RE4R_ShapeKeyStudio.exe\" --test-probe",exe);
    STARTUPINFOW si={sizeof(si)};PROCESS_INFORMATION child={0};
    if(!CreateProcessW(NULL,child_cmd,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,NULL,&si,&child))return 10;
    if(WaitForSingleObject(child.hProcess,5000)!=WAIT_OBJECT_0)return 11;
    DWORD exitcode=0;GetExitCodeProcess(child.hProcess,&exitcode);CloseHandle(child.hProcess);CloseHandle(child.hThread);
    if(exitcode)return 12;
    if(luaL_dostring(L,"assert(shapekey_desktop.poll()=='PING\\n')"))return 13;
    Sleep(2050);if(luaL_dostring(L,"assert(not shapekey_desktop.connected())"))return 8;
    lua_close(L);puts("BRIDGE_PIPE_LUA_AND_DESKTOP_CLIENT_PASS");return 0;
}
