#include "hex/lua.h"

static int
lua_report_log_incantation(lua_State *L) {
	const int top = lua_gettop(L);

	if (top != 1) {
		return luaL_error(L, "report-log.incantation: Expected 1 argument, found %d", top);
	}

	lua_getglobal(L, "log");
	lua_getfield(L, -1, "notice");
	lua_pushliteral(L, "Performing incantation for ");
	lua_rotate(L, 1, -1);
	lua_call(L, 2, 0);

	return 0;
}

static int
lua_report_log_invocation(lua_State *L) {
	const int top = lua_gettop(L);

	if (top != 2) {
		return luaL_error(L, "report-log.incantation: Expected 2 arguments, found %d", top);
	}

	lua_getglobal(L, "log");
	lua_getfield(L, -1, "info");
	lua_pushliteral(L, "Invocation of ");
	lua_rotate(L, 1, -2);
	lua_pushliteral(L, " ");
	lua_rotate(L, -2, 1);
	lua_call(L, 4, 0);

	return 0;
}

static int
lua_report_log_copy(lua_State *L) {
	const int top = lua_gettop(L);

	if (top != 2) {
		return luaL_error(L, "report-log.copy: Expected 2 arguments, found %d", top);
	}

	lua_getglobal(L, "log");
	lua_getfield(L, -1, "info");
	lua_pushliteral(L, "Copying file(s) from ");
	lua_rotate(L, 1, -2);
	lua_pushliteral(L, " to ");
	lua_rotate(L, -2, 1);
	lua_call(L, 4, 0);

	return 0;
}

static int
lua_report_log_remove(lua_State *L) {
	const int top = lua_gettop(L);

	if (top == 0) {
		lua_pushliteral(L, "report-log.copy: Expected at least 1 argument, found none");
		return lua_error(L);
	}

	lua_getglobal(L, "log");
	lua_getfield(L, -1, "info");
	lua_pushliteral(L, "Removing file(s) at ");
	lua_rotate(L, 1, -1);
	for (int separators = top - 1; separators != 0; separators--) {
		lua_pushliteral(L, ", ");
		lua_rotate(L, 1, -1);
	}
	lua_call(L, 2 * top, 0);

	return 0;
}

static int
lua_report_log_preprocess(lua_State *L) {
	const int top = lua_gettop(L);

	if (top != 3) {
		return luaL_error(L, "report-log.copy: Expected 3 arguments, found %d", top);
	}

	lua_getglobal(L, "log");
	lua_getfield(L, -1, "info");
	lua_pushliteral(L, "Preprocessing ");
	lua_rotate(L, 1, -2);
	lua_pushliteral(L, " into ");
	lua_rotate(L, -2, 1);
	lua_call(L, 4, 0);

	return 0;
}

static int
lua_report_log_failure(lua_State *L) {
	const int top = lua_gettop(L);

	if (top != 1) {
		return luaL_error(L, "report-log.failure: Expected 1 argument, found %d", top);
	}

	lua_getglobal(L, "log");
	lua_getfield(L, -1, "error");
	lua_rotate(L, 1, -1);
	lua_call(L, 1, 0);

	return 0;
}

static const luaL_Reg report_log_funcs[] = {
	{ "incantation", lua_report_log_incantation },
	{ "invocation",  lua_report_log_invocation },
	{ "copy",        lua_report_log_copy },
	{ "remove",      lua_report_log_remove },
	{ "preprocess",  lua_report_log_preprocess },
	{ "failure",     lua_report_log_failure },
	{ NULL, NULL }
};

int
luaopen_report_log(lua_State *L) {

	luaL_newlib(L, report_log_funcs);

	return 1;
}
