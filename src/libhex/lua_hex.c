#define _GNU_SOURCE
#include "hex/lua.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <alloca.h>
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
		/* For each argument, we check if it is a table */
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

static void
hex_print_command(lua_State *L, int top, char **argv) {
	int silent;

	/* Get verbosity */
	lua_getglobal(L, "hex");
	lua_getfield(L, -1, "silent");
	silent = lua_toboolean(L, -1);
	lua_settop(L, top);

	/* Print command if not silent */
	if (!silent) {
		const int last = top - 1;
		int i = 0;

		while (i < last) {
			fputs(argv[i], stdout);
			fputc(' ', stdout);
			i++;
		}
		fputs(argv[i], stdout);
		fputc('\n', stdout);
		fflush(stdout);
	}
}

static int
lua_hex_cast(lua_State *L) {
	const int top = hex_unpack_arguments(L);
	char *argv[top + 1];

	/* Fill argv */
	for (int i = 0; i < top; i++) {
		size_t length;
		const char *arg = luaL_checklstring(L, i + 1, &length);
		argv[i] = strncpy(alloca(length + 1), arg, length + 1);
	}
	argv[top] = NULL;

	hex_print_command(L, top, argv);

	const pid_t pid = fork();
	switch (pid) {
	case 0:
		execvp(*argv, argv);
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
	char *argv[top + 1];

	/* Fill argv */
	for (int i = 0; i < top; i++) {
		size_t length;
		const char *arg = luaL_checklstring(L, i + 1, &length);
		argv[i] = strncpy(alloca(length + 1), arg, length + 1);
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
		execvp(*argv, argv);
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

	const size_t len = luaL_bufflen(&b);
	if (len != 0 && luaL_buffaddr(&b)[len - 1] == '\n') {
		luaL_buffsub(&b, 1);
	}

	luaL_pushresult(&b);

	hex_wait_pid(L, "hex.charm", pid);

	return 1;
}

static int
lua_hex_invoke(lua_State *L) {
	size_t outputlen;
	const char *output = lua_tolstring(L, -1, &outputlen);
	char *filename;

	/* We have no lua GC's guarantee that after lua_pop, output
	 * will still be valid. So we create a local copy */
	if (output != NULL) {
		filename = alloca(outputlen + 1);
		strncpy(filename, output, outputlen + 1);
		lua_pop(L, 1);
	} else {
		filename = NULL;
	}

	const int top = hex_unpack_arguments(L);
	const pid_t pid = fork();

	switch (pid) {
	case 0:
		if (filename != NULL) {
			/* Output redirection into a file */
			int fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0666);

			if (fd < 0) {
				fprintf(stderr, "open %s: %s\n", filename, strerror(errno));
				exit(EXIT_FAILURE);
			}

			if (dup2(fd, STDOUT_FILENO) != STDOUT_FILENO) {
				fprintf(stderr, "dup2 (stdout) %s: %s\n", filename, strerror(errno));
				exit(EXIT_FAILURE);
			}

			if (dup2(fd, STDERR_FILENO) != STDERR_FILENO) {
				fprintf(stderr, "dup2 (stderr) %s: %s\n", filename, strerror(errno));
				exit(EXIT_FAILURE);
			}

			close(fd);
		}

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
	/* Create our names table */
	lua_createtable(L, top, 0);

	/* Rotate the rituals, incantation and names tables at the bottom,
	then pop the global hex table. */
	lua_rotate(L, 1, 3);
	lua_settop(L, top + 3);

	/* Resolve rituals and insert them into incantation table */
	for (int i = top; i > 0; i--) {
		switch (lua_type(L, -1)) {
		case LUA_TSTRING:
			/* Named ritual */
			/* Add value in the names table, must duplicate it on the stack first */
			lua_pushvalue(L, -1);
			lua_rawseti(L, 3, i);
			/* Must look up into the hex.rituals table (stored at position 1 in the stack) */
			if (lua_gettable(L, 1) != LUA_TFUNCTION) {
				return luaL_argerror(L, i, "hex.incantation: Invalid ritual table element");
			}
			break;
		case LUA_TFUNCTION:
			/* Direct ritual or anonymous one, directly set it, no name available */
			break;
		default:
			return luaL_argerror(L, i, "hex.incantation: Invalid ritual argument element");
		}

		lua_rawseti(L, 2, i);
	}

	/* Only return the incantation and names tables */
	return 2;
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

static int
hex_preprocess(lua_State *L, int fd, FILE *output) {
	enum preprocessor_state {
		PREPROCESSOR_STATE_PRINT,
		PREPROCESSOR_STATE_BEGIN_AT,
		PREPROCESSOR_STATE_FILLS_AT,
	} state = PREPROCESSOR_STATE_PRINT;
	char buffer[getpagesize()];
	ssize_t readval;

	/* Read as many as we can */
	while (readval = read(fd, buffer, sizeof (buffer)), readval > 0) {
		const char *current = buffer;
		size_t left = readval;

		/* Until everything read is done, execute state. */
		while (left != 0) {
			switch (state) {
			case PREPROCESSOR_STATE_PRINT: {
				/* Forward read data directly onto the output */
				const char *at = memchr(current, '@', left);
				size_t n;

				if (at != NULL) {
					state = PREPROCESSOR_STATE_BEGIN_AT;
					n = at - current;
				} else {
					n = left;
				}

				if (fwrite(current, sizeof (*current), n, output) != n) {
					return -1;
				}

				current += n;
				left -= n;
			} break;
			case PREPROCESSOR_STATE_BEGIN_AT:
				/* Add a new accumulator string on the stack */
				state = PREPROCESSOR_STATE_FILLS_AT;
				lua_pushliteral(L, "");
				current++;
				left--;
				break;
			case PREPROCESSOR_STATE_FILLS_AT: {
				/* Concatenate what we have with the accumulator string,
				 * if we have what we need, access the table left at index 3,
				 * and print its content if one is available, if none available,
				 * nothing is to be printed. */
				const char *at = memchr(current, '@', left);
				size_t n;

				if (at != NULL) {
					const char *value;
					size_t length;

					state = PREPROCESSOR_STATE_PRINT;
					n = at - current;

					lua_pushlstring(L, current, n);
					lua_concat(L, 2);
					lua_rawget(L, 3);
					value = lua_tolstring(L, -1, &length);

					if (value != NULL && fwrite(value, sizeof (*value), length, output) != length) {
						return -1;
					}

					lua_settop(L, 3);
					n++;
				} else {
					n = left;
					lua_pushlstring(L, current, n);
					lua_concat(L, 2);
				}

				current += n;
				left -= n;
			} break;
			}
		}
	}

	return 0;
}

static int
lua_hex_preprocess(lua_State *L) {
	const char *source = luaL_checkstring(L, 1);
	const char *destination = luaL_checkstring(L, 2);
	int haserror = 0;

	luaL_checktype(L, 3, LUA_TTABLE);

	lua_getglobal(L, "report");
	lua_getfield(L, -1, "preprocess");
	lua_pushvalue(L, 1);
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_call(L, 3, 0);
	lua_settop(L, 3);

	const int fd = open(source, O_RDONLY);
	if (fd >= 0) {
		FILE * const output = fopen(destination, "w");

		if (output != NULL) {

			if (hex_preprocess(L, fd, output) != 0) {
				lua_pushfstring(L, "hex.preprocess: Unable to process '%s' into '%s': %s", source, destination, strerror(errno));
				haserror = 1;
			}

			fclose(output);
		} else {
			lua_pushfstring(L, "hex.preprocess: fopen '%s': %s", destination, strerror(errno));
			haserror = 1;
		}

		close(fd);
	} else {
		lua_pushfstring(L, "hex.preprocess: open '%s': %s", source, strerror(errno));
		haserror = 1;
	}

	if (haserror != 0) {
		return lua_error(L);
	}

	return 0;
}

static int
lua_hex_dofile(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);
	const int top = lua_gettop(L);

	if (luaL_loadfile(L, filename) != LUA_OK) {
		return luaL_error(L, "hex.dofile: Unable to load file '%s': %s", filename, strerror(errno));
	}

	lua_rotate(L, 1, -1); /* Put the filename on the top */
	lua_settop(L, top); /* Remove filename from the top */
	lua_rotate(L, 1, 1); /* Set the function on the bottom */

	lua_call(L, top - 1, LUA_MULTRET);

	return lua_gettop(L); /* Forward all returned values if no error occured */
}

static const luaL_Reg hex_funcs[] = {
	{ "exit",        lua_hex_exit },
	{ "cast",        lua_hex_cast },
	{ "charm",       lua_hex_charm },
	{ "invoke",      lua_hex_invoke },
	{ "incantation", lua_hex_incantation },
	{ "preprocess",  lua_hex_preprocess },
	{ "hinderuser",  lua_hex_hinderuser },
	{ "dofile",      lua_hex_dofile },
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
