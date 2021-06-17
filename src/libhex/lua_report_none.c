#include "hex/lua.h"

static int
lua_report_none_incantation(lua_State *L) {
	return 0;
}

static int
lua_report_none_invocation(lua_State *L) {
	return 0;
}

static int
lua_report_none_failure(lua_State *L) {
	return 0;
}

static const luaL_Reg report_none_funcs[] = {
	{ "incantation", lua_report_none_incantation },
	{ "invocation",  lua_report_none_invocation },
	{ "failure",     lua_report_none_failure },
	{ NULL, NULL }
};

int
luaopen_report_none(lua_State *L) {

	luaL_newlib(L, report_none_funcs);

	return 1;
}

