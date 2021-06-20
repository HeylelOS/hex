#include "hex/lua.h"

#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fts.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <libgen.h>
#include <alloca.h>
#include <fcntl.h>
#include <errno.h>

#ifdef __APPLE__
#include <sys/clonefile.h>
#endif

#ifdef __linux__
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif

struct fs_copy {
	struct stat *srcst;
	const char *src, *dest;
	size_t srclen, destlen;
};

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
fs_copy_synopsis(lua_State *L, struct fs_copy *root) {
	struct stat destst;

	if (stat(root->src, root->srcst) != 0) {
		return luaL_error(L, "fs.copy: stat %s: %s", root->src, strerror(errno));
	}

	if (stat(root->dest, &destst) != 0) {
		if (errno != ENOENT) {
			return luaL_error(L, "fs.copy: stat %s: %s", root->dest, strerror(errno));
		}
	} else {
		if ((root->srcst->st_mode & S_IFMT) != (destst.st_mode & S_IFMT)) {
			return luaL_error(L, "fs.copy: File type for %s and %s differ", root->src, root->dest);
		}
	}

	return 0;
}

static int
fs_copy_file(lua_State *L, const struct fs_copy *copy) {
#ifdef __APPLE__
	if (clonefile(copy->src, copy->dest, 0) == 0) {
		return 0;
	}
#endif
	int srcfd = open(copy->src, O_RDONLY);

	if (srcfd < 0) {
		return luaL_error(L, "fs.copy: open %s: %s", copy->src, strerror(errno));
	}

	int destfd = open(copy->dest, O_WRONLY | O_CREAT | O_TRUNC, copy->srcst->st_mode & 0777);

	if (destfd < 0) {
		close(srcfd);
		return luaL_error(L, "fs.copy: open %s: %s", copy->dest, strerror(errno));
	}

#ifdef __linux__
	/* Copy on write for supported filesystems on linux */
	if (ioctl(destfd, FICLONE, srcfd) == 0) {
		close(srcfd);
		close(destfd);
		return 0;
	}
#endif

	char block[copy->srcst->st_blksize];
	ssize_t readval;

	while (readval = read(srcfd, block, sizeof (block)), readval > 0) {
		const char *current = block;
		size_t left = readval;

		while (left != 0) {
			const ssize_t writeval = write(destfd, current, left);

			if (writeval < 0) {
				return luaL_error(L, "fs.copy: write %s: %s", copy->dest, strerror(errno));
			}

			current += writeval;
			left -= writeval;
		}
	}

	if (readval != 0) {
		return luaL_error(L, "fs.copy: read %s: %s", copy->src, strerror(errno));
	}

	close(srcfd);
	close(destfd);

	return 0;
}

static int
fs_copy_symlink(lua_State *L, const struct fs_copy *copy) {
#ifdef __APPLE__
	if (clonefile(copy->src, copy->dest, CLONE_NOFOLLOW) == 0) {
		return 0;
	}
#endif
	char target[copy->srcst->st_size + 1];
	ssize_t linklen = readlink(copy->src, target, sizeof (target));

	/* Taking stat's size might not work for every filesystem, better safe than segfault */
	if (linklen != sizeof (target) - 1) {
		return luaL_error(L, "fs.copy: readlink %s: %s", copy->src, strerror(errno));
	}
	target[linklen] = '\0';

	if (unlink(copy->dest) != 0 && errno != ENOENT) {
		return luaL_error(L, "fs.copy: unlink %s: %s", copy->dest, strerror(errno));
	}

	if (symlink(target, copy->dest) != 0) {
		return luaL_error(L, "fs.copy: symlink %s %s: %s", target, copy->dest, strerror(errno));
	}

	return 0;
}

static int
fs_copy_directory(lua_State *L, const struct fs_copy *copy) {

	if (mkdir(copy->dest, copy->srcst->st_mode & 0777) != 0 && errno != EEXIST) {
		return luaL_error(L, "fs.copy: mkdir %s: %s", copy->dest, strerror(errno));
	}

	return 0;
}

static inline const char *
fs_copy_destcpy(char *buffer, size_t buffersize, const char *relpath, const char *dest, size_t destlen) {
	return strncpy(stpncpy(buffer, dest, destlen), relpath, buffersize - destlen) - destlen;
}

static int
fs_copy_tree(lua_State *L, const struct fs_copy *root) {
	char * const paths[] = { strncpy(alloca(root->srclen + 1), root->src, root->srclen + 1), NULL };
	FTS *ftsp = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
	FTSENT *entry = fts_read(ftsp);

	/* Skipped first entry, src pre-order */
	if (entry == NULL) {
		return luaL_error(L, "fs.copy: fts_read %s: %s", root->src, strerror(errno));
	}

	/* Create destination if not previously existing */
	if (mkdir(root->dest, root->srcst->st_mode & 0777) != 0 && errno != EEXIST) {
		return luaL_error(L, "fs.copy: mkdir %s: %s", root->dest, strerror(errno));
	}

	while (entry = fts_read(ftsp), entry != NULL) {
		char dest[root->destlen + entry->fts_pathlen - root->srclen + 2];
		const struct fs_copy copy = {
			.srcst = entry->fts_statp,
			.src = entry->fts_path,
			.dest = fs_copy_destcpy(dest, sizeof (dest),
				entry->fts_path + root->srclen, root->dest, root->destlen),
			.srclen = entry->fts_pathlen,
			.destlen = sizeof (dest) - 1,
		};
		switch (entry->fts_info) {
		case FTS_D:
			fs_copy_directory(L, &copy);
		case FTS_DP:
			break;
		case FTS_F:
			fs_copy_file(L, &copy);
			break;
		case FTS_SL:
		case FTS_SLNONE:
			fs_copy_symlink(L, &copy);
			break;
		case FTS_DNR:
		case FTS_ERR:
		case FTS_NS:
			return luaL_error(L, "fs.copy: fts_read %s: %s", entry->fts_path, strerror(errno));
		default:
			return luaL_error(L, "fs.copy: fts_read %s: Unsupported file type", entry->fts_path);
		}
	}

	/* Cleanup */
	if (errno != 0) {
		return luaL_error(L, "fs.copy: %s to %s: %s", root->src, root->dest, strerror(errno));
	}

	fts_close(ftsp);

	return 0;
}

static int
lua_fs_copy(lua_State *L) {
	struct stat st;
	struct fs_copy root;

	root.srcst = &st;
	root.src = luaL_checklstring(L, 1, &root.srclen),
	root.dest = luaL_checklstring(L, 2, &root.destlen),

	fs_copy_synopsis(L, &root);

	switch (st.st_mode & S_IFMT) {
	case S_IFREG:
		return fs_copy_file(L, &root);
	case S_IFLNK:
		return fs_copy_symlink(L, &root);
	case S_IFDIR:
		return fs_copy_tree(L, &root);
	default:
		return luaL_error(L, "fs.copy: Unsupported copy for file %s to %s", root.src, root.dest);
	}
}

static int
lua_fs_remove(lua_State *L) {
	size_t length;
	const char *path = luaL_checklstring(L, 1, &length);
	char * const paths[] = { strncpy(alloca(length + 1), path, length + 1), NULL };
	FTS *ftsp = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
	FTSENT *entry;

	while (entry = fts_read(ftsp), entry != NULL) {
		switch (entry->fts_info) {
		case FTS_DP:
			if (rmdir(entry->fts_path) != 0) {
				return luaL_error(L, "fs.remove: rmdir %s: %s", entry->fts_path, strerror(errno));
			}
		case FTS_D:
			break;
		case FTS_F:
		case FTS_SL:
		case FTS_SLNONE:
			if (unlink(entry->fts_path) != 0) {
				return luaL_error(L, "fs.remove: unlink %s: %s", entry->fts_path, strerror(errno));
			}
			break;
		case FTS_DNR:
		case FTS_ERR:
		case FTS_NS:
			if (errno == ENOENT) {
				break;
			}
			return luaL_error(L, "fs.remove: fts_read %s: %s", entry->fts_path, strerror(errno));
		default:
			return luaL_error(L, "fs.remove: fts_read %s: Unsupported file type", entry->fts_path);
		}
	}

	if (errno != 0) {
		return luaL_error(L, "fs.remove: fts_read %s: %s", *paths, strerror(errno));
	}

	fts_close(ftsp);

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
	{ "copy",     lua_fs_copy },
	{ "remove",   lua_fs_remove },
	{ "mkdirs",   lua_fs_mkdirs },
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
