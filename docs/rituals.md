# Rituals

A ritual is a function, taking two arguments:
- name: The name of the material being manipulated.
- material: The material being manipulated.
Rituals are usually stored inside the `hex.rituals` table, indexed by a name
which will be the key for resolution using `hex.incantation`.
A ritual should not be called manually, but should be resolved and be `hex.invoke`d by a call to `hex.perform`.
Its goal is to perform a task inside a build system, it can be the configuration,
the build itself, the installation, or even packaging.

We can separate the predefined ones in two groups: redirecting, and effective.
A redirecting ritual redirects to an effective ritual, and supports override.
An effective ritual actually does something, it uses the `material.setup` table to find its parameters.
They're usually nested in the key corresponding to their associated redirecting ritual, for example,
`cmake-configure` will check its parameters in `material.setup.configure`.

# Overrides

It can happen that a default behaviour for a given source tree is incompatible
or do not satisfy your needs. Luckily, rituals are just functions stored
inside a table, you can manipulate them, change defaults, encapsulate one.
Modifying predefined rituals is not recommended, you should prefer using overrides.

In case a build system is not recognized successfully, or a repository supports
multiple ones, and the default isn't your choice, redirecting rituals support overrides.
The override is a table in the material (`material.override`), it indexes rituals that should
be prioritized, according to the name of the redirecting ritual.

For example, if you want a material to explicitly use `gnu-configure`, you can do the following:
```
material.override.configure = hex.rituals['gnu-configure']
```
Then, when a call to `hex.perform('configure')` is made, the `configure` ritual will call the given override.

This behaviour is not automatic, and each ritual is free to follow it or not.
See their respective documentation for more informations.

# Available rituals

By default, `hex.rituals` is populated with several rituals.
It exposes behaviours and interfaces to configure, build and install based on some build systems.

## build

The `build` ritual redirects to the supported ritual
if the given condition is met, in the given precedence.
It supports override for the key `build`.

- `cmake-build`: If a 'CMakeCache.txt' file is available in the material's build directory.
- `unix-build`: If a 'Makefile' file is available in the material's build directory.
- `kbuild-build`: If a 'Kbuild' file is available in the material's source directory.

## cmake-build

Uses cmake to perform a build from the `material.source` to the `material.build`.
It supports the following parameters, passed along to the `cmake --build` command:
- options
- arguments

## cmake-configure

Uses cmake to configure the `material.source` to be built into `material.build`.
It supports the following parameters, passed along to the `cmake` command:
- variables: Key/Value table of cache variables, translated and appended to the command line options.
- options

## cmake-install

Uses cmake to install what was previously built/configured into `material.build`.
It supports the following parameter, passed along to the `cmake --install` command:
- options

## configure

The `configure` ritual redirects to the supported ritual
if the given condition is met, in the given precedence.
It supports override for the key `configure`.

- `cmake-configure`: If a 'CMakeLists.txt' file is available in the material's source directory.
- `gnu-configure`: If a 'configure.ac' file is available in the material's source directory.
- `unix-configure`: If a 'configure' file is available in the material's source directory.
- `kbuild-configure`: If a 'Kconfig' file is available in the material's source directory.

## gnu-configure

Uses GNU autotools to configure the `material.source` tree, the configure script executed is executed in the `material.build` directory.
It supports the following parameters:
- autoreconf: Options passed to `autoreconf`, when autoreconfing the `material.source`.
- script: Script used to configure the build tree. If not specified, expanded to an absolute path pointing to a `configure` script at the top of the source tree.
- options: Options passed to the configure script.

## install

The `install` ritual redirects to the supported ritual
if the given condition is met, in the given precedence.
It supports override for the key `install`.

- `cmake-install`: If a 'CMakeCache.txt' file is available in the material's build directory.
- `unix-install`: If a 'Makefile' file is available in the material's build directory.
- `kbuild-install`: If a 'Kbuild' file is available in the material's source directory.

## kbuild-build

Executes the `material.source`'s `Makefile` in the `material.source` directory:
- options: Options for the `make` command.
- targets: Targets for the `make` command.

## kbuild-configure

Configures a build for a Kbuild based source tree.
Copies in the working directory, executes the `Makefile` in the `m̀aterial.source` directory, takes two parameters:
- target: Target the `Makefile` will run, `defconfig` if none specified.
- config: A file or directory copied in the `material.source` tree as `.config`.

## kbuild-install

Executes the `material.source`'s `Makefile` install target in the `material.source` directory:
- options: Options for the `make` command.

## unix-build

Executes the `material.build`'s `Makefile` in the `material.build` directory:
- options: Options for the `make` command.
- targets: Targets for the `make` command.

## unix-configure

Executes the `material.source`'s `configure` script in the `material.build` directory.
- script: Script used to configure the build tree. If not specified, expanded to an absolute path pointing to a `configure` script at the top of the source tree.
- options: Options passed to the configure script.

## unix-install

Executes the `material.build`'s `Makefile` install target in the `material.build` directory:
- options: Options for the `make` command.

