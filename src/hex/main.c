#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "hex/lua.h"

static const char version[] =
"Hex - Copyright (C) 2021, Valentin Debon\n"
LUA_COPYRIGHT"\n"
"                   6666666                   \n"
"             666666       666666             \n"
"         6666                   666          \n"
"       666  6                    6 666       \n"
"     666     66               666    66      \n"
"    66       66666         66666      666    \n"
"   66         6   66     66   66       666   \n"
"  66          66    66666    66         666  \n"
" 666           66  66   66  66           66  \n"
" 66            6666       6666           666 \n"
" 66           6666         6666          666 \n"
" 66        666  66         66  666       666 \n"
" 66     666      66       66      666    666 \n"
" 666  666666666666666666666666666666666  66  \n"
"  66               66   66              666  \n"
"   66              66   66             666   \n"
"    66              66 66             666    \n"
"      6666           666           666       \n"
"         6666         6         666          \n"
"            6666666   6   666666             \n"
"                   6666666                   \n"
;

struct hex_args {
	const char *progname;
};

static void
hex_usage(const struct hex_args *args, int status) {
	fprintf(stderr, "usage: %s rituals...\n", args->progname);
	exit(status);
}

static const struct hex_args
hex_parse_args(int argc, char **argv) {
	struct hex_args args = {
		.progname = strrchr(*argv, '/'),
	};
	int c;

	if (args.progname == NULL) {
		args.progname = *argv;
	} else {
		args.progname++;
	}

	while (c = getopt(argc, argv, ":h"), c != -1) {
		switch (c) {
		case 'h':
			fputs(version, stdout);
			hex_usage(&args, EXIT_SUCCESS);
		case ':':
			fprintf(stderr, "%s: -%c: Missing argument\n", args.progname, optopt);
			hex_usage(&args, EXIT_FAILURE);
		default:
			fprintf(stderr, "%s: Unknown argument -%c\n", args.progname, optopt);
			hex_usage(&args, EXIT_FAILURE);
		}
	}

	if (argc == optind) {
		fprintf(stderr, "%s: Missing input file(s)\n", args.progname);
		hex_usage(&args, EXIT_FAILURE);
	}

	return args;
}

static void
lua_openlibs(lua_State *L) {
	static const luaL_Reg libraries[] = {
		/* Lua libraries */
		{ "_G", luaopen_base },
		{ "table", luaopen_table },
		{ "string", luaopen_string },
		{ "utf8", luaopen_utf8 },
		/* Hex libraries */
		{ "fs", luaopen_fs },
		{ "env", luaopen_env },
		{ "log", luaopen_log },
		{ "hash", luaopen_hash },
		{ "hex", luaopen_hex },
	};

	for (const luaL_Reg *library = libraries; library != libraries + sizeof (libraries) / sizeof (*libraries); library++) {
		luaL_requiref(L, library->name, library->func, 1);
		lua_pop(L, 1);
	}
}

static int
hex_lua_panic(lua_State *L) {
	fprintf(stderr, "Congratulation, you managed to panic the interpreter!: %s\n", luaL_checkstring(L, -1));
	exit(EXIT_FAILURE);
}

static bool
hex_lua_runtime_init(lua_State *L, const struct hex_args *args) {
	extern const char hex_lua_runtime_start, hex_lua_runtime_end;

	luaL_checkversion(L);
	lua_openlibs(L);
	lua_atpanic(L, hex_lua_panic);

	switch (luaL_loadbufferx(L, &hex_lua_runtime_start, &hex_lua_runtime_end - &hex_lua_runtime_start, args->progname, "b")) {
	case LUA_OK:
		if (lua_pcall(L, 0, 0, 0) == LUA_OK) {
			return true;
		}
		/* fallthrough */
	default: /* Any error, from luaL_loadbufferx or lua_pcall */
		fprintf(stderr, "%s: %s\n", args->progname, lua_tostring(L, -1));
		lua_pop(L, 1);
		return false;
	}
}

int
main(int argc, char **argv) {
	const struct hex_args args = hex_parse_args(argc, argv);
	char **argpos = argv + optind, ** const argend = argv + argc;
	lua_State * const L = luaL_newstate();
	int retval;

	if (hex_lua_runtime_init(L, &args)) {
		retval = EXIT_SUCCESS;

		while (argpos != argend) {
			const char * const filename = *argpos;

			if (luaL_dofile(L, filename) != LUA_OK) {
				/* NB: If not ok, only the error is pushed on the stack */
				lua_getglobal(L, "log");
				lua_getfield(L, -1, "error");
				lua_rotate(L, 1, -1);
				lua_call(L, 1, 0);
				retval = EXIT_FAILURE;
				break;
			}

			argpos++;
		}
	} else {
		retval = EXIT_FAILURE;
	}

	lua_close(L);

	return retval;
}
