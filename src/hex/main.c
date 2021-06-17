#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "hex/lua.h"

static const char version[] =
	"Hex - Copyright (C) 2021, Valentin Debon\n"
	LUA_COPYRIGHT"\n"
;

struct hex_args {
	const char *progname;
	const char *report;
	bool silent;
};

static void
hex_usage(const struct hex_args *args, int status) {
	fprintf(stderr, "usage: %s [-hs] [-H <report>] [-C <dir>] rituals...\n", args->progname);
	exit(status);
}

static const struct hex_args
hex_parse_args(int argc, char **argv) {
	const char *workdir = NULL;
	struct hex_args args = {
		.progname = strrchr(*argv, '/'),
		.report = NULL,
		.silent = false,
	};
	int c;

	if (args.progname == NULL) {
		args.progname = *argv;
	} else {
		args.progname++;
	}

	while (c = getopt(argc, argv, ":hsH:C:"), c != -1) {
		switch (c) {
		case 'h':
			fputs(version, stdout);
			hex_usage(&args, EXIT_SUCCESS);
		case 's':
			args.silent = true;
			break;
		case 'H':
			args.report = optarg;
			break;
		case 'C':
			workdir = optarg;
			break;
		case ':':
			fprintf(stderr, "%s: -%c: Missing argument\n", args.progname, optopt);
			hex_usage(&args, EXIT_FAILURE);
		default:
			fprintf(stderr, "%s: Unknown argument -%c\n", args.progname, optopt);
			hex_usage(&args, EXIT_FAILURE);
		}
	}

	if (args.report == NULL) {
		if (isatty(STDERR_FILENO) != 0) {
			args.report = "log";
		} else {
			args.report = "none";
		}
	}

	if (argc == optind) {
		fprintf(stderr, "%s: Missing input file(s)\n", args.progname);
		hex_usage(&args, EXIT_FAILURE);
	}

	if (workdir != NULL) {
		if (chdir(workdir) != 0) {
			err(EXIT_FAILURE, "chdir %s", workdir);
		}
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
	err(EXIT_FAILURE, "Congratulation, you managed to panic the interpreter!: %s\n", luaL_checkstring(L, -1));
}

static bool
hex_lua_runtime_init(lua_State *L, const struct hex_args *args) {
	/*********************
	 * Generic lua setup *
	 *********************/
	luaL_checkversion(L);
	lua_openlibs(L);
	lua_atpanic(L, hex_lua_panic);

	/******************
	 * Report library *
	 ******************/
	static const luaL_Reg reportlibraries[] = {
		{ "report-none", luaopen_report_none },
		{ "report-log", luaopen_report_log },
	};
	static const luaL_Reg * const reportlibrariesend = reportlibraries + sizeof (reportlibraries) / sizeof (*reportlibraries);
	const luaL_Reg *reportlibrary = reportlibraries;

	while (reportlibrary != reportlibrariesend
		&& strcmp(reportlibrary->name + 7, args->report) != 0) {
		reportlibrary++;
	}

	if (reportlibrary == reportlibrariesend) {
		fprintf(stderr, "%s: %s: Invalid report type\n", args->progname, args->report);
		hex_usage(args, EXIT_FAILURE);
	}

	luaL_requiref(L, reportlibrary->name, reportlibrary->func, 1);
	lua_setglobal(L, "report");

	/**************************
	 * Check if hex is silent *
	 **************************/
	if (args->silent) {
		lua_getglobal(L, "hex");
		lua_pushboolean(L, args->silent);
		lua_setfield(L, -2, "silent");
		lua_pop(L, 1);
	}

	/****************************
	 * Loading extended runtime *
	 ****************************/
	switch (luaL_loadbufferx(L, hex_runtime, hex_runtime_size, args->progname, "b")) {
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
				lua_getglobal(L, "report");
				lua_getfield(L, -1, "failure");
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
