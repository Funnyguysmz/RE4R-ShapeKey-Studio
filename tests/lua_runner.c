#include <stdio.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
/* Minimal file runner for library-only Lua source bundles; no interactive REPL. */
int main(int argc,char **argv){
    if(argc<2)return 2;lua_State *L=luaL_newstate();if(!L)return 3;luaL_openlibs(L);
    lua_newtable(L);for(int i=1;i<argc;i++){lua_pushstring(L,argv[i]);lua_rawseti(L,-2,i-1);}lua_setglobal(L,"arg");
    int e=luaL_loadfile(L,argv[1]);if(!e)e=lua_pcall(L,0,LUA_MULTRET,0);
    if(e)fprintf(stderr,"%s\n",lua_tostring(L,-1));lua_close(L);return e?1:0;
}
