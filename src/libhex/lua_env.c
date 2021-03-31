#include "hex/lua.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int
lua_env_get(lua_State *L) {

	lua_pushstring(L, getenv(luaL_checkstring(L, 1)));

	return 1;
}

static int
lua_env_set(lua_State *L) {
	const char * const name = luaL_checkstring(L, 1);
	const char * const value = lua_tostring(L, 2);

	if (value != NULL) {
		/* It is kinda unusual for people used to shells to force the overwrite,
		let the possibility but switch the argument (must specify no overwrite) */
		const int overwrite = lua_toboolean(L, 3) == 0;

		if (setenv(name, value, overwrite) != 0) {
			static const char * const overwrite_strings[2] = {
				"(no overwrite)", "(overwrite)"
			};

			return luaL_error(L, "env.set: setenv %s %s %s: %s", name, value, overwrite_strings[overwrite], strerror(errno));
		}
	} else {
		if (unsetenv(name) != 0) {
			return luaL_error(L, "env.set: unsetenv %s: %s", name, strerror(errno));
		}
	}

	return 0;
}

static int
lua_env_clear(lua_State *L) {

	if (clearenv() != 0) {
		lua_pushliteral(L, "env.clear: Unable to clear environment");
		return lua_error(L);
	}

	return 0;
}

static const luaL_Reg env_funcs[] = {
	{ "get",   lua_env_get },
	{ "set",   lua_env_set },
	{ "clear", lua_env_clear },
	{ NULL, NULL }
};

int
luaopen_env(lua_State *L) {

	luaL_newlib(L, env_funcs);

	return 1;
}
