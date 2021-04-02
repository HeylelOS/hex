#ifndef HEX_LUA_H
#define HEX_LUA_H

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

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

extern const char hex_runtime[];
extern const unsigned long hex_runtime_size;

/* HEX_LUA_H */
#endif
