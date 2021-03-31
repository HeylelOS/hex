#include "hex/lua.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#define ANSI_ESC "\x1B"
#define ANSI_CSI ANSI_ESC "["
#define ANSI_SGR(n) ANSI_CSI n "m"

#define ANSI_SGR_RESET ANSI_SGR("0")
#define ANSI_SGR_BOLD(color) ANSI_SGR("1;" color)

#define ANSI_COLOR_BLACK   "30"
#define ANSI_COLOR_RED     "31"
#define ANSI_COLOR_GREEN   "32"
#define ANSI_COLOR_YELLOW  "33"
#define ANSI_COLOR_BLUE    "34"
#define ANSI_COLOR_MAGENTA "35"
#define ANSI_COLOR_CYAN    "36"
#define ANSI_COLOR_WHITE   "37"

enum log_level {
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_NOTICE,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_ERROR,
};

static const char * const log_levels[] = {
	"debug",
	"info",
	"notice",
	"warning",
	"error",
	NULL,
};

static const char * const log_fancy[] = {
	ANSI_SGR_BOLD(ANSI_COLOR_MAGENTA), /* debug */
	ANSI_SGR_BOLD(ANSI_COLOR_WHITE), /* info */
	ANSI_SGR_BOLD(ANSI_COLOR_CYAN), /* notice */
	ANSI_SGR_BOLD(ANSI_COLOR_YELLOW), /* warning */
	ANSI_SGR_BOLD(ANSI_COLOR_RED), /* error */
};

static int
lua_log_print(lua_State *L) {
	const enum log_level level = luaL_checkoption(L, 1, NULL, log_levels);
	const int top = lua_gettop(L);

	if (top == 1) {
		return luaL_argerror(L, 2, "log.print: Expected at least one string to log, found none");
	}

	lua_concat(L, top - 1);

	const char *string = luaL_checkstring(L, -1);

	if (isatty(STDERR_FILENO) == 1) {
		fprintf(stderr, "[%s%s"ANSI_SGR_RESET"]: %s\n", log_fancy[level], log_levels[level], string);
	} else {
		fprintf(stderr, "[%s]: %s\n", log_levels[level], string);
	}

	return 0;
}

static const luaL_Reg log_funcs[] = {
	{ "print",   lua_log_print },
	{ NULL, NULL }
};

int
luaopen_log(lua_State *L) {

	luaL_newlib(L, log_funcs);

	return 1;
}
