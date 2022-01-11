# hex

Hex meta build system framework core.

### hex.silent

Used to determine if `hex.cast` and `hex.charm` print executed commands.

### hex.cast (program[, arguments...])

Executes **program** with the following **arguments**.
If `hex.silent` is `true`, does not print command on standard output.
Waits the process for termination, raises an error if it failed
and returns nothing if it succeeded.

### hex.charm (program[, arguments...])

Executes **program** with the following **arguments**.
If `hex.silent` is `true`, does not print command on standard output.
Waits the process for termination, raises an error if it failed.
Returns its _standard output_, with the last line delimiter removed, if it succeeded.

### hex.crucible (molten)

Creates the crucible `molten` directory if it didn't already exist (cf. `fs.mkdirs`).
Returns a new crucible, with its `molten` attribute set to **molten**.
Its `schackle`, `melted` and `env` all initialized as empty tables.

### hex.dofile (filename[, arguments...])

Loads an runs the given file, forwarding the given arguments.
Raises an error if unable to load the chunk, or if the chunk raises any.
Returns the file's code chunk return values if any.

### hex.exit ([status])

Exits the script with the given **status**.
**Status** can be one of the following, translated accordingly:
- Not given, or nil: Success.
- Number: The given number, as an integer.
- Boolean: Success if `true`, Failure if `false`.
- String: Success if `success`, Failure if `failure`.
The function raises an error if status is not of the previously defined types/values.

### hex.hinder (shackle)

Executes `hex.hinderuser` and `hex.hinderfilesystem` for the calling process,
according to the presence of the respective `user` and `filesystem` shackle attributes.

### hex.hinderfilesystem (filesystem)

Mounts all `filesystem`'s `mountpoints` elements before
entering a the new root specified by the `root` attribute.

### hex.hinderuser (user)

Changes _user namespace_ and makes the calling process believe it has
the given `uid` and `gid` attributes as respective _real user id_ and _real group id_.
Implementation supported for Linux, emits a warning for any other non-Linux platforms.

### hex.incantation ([rituals...])

For each ritual in **rituals**, if ritual is a string, resolves it through `hex.rituals`,
else if ritual is a function, forward the function. Returns an array of all resolved **rituals**.
Raises an error if any of the ritual is not a function.

### hex.invoke ([functions...][, filename])

Creates a new process and runs every **functions**. Waits for process termination.
If **filename** is specified, the process's standard output is redirected into it, creating it and truncating it if required.
Returns if successful, raises an error if the process didn't return successfully.

### hex.melt (crucible, source)

Adds the specified **source** directory as a material to the **crucible**'s `melted`.
The source's name will be its _basename_ (cf. `fs.basename`), invalid if it is a hidden file or if it expands to `/`.
Creates a new build directory entry with the previously defined name in the crucible `molten`'s `artifacts` directory.
The build directory is used for artifacts if the material default rituals support out-of-tree builds.
The created material is created with the following attributes:
- `build`: Build directory in the crucible `molten`'s `artifacts` directory.
- `source`: The given source directory.
- `dependencies`: Empty array of other materials names this material depends upon.
- `override`: Empty table of rituals to override.
- `setup`: Empty table of miscellaneous setup informations to forward to rituals.
- `env`: Empty table of environment variables to add to the rituals processes.

### hex.preprocess (source, destination, variables)

Preprocesses the `source` file into the `destination` file, creating or truncating the latest accordingly.
Replaces every occurence of any `@<variable>@` in `source` with the content of the `<variable>` key in table `variables`.
If the value associated with the `<variable>` key cannot be coerced into a string, it is replaced by an empty string.
If the file is terminated before the closing of any @ pattern, it should be considered an undefined behavior.
No truncation is performed for the content enclosed between @s.
- `source`: The pattern file.
- `destination`: The output file where variables are to be expanded.
- `variables`: A table, indexing keys with their associated replacements. No metamethod is to be called on table, replacement is done by raw access.

### hex.perform (crucible[, rituals...])

Invoke every ritual in **rituals** for each `melted` material according to an order
satisfying their dependencies. **rituals** is resolved as in `hex.incantation`.
Before any ritual is started for a material, a log of level `notice` is emitted for itself.
And before a ritual is started for a material, a log of level `info` is emitted for the said material/ritual.
For every material, every ritual is invoked in order, hindered by the **crucible**'s `shackle`.
The environment is overriden according to the **crucible**'s and material's `env` attributes.
The incantation is finally executed with the appropriate name and material.
