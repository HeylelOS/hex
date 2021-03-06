# fs

Filesystem-related functions extension.

### fs.isreg (path)

Returns `true` if **path** references a file (see `access(2)`), `false` else.

### fs.isdir (path)

Returns `true` if **path** references a directory (see `stat(2)`), `false` else.

### fs.isexe (path)

Returns `true` if **path** references an executable (see `access(2)`), `false` else.
Note executable can also mean directories, you should also check with `fs.isreg` if you are looking for a script/binary executable.

### fs.copy (source, destination)

Copies content of **source** into **destination**. If **destination** exists, it must be of same type as **source**.
If **source** is a file or a symlink, **destination** is removed and replaced by a copy of **source**.
If **source** is a directory, all its content is recursively copied in **destination**, as if the content was added or overwritten.
On supported systems, files are copied using copy on write if the underlying filesystem supports it.
Returns nothing on success, raises an error on any failure.

### fs.remove ([paths...])

Removes content at **paths**. If one of **paths** is a regular file/symlink, it is unlinked.
If one of **paths** is a directory, all content is recursively removed, and then the entry is removed.
Returns nothing on success, raises an error on any failure.

### fs.mkdirs ([paths...])

Creates every non-existing directory in **paths** as in a `mkdir -p` command.
If a directory already exists it is not considered an error. If it exists and is not a directory, it is considered an error.
Returns nothing on success, raises an error on failure.

### fs.mount (source, target, filesystemtype[, mountflags]\[, opts])

_Mounts_ the device **source** on **target** as a filesystem of type **filesystemtype** and according to the flags in **mountflags** if specified.
On Glibc systems, **opts** is used as the last `mount(2)` argument. It is unused on other platforms.
Semantic, flags and behaviours are highly platform dependent, please refer to `mount(2)` for better informations.
Returns nothing on success, raises an error on failure.

### fs.umount (target[, mountflags...])

_Unmounts_ the filesystem mounted at **target**. **mountflags** are used on supporting platforms. Refer to `umount(2)` or `unmount(2)` depending on your platform.
Returns nothing on success, raises an error on failure.

### fs.pwd ()

Returns the calling process' current working directory on success, raises an error on failure.

### fs.path ([components...])

Concatenates **components** as path components and returns the associated path on success.
Raises an error on failure.

### fs.chdir (path)

Changes the current process working directory to **path** (see `chdir(2)`).
Returns nothing on success, raises an error on failure.

### fs.chroot (path)

Changes the current process filesystem's root directory to **path** (see `chroot(2)`).
Returns nothing on success, raises an error on failure.

### fs.dirname (path)

Returns the `dirname(3)` associated with **path** on success, raises an error on failure.

### fs.basename (path)

Returns the `basename(3)` associated with **path** on success, raises an error on failure.

