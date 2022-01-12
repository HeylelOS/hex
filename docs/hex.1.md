# NAME
hex - Hex meta build system Lua interpreter.

# SYNOPSIS
- **hex** [-hs] [-L \<loglevel\>] [-H \<report\>] [-C \<dir\>] rituals...

# DESCRIPTION
Lua interpreter for the Hex meta build system framework.
It exports the four hex libraries: **fs**, **env**, **log** and **hex** and the **hex** extended lua runtime.
And also exports the specified reporting library depending on the given report argument, available as **report**.
However, it doesn't export the whole Lua standard libraries.
This was chosen to restrict scripts to the Hex framework paradigm (explicitly avoiding bad programming practices like, for example, using coroutines when they aren't needed).
The following components from the Lua standard libraries still are available:
- \_G: The base components, which regroups every global functions (like __print__).
- table: Table related functions.
- string: String related functions.
- utf8: UTF8 related functions

# OPTIONS
- -h : Prints usage and exits.
- -s : Silence hex, executed commands through casts and charms won't be printed on standard output.
- -L \<loglevel\> : Shortcut to set **log.level**, if none is specified, nothing will be set.
- -H \<report\> : Report type to export, valid types are **log** and **none**. Default is **log**.
- -C \<dir\> : Current working directory, changed before doing anything else.

# AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

# SEE ALSO
lua(1).

