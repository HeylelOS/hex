#include "hex/lua.h"

#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static int
lua_fs_isreg(lua_State *L) {

	lua_pushboolean(L, access(luaL_checkstring(L, 1), F_OK) == 0);

	return 1;
}

static int
lua_fs_isdir(lua_State *L) {
	struct stat st;

	lua_pushboolean(L, stat(luaL_checkstring(L, 1), &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR);

	return 1;
}

static int
lua_fs_isexe(lua_State *L) {

	lua_pushboolean(L, access(luaL_checkstring(L, 1), X_OK) == 0);

	return 1;
}

static int
lua_fs_create(lua_State *L) {
	const char * const path = luaL_checkstring(L, 1);
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

	if (fd < 0) {
		return luaL_error(L, "fs.create: open %s: %s", path, strerror(errno));
	}

	size_t length;
	const char * const string = lua_tolstring(L, 2, &length);

	if (string != NULL && length != 0) {
		const char * current = string, * const end = string + length;

		while (current != end) {
			const ssize_t writeval = write(fd, current, end - current);

			if (writeval < 0) {
				close(fd);
				return luaL_error(L, "fs.create: write %s: %s", path, strerror(errno));
			}

			current += writeval;
		}
	}

	close(fd);

	return 0;
}

static int
lua_fs_read(lua_State *L) {
	const char * const path = luaL_checkstring(L, 1);
	int fd = open(path, O_RDONLY);
	luaL_Buffer b;

	if (fd < 0) {
		return luaL_error(L, "fs.read: open %s: %s", path, strerror(errno));
	}

	luaL_buffinit(L, &b);

	char *buffer;
	ssize_t readval;
	while (buffer = luaL_prepbuffer(&b), readval = read(fd, buffer, LUAL_BUFFERSIZE), readval > 0) {
		luaL_addsize(&b, readval);
	}

	const int errcode = errno;

	close(fd);

	if (readval < 0) {
		return luaL_error(L, "fs.read: read %s: %s", path, strerror(errcode));
	}

	luaL_pushresult(&b);

	return 1;
}

static int
lua_fs_unlink(lua_State *L) {
	const char * const path = luaL_checkstring(L, 1);

	if (unlink(path) != 0) {
		return luaL_error(L, "fs.unlink: unlink %s: %s", path, strerror(errno));
	}

	return 0;
}

static int
lua_fs_mkdir(lua_State *L) {
	const char * const path = luaL_checkstring(L, 1);

	if (mkdir(path, 0777) != 0) {
		return luaL_error(L, "fs.mkdir: mkdir %s: %s", path, strerror(errno));
	}

	return 0;
}

static bool
fs_parent_separator(const char *path, char **separatorp) {
	char *separator = strchr(path, '/');
	bool isend;

	if (separator != NULL) {

		while (separator[1] == '/') {
			separator++;
		}

		*separatorp = separator;

		return separator[1] != '\0';
	} else {
		return false;
	}
}

static int
lua_fs_mkdirs(lua_State *L) {
	size_t length;
	const char * const path = luaL_checklstring(L, 1, &length);
	char buffer[length + 1];

	strncpy(buffer, path, sizeof (buffer));

	char *current = buffer, *separator;
	while (fs_parent_separator(current, &separator)) {
		*separator = '\0';

		if (mkdir(buffer, 0777) != 0 && errno != EEXIST) {
			return luaL_error(L, "fs.mkdirs: mkdir %s: %s", buffer, strerror(errno));
		}

		*separator = '/';
		separator++;

		current = separator;
	}

	if (mkdir(buffer, 0777) != 0) {
		int const errcode = errno;
		struct stat st;

		/* In case of error, or, if it already exists, is not a directory */
		if (errcode != EEXIST || (stat(buffer, &st) == 0 && (st.st_mode & S_IFMT) != S_IFDIR)) {
			return luaL_error(L, "fs.mkdirs: mkdir %s: %s", buffer, strerror(errcode));
		}
	}

	return 0;
}

static int
lua_fs_readdir(lua_State *L) {
	const char * const path = luaL_checkstring(L, 1);
	DIR *dirp = opendir(path);

	if (dirp == NULL) {
		return luaL_error(L, "fs.readdir: opendir %s: %s", path, strerror(errno));
	}

	struct dirent *entry;
	int index = 1;

	lua_newtable(L);
	while (errno = 0, entry = readdir(dirp), entry != NULL) {
		if (*entry->d_name != '.') {
			lua_pushstring(L, entry->d_name);
			lua_rawseti(L, -2, index);
			index++;
		}
	}

	const int errcode = errno;

	closedir(dirp);

	if (errcode != 0) {
		return luaL_error(L, "fs.readdir: readdir %s: %s", path, strerror(errcode));
	}

	return 1;
}

static int
lua_fs_rmdir(lua_State *L) {
	const char * const path = luaL_checkstring(L, 1);

	if (rmdir(path) != 0) {
		return luaL_error(L, "fs.rmdir: rmdir %s: %s", path, strerror(errno));
	}

	return 0;
}

static unsigned long
fs_mount_flags(lua_State *L) {
	static const struct mount_flags {
		const char *name;
		const unsigned long mask;
	} flags[] = {
#ifdef __GLIBC__
		{ "rdonly",      MS_RDONLY },
		{ "nosuid",      MS_NOSUID },
		{ "nodev",       MS_NODEV },
		{ "noexec",      MS_NOEXEC },
		{ "synchronous", MS_SYNCHRONOUS },
		{ "remount",     MS_REMOUNT },
		{ "mandlock",    MS_MANDLOCK },
		{ "dirsync",     MS_DIRSYNC },
		{ "noatime",     MS_NOATIME },
		{ "nodiratime",  MS_NODIRATIME },
		{ "bind",        MS_BIND },
		{ "move",        MS_MOVE },
		{ "rec",         MS_REC },
		{ "silent",      MS_SILENT },
		{ "posixacl",    MS_POSIXACL },
		{ "unbindable",  MS_UNBINDABLE },
		{ "private",     MS_PRIVATE },
		{ "slave",       MS_SLAVE },
		{ "shared",      MS_SHARED },
		{ "relatime",    MS_RELATIME },
		{ "kernmount",   MS_KERNMOUNT },
		{ "i-version",   MS_I_VERSION },
		{ "strictatime", MS_STRICTATIME },
		{ "lazytime",    MS_LAZYTIME },
		{ "active",      MS_ACTIVE },
		{ "nouser",      MS_NOUSER },
#elif defined(__APPLE__)
		{ "rdonly",           MNT_RDONLY },
		{ "synchronous",      MNT_SYNCHRONOUS },
		{ "noexec",           MNT_NOEXEC },
		{ "nosuid",           MNT_NOSUID },
		{ "nodev",            MNT_NODEV },
		{ "union",            MNT_UNION },
		{ "async",            MNT_ASYNC },
		{ "cprotect",         MNT_CPROTECT },
		{ "exported",         MNT_EXPORTED },
		{ "removable",        MNT_REMOVABLE },
		{ "quarantine",       MNT_QUARANTINE },
		{ "local",            MNT_LOCAL },
		{ "quota",            MNT_QUOTA },
		{ "rootfs",           MNT_ROOTFS },
		{ "dovolfs",          MNT_DOVOLFS },
		{ "dontbrowse",       MNT_DONTBROWSE },
		{ "ignore-ownership", MNT_IGNORE_OWNERSHIP },
		{ "automounted",      MNT_AUTOMOUNTED },
		{ "journaled",        MNT_JOURNALED },
		{ "nouserxattr",      MNT_NOUSERXATTR },
		{ "defwrite",         MNT_DEFWRITE },
		{ "multilabel",       MNT_MULTILABEL },
		{ "noatime",          MNT_NOATIME },
		{ "snapshot",         MNT_SNAPSHOT },
		{ "strictatime",      MNT_STRICTATIME },
		{ "update",           MNT_UPDATE },
		{ "noblock",          MNT_NOBLOCK },
		{ "reload",           MNT_RELOAD },
		{ "force",            MNT_FORCE },
#else
#error "Unsupported platform's mount flags"
#endif
	};
	unsigned long mountflags = 0;

	if (lua_type(L, 4) == LUA_TTABLE) {
		const struct mount_flags * const flagsend = flags + sizeof (flags) / sizeof (*flags);
		int i = 1;

		while (lua_rawgeti(L, 4, i) == LUA_TSTRING) {
			const char * const flag = luaL_checkstring(L, -1);
			const struct mount_flags *current = flags;
	
			while (current != flagsend && strcmp(current->name, flag) != 0) {
				current++;
			}
	
			if (current != flagsend) {
				mountflags |= current->mask;
			}
	
			lua_pop(L, 1);
			i++;
		}
	}

	return mountflags;
}

static int
lua_fs_mount(lua_State *L) {
	const char * const source = luaL_checkstring(L, 1), * const target = luaL_checkstring(L, 2), * const filesystemtype  = luaL_checkstring(L, 3);
	const unsigned long mountflags = fs_mount_flags(L);

#ifdef __GLIBC__
	const char * const opts = lua_tostring(L, 5);
	if (mount(source, target, filesystemtype, mountflags, (void *)opts) != 0) {
		return luaL_error(L, "fs.mount: mount %s %s %s: %s", source, target, filesystemtype, strerror(errno));
	}
#elif defined(__APPLE__)
	if (mount(filesystemtype, target, mountflags, (void *)source) != 0) {
		return luaL_error(L, "fs.mount: mount %s %s %s: %s", source, target, filesystemtype, strerror(errno));
	}
#else
#error "Unsupported platform's mount"
#endif

	return 0;
}

static int
lua_fs_umount(lua_State *L) {
	const char * const target = luaL_checkstring(L, 1);

#ifdef __GLIBC__
	if (umount(target) != 0) {
		return luaL_error(L, "fs.umount: umount %s: %s", target, strerror(errno));
	}
#elif defined(__APPLE__)
	const unsigned long mountflags = fs_mount_flags(L);

	if (unmount(target, mountflags) != 0) {
		return luaL_error(L, "fs.mount: unmount %s %X: %s", target, mountflags, strerror(errno));
	}
#else
#error "Unsupported platform's umount"
#endif

	return 0;
}

static int
lua_fs_pwd(lua_State *L) {
	bool toosmall = true;
	size_t size = 512;

	while (toosmall) {
		char buffer[size];
		const char *path = (errno = 0, getcwd(buffer, sizeof (buffer)));

		if (path == NULL) {
			if (errno != ERANGE) {
				return luaL_error(L, "fs.pwd: getcwd (%luB): %s", size, strerror(errno));
			}
			size *= 2;
		} else {
			lua_pushstring(L, path);
			toosmall = false;
		}
	}

	return 1;
}

static int
lua_fs_path(lua_State *L) {
	const int top = lua_gettop(L);
	luaL_Buffer b;

	luaL_buffinit(L, &b);

	for (int i = 1; i <= top; i++) {
		size_t length;
		const char *string = luaL_checklstring(L, i, &length);

		/* Remove trailing slashes, makes good-looking paths */
		while (string[length - 1] == '/') {
			length--;
		}

		if (i > 1) {
			while (*string == '/') {
				length--;
				string++;
			}

			luaL_addchar(&b, '/');
		}

		luaL_addlstring(&b, string, length);
	}

	luaL_pushresult(&b);

	return 1;
}

static int
lua_fs_chdir(lua_State *L) {
	const char * const path = luaL_checkstring(L, 1);

	if (chdir(path) != 0) {
		return luaL_error(L, "fs.chdir: chdir %s: %s", path, strerror(errno));
	}

	return 0;
}

static int
lua_fs_chroot(lua_State *L) {
	const char * const path = luaL_checkstring(L, 1);

	if (chroot(path) != 0) {
		return luaL_error(L, "fs.chroot: chroot %s: %s", path, strerror(errno));
	}

	return 0;
}

static int
lua_fs_dirname(lua_State *L) {
	size_t length;
	const char * const path = luaL_checklstring(L, 1, &length);
	char buffer[length + 1];

	strncpy(buffer, path, sizeof (buffer));
	lua_pushstring(L, dirname(buffer));

	return 1;
}

static int
lua_fs_basename(lua_State *L) {
	size_t length;
	const char * const path = luaL_checklstring(L, 1, &length);
	char buffer[length + 1];

	strncpy(buffer, path, sizeof (buffer));
	lua_pushstring(L, basename(buffer));

	return 1;
}

static const luaL_Reg fs_funcs[] = {
	{ "isreg",    lua_fs_isreg },
	{ "isdir",    lua_fs_isdir },
	{ "isexe",    lua_fs_isexe },
	{ "create",   lua_fs_create },
	{ "read",     lua_fs_read },
	{ "unlink",   lua_fs_unlink },
	{ "mkdir",    lua_fs_mkdir },
	{ "mkdirs",   lua_fs_mkdirs },
	{ "readdir",  lua_fs_readdir },
	{ "rmdir",    lua_fs_rmdir },
	{ "mount",    lua_fs_mount },
	{ "umount",   lua_fs_umount },
	{ "pwd",      lua_fs_pwd },
	{ "path",     lua_fs_path },
	{ "chdir",    lua_fs_chdir },
	{ "chroot",   lua_fs_chroot },
	{ "dirname",  lua_fs_dirname },
	{ "basename", lua_fs_basename },
	{ NULL, NULL }
};

int
luaopen_fs(lua_State *L) {

	luaL_newlib(L, fs_funcs);

	return 1;
}
