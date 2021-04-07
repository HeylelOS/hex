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
		return luaL_argerror(L, 1, "Invalid type");
	}

	exit(status);
}

static int
hex_unpack_arguments(lua_State *L) {
	int arg = 1, top = lua_gettop(L);

	while (arg <= top) {
		/* For, each argument, we check if it is a table */
		if (lua_type(L, arg) == LUA_TTABLE) {
			int i = 1;

			/* We unpack its content at the top of the stack */
			while (lua_rawgeti(L, arg, i) != LUA_TNIL) {
				i++;
			}

			/* Rotate unpacked at arg position */
			lua_rotate(L, arg, i);
			/* Recalibrate arg, points to nil */
			arg += i - 1;
			/* Set nil and unpacked arg top, arg now points to next processed */
			lua_rotate(L, arg, -2);
			/* Recalibrate top */
			top += i - 2;
			/* Pop nil and unpacked arg */
			lua_settop(L, top);
		} else {
			arg++;
		}
	}

	return top;
}

static void
hex_wait_pid(lua_State *L, const char *enchantment, pid_t pid) {
	int status;

	/* Wait for process termination, and fail if failure */
	waitpid(pid, &status, 0);

	if (WIFSIGNALED(status)) {
		const int signo = WTERMSIG(status);
		luaL_error(L, "%s: Terminated with signal %d (%s)", enchantment, signo, strsignal(signo));
	}

	if (WIFEXITED(status)) {
		const int exitstatus = WEXITSTATUS(status);
		if (exitstatus != 0) {
			luaL_error(L, "%s: Exited with code %d", enchantment, exitstatus);
		}
	}
}

static int
lua_hex_cast(lua_State *L) {
	const int top = hex_unpack_arguments(L);
	const char *argv[top + 1];

	/* Fill the arguments table */
	for (int i = 0; i < top; i++) {
		argv[i] = luaL_checkstring(L, i + 1);
	}
	argv[top] = NULL;

	const pid_t pid = fork();
	switch (pid) {
	case 0:
		/* We cast const char *argv[] to char *argv[] because we're in the child anyway,
		and I can't really believe execve does modify the argument array */
		execvp(*argv, (void *)argv);
		fprintf(stderr, "execve %s: %s\n", *argv, strerror(errno));
		exit(-1);
	case -1:
		return luaL_error(L, "hex.cast: fork: %s", strerror(errno));
	default:
		break;
	}

	hex_wait_pid(L, "hex.cast", pid);

	return 0;
}

static int
lua_hex_charm(lua_State *L) {
	const int top = hex_unpack_arguments(L);
	const char *argv[top + 1];

	/* Fill the arguments table */
	for (int i = 0; i < top; i++) {
		argv[i] = luaL_checkstring(L, i + 1);
	}
	argv[top] = NULL;

	/* Until here, it was the exact same call as lua_hex_cast, but we will redirect
	the new process STOUT_FILENO in a pipe and retrieve its value in a lua buffer */
	int filedes[2];

	if (pipe(filedes) != 0) {
		return luaL_error(L, "hex.charm: fork: %s", strerror(errno));
	}

	const pid_t pid = fork();
	switch (pid) {
	case 0:
		if (dup2(filedes[1], STDOUT_FILENO) != STDOUT_FILENO) {
			fprintf(stderr, "dup2 %s: %s\n", *argv, strerror(errno));
			exit(-1);
		}
		close(filedes[0]);
		close(filedes[1]);
		/* We cast const char *argv[] to char *argv[] because we're in the child anyway,
		and I can't really believe execve does modify the argument array */
		execvp(*argv, (void *)argv);
		fprintf(stderr, "execve %s: %s\n", *argv, strerror(errno));
		exit(-1);
	case -1:
		return luaL_error(L, "hex.charm: fork: %s", strerror(errno));
	default:
		break;
	}

	luaL_Buffer b;

	luaL_buffinit(L, &b);
	close(filedes[1]);

	char *buffer;
	ssize_t readval;
	while (buffer = luaL_prepbuffer(&b), readval = read(filedes[0], buffer, LUAL_BUFFERSIZE), readval > 0) {
		luaL_addsize(&b, readval);
	}

	const int errcode = errno;

	close(filedes[0]);

	if (readval < 0) {
		return luaL_error(L, "hex.charm: read: %s", strerror(errcode));
	}

	luaL_pushresult(&b);

	hex_wait_pid(L, "hex.charm", pid);

	return 1;
}

static int
lua_hex_invoke(lua_State *L) {
	const int top = hex_unpack_arguments(L);
	const pid_t pid = fork();

	switch (pid) {
	case 0:
		for (int i = 1; i <= top; i++) {
			/* We guarantee the first argument to be the first one
			executed, so each new argument is rotated on the top to be directly
			removed after its call */
			lua_rotate(L, 1, -1);
			lua_call(L, 0, 0);
		}
		exit(EXIT_SUCCESS);
	case -1:
		return luaL_error(L, "hex.invoke: fork: %s", strerror(errno));
	default:
		break;
	}

	hex_wait_pid(L, "hex.invoke", pid);

	return 0;
}

static int
lua_hex_incantation(lua_State *L) {
	/* Keep number of rituals composing incantation */
	const int top = hex_unpack_arguments(L);

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

		snprintf(buffer, length, format, newid, oldid);

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
	{ "charm",       lua_hex_charm },
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
