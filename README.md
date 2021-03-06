# Hex

Hex is a Lua-based meta-build configuration tool and framework.

## What?

When you're building a project using several subprojects, themselves having
several kinds of build systems you begin to explore depths of hideous scripts
(usually shell ones) which are supposed to do many things like
configuring, building, testing, packaging, reporting, etc...
These scripts end up extremely complex, undocumented, unmodular and sometimes unstable.
Moreover, when it comes to isolating builds from the default machine configuration,
it becomes very complicated to manage several toolchains and avoid conflicts.

Hex was created to ease these kinds of builds.
It provides a mixed procedural/declarative way to configure (_melt_) your
projects inside a unified build place (called a _crucible_) and handling all
the boilerplate code associated with each build systems inside a set of rules
(called _rituals_).

One of its goal is also to provide the user with a choice for isolation of the host builder.
It is dangerous to release informations about the host machine to the public.
Exposing the username and the paths used to build the toolchains are reproducibility and security concerns,
and companies often end up using overkill solutions like VMs or dockers.

## Why Lua?

Lua is a fast, portable, embeddable scripting language. It is extremely extensible by design.
Using Lua to describe a build sequence is basically using configuration files on steroids.
It is also well documented and its C interface makes it a good choice for cross-language support and system interaction.

## Features

- [x] Targets dependencies
- [x] Environment variable forwarding
- [x] File configuration with @ rules
- [ ] Packaging

Hex supports the following isolation capabilites:
- [x] User/Group isolation (Linux specific implementation)
- [x] Filesystem isolation

Hex intends to support the following systems, for the specified rituals:
- [x] CMake (configure, build, install)
- [x] UNIX configure/make (configure, build, install) (build/installed behaviours mimic'd on GNU ones)
- [x] Kbuild (configure, build, install)
- [x] GNU autotools (configure)
- [ ] QMake (configure)
- [ ] Ninja (build)

If you have suggestions or ideas concerning other build systems, feel free to come forward.

## Configure, build and install

Meson is used to configure, build and install binaries and manpages:

```sh
meson setup build
meson compile -C build
meson install -C build
```

## Documentation

Documentation is built mainly from markdown pages. They're available in the `docs` top level directory.

