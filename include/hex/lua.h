#ifndef HEX_LUA_H
#define HEX_LUA_H

#include <lua5.3/lua.h>
#include <lua5.3/lauxlib.h>
#include <lua5.3/lualib.h>

int
luaopen_fs(lua_State *L);

int
luaopen_env(lua_State *L);

int
luaopen_log(lua_State *L);

int
luaopen_hash(lua_State *L);

int
luaopen_hex(lua_State *L);

/* HEX_LUA_H */
#endif
