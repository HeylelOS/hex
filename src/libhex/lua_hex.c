#define _GNU_SOURCE
#include "hex/lua.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>

#include <assert.h>

static int
lua_hex_exit(lua_State *L) {
	static const char *statuses[] = {
		"success", "failure", NULL
	};
	int status;

	switch (lua_type(L, 1)) {
	case LUA_TNONE:
	case LUA_TNIL:
		status = EXIT_SUCCESS;
		break;
	case LUA_TNUMBER:
		status = luaL_checkinteger(L, 1);
		break;
	case LUA_TBOOLEAN:
		status = !lua_toboolean(L, 1);
		break;
	case LUA_TSTRING:
		status = luaL_checkoption(L, 1, NULL, statuses);
		break;
	default:
		luaL_argerror(L, 1, "Invalid type");
		break;
	}

	exit(status);
}

static bool
hex_unpack_arguments(lua_State *L) {
	const bool valid = lua_gettop(L) == 1;

	if (valid) {
		int i = 1;

		while (lua_rawgeti(L, 1, i) != LUA_TNIL) {
			i++;
		}

		lua_rotate(L, 1, -1);
		lua_settop(L, i - 1);
	}

	return valid;
}

static int
hex_wait_pid(lua_State *L, pid_t pid) {
	int status;

	/* Wait for process termination, and fail if failure */
	assert(waitpid(pid, &status, 0) == pid);

	if (WIFSIGNALED(status)) {
		const int signo = WTERMSIG(status);
		return luaL_error(L, "hex: Terminated with signal %d (%s)", signo, strsignal(signo));
	}

	if (WIFEXITED(status)) {
		const int exitstatus = WEXITSTATUS(status);
		if (exitstatus != 0) {
			return luaL_error(L, "hex: Exited with code %d", exitstatus);
		}
	}

	return 0;
}

static int
lua_hex_cast(lua_State *L) {

	/* The first argument determines the semantic of the cast */
	switch (lua_type(L, 1)) {
	case LUA_TSTRING:
		/* Arguments only semantic, everything is already on the stack */
		break;
	case LUA_TTABLE:
		/* Array semantic, must unpack content on stack */
		if (!hex_unpack_arguments(L)) {
			lua_pushliteral(L, "hex.cast: Array semantic expects one argument");
			return lua_error(L);
		}
		break;
	case LUA_TFUNCTION:
		/* Function evaluation semantic, just execute it */
		lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
		break;
	default:
		return luaL_argerror(L, 1, "hex.cast: Invalid semantic");
	}

	/* Fill the arguments table */
	const int top = lua_gettop(L);
	const char *argv[top + 1];
	for (int i = 0; i < top; i++) {
		argv[i] = luaL_checkstring(L, i + 1);
	}
	argv[top] = NULL;

	const pid_t pid = fork();
	switch (pid) {
	case 0:
		/* We cast const char *argv[] to char *argv[] because we're in the child anyway,
		and I can't really believe execve does modify the argument array */
		execvp(argv[0], (void *)argv);
		perror("execve");
		exit(-1);
	case -1:
		return luaL_error(L, "hex.cast: fork: %s", strerror(errno));
	default:
		break;
	}

	return hex_wait_pid(L, pid);
}

static int
lua_hex_invoke(lua_State *L) {

	/* The first argument determines the semantic of the invocation */
	if (lua_type(L, 1) == LUA_TTABLE) {
		/* Array semantic, must unpack content on stack */
		if (!hex_unpack_arguments(L)) {
			lua_pushliteral(L, "hex.invoke: Array semantic expects one argument");
			return lua_error(L);
		}
	}

	const pid_t pid = fork();
	switch (pid) {
	case 0: {
		const int top = lua_gettop(L);

		for (int i = 1; i <= top; i++) {
			/* We guarantee the first argument to be the first one
			executed, so each new argument is rotated on the top to be directly
			removed after its call */
			lua_rotate(L, 1, -1);
			lua_call(L, 0, 0);
		}

	}	exit(EXIT_SUCCESS);
	case -1:
		return luaL_error(L, "hex.invoke: fork: %s", strerror(errno));
	default:
		break;
	}

	return hex_wait_pid(L, pid);
}

static int
lua_hex_incantation(lua_State *L) {

	/* The first argument determines the semantic of the invocation */
	if (lua_type(L, 1) == LUA_TTABLE) {
		/* Array semantic, must unpack content on stack */
		if (!hex_unpack_arguments(L)) {
			lua_pushliteral(L, "hex.incantation: Array semantic expects one argument");
			return lua_error(L);
		}
	}

	/* Keep number of rituals composing incantation */
	const int top = lua_gettop(L);

	/* Get the rituals table */
	lua_getglobal(L, "hex");
	if (lua_getfield(L, -1, "rituals") != LUA_TTABLE) {
		lua_pushliteral(L, "hex.incantation: Rituals table isn't a table");
		return lua_error(L);
	}

	/* Create our incantation table */
	lua_createtable(L, top, 0);

	/* Rotate the rituals and incantation table at the bottom,
	then pop the global hex table. */
	lua_rotate(L, 1, 2);
	lua_settop(L, top + 2);

	/* Resolve rituals and insert them into incantation table */
	for (int i = top; i > 0; i--) {
		switch (lua_type(L, -1)) {
		case LUA_TSTRING:
			/* Named ritual, must look up into the hex.rituals table
			(stored at position 1 in the stack) */
			if (lua_gettable(L, 1) != LUA_TFUNCTION) {
				return luaL_argerror(L, i, "hex.incantation: Invalid ritual table element");
			}
			break;
		case LUA_TFUNCTION:
			/* Direct ritual or anonymous one, directly set it */
			break;
		default:
			return luaL_argerror(L, i, "hex.incantation: Invalid ritual argument element");
		}

		lua_rawseti(L, 2, i);
	}

	/* Only return the incantation table */
	return 1;
}

static id_t
hex_getid_field(lua_State *L, int position, const char *field) {
	/* The table to get from should be at position,
	we should return a balanced stack */
	id_t id;

	lua_getfield(L, position, field);
	id = luaL_checkinteger(L, -1);
	lua_pop(L, 1);

	return id;
}

static int
hex_id_map(const char *path, id_t newid, id_t oldid) {
	const int fd = open(path, O_WRONLY);
	int retval = 0;

	if (fd >= 0) {
		static const char format[] = "%d %d 1";
		const int length = snprintf(NULL, 0, format, newid, oldid) + 1;
		char buffer[length];

		assert(snprintf(buffer, length, format, newid, oldid) < length);
		if (write(fd, buffer, length) != length) {
			retval = -1;
		}

		close(fd);
	} else {
		retval = -1;
	}

	return retval;
}

static int
hex_deny(const char *path) {
	const int fd = open(path, O_WRONLY);
	int retval = 0;

	if (fd >= 0) {
		static const char deny[] = "deny";
		const size_t size = sizeof (deny) - 1;

		if (write(fd, deny, size) != size) {
			retval = -1;
		}

		close(fd);
	} else {
		retval = -1;
	}

	return retval;
}

static int
lua_hex_hinderuser(lua_State *L) {
#ifdef __linux__
	/* We must keep ids now, because after unshare(2) unmapped ids
	   are resolved to the overflow user/group id */
	const uid_t uid = getuid();
	const gid_t gid = getgid();

	/* Create new user namespace */
	if (unshare(CLONE_NEWUSER | CLONE_NEWNS) != 0) {
		return luaL_error(L, "hex.hinderuser: unshare: %s", strerror(errno));
	}

	/* Map our user id to specified id or root */
	const uid_t newuid = hex_getid_field(L, 1, "uid");
	if (hex_id_map("/proc/self/uid_map", newuid, uid) != 0) {
		return luaL_error(L, "hex.hinderuser: Unable to map user id %d to %d: %s", newuid, uid, strerror(errno));
	}

	/* Only a process with CAP_SYSADMIN is allowed to maintain setgroups to "allow"
	 * in a new namespace, "deny" setgroups for every hex invocations. */
	if (hex_deny("/proc/self/setgroups") != 0) {
		return luaL_error(L, "hex.hinderuser: Unable to deny setgroups: %s", strerror(errno));
	}

	/* Map our user id to specified id or zero too */
	const gid_t newgid = hex_getid_field(L, 1, "gid");
	if (hex_id_map("/proc/self/gid_map", newgid, gid) != 0) {
		return luaL_error(L, "hex.hinderuser: Unable to map group id %d to %d: %s", newgid, gid, strerror(errno));
	}
#else
#warning "hex.hinderuser is not supported, user isolation will not be available"
	lua_getglobal(L, "log");
	lua_getfield(L, -1, "warning");
	lua_pushliteral(L, "User isolation is not supported on this platform!");
	lua_call(L, 1, 0);
#endif

	return 0;
}

static const luaL_Reg hex_funcs[] = {
	{ "exit",        lua_hex_exit },
	{ "cast",        lua_hex_cast },
	{ "invoke",      lua_hex_invoke },
	{ "incantation", lua_hex_incantation },
	{ "hinderuser",  lua_hex_hinderuser },
	{ NULL, NULL }
};

int
luaopen_hex(lua_State *L) {

	luaL_newlib(L, hex_funcs);

	lua_pushliteral(L, "rituals");
	lua_newtable(L);
	lua_rawset(L, -3);

	return 1;
}
