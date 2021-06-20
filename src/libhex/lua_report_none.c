#include "hex/lua.h"

static int
lua_report_nothing(lua_State *L) {
	return 0;
}

static const luaL_Reg report_none_funcs[] = {
	{ "incantation", lua_report_nothing },
	{ "invocation",  lua_report_nothing },
	{ "copy",        lua_report_nothing },
	{ "remove",      lua_report_nothing },
	{ "failure",     lua_report_nothing },
	{ NULL, NULL }
};

int
luaopen_report_none(lua_State *L) {

	luaL_newlib(L, report_none_funcs);

	return 1;
}

