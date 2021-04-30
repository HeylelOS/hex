# hex 1 2021-04-24 HeylelOS

## NAME
hex - Hex meta build system Lua interpreter.

## SYNOPSIS
- **hex** [-h] [-C \<dir\>] rituals...

## DESCRIPTION
Lua interpreter for the Hex meta build system framework.
It exports the five hex libraries: **fs**, **env**, **log**, **hash** and **hex** and the **hex** extended lua runtime.
However, it doesn't export the whole Lua standard libraries.
This was chosen to restrict scripts to the Hex framework paradigm (explicitly avoiding bad programming practices like, for example, using coroutines when they aren't needed).
The following components from the Lua standard libraries still are available:
- \_G: The base components, which regroups every global functions (like __print__).
- table: Table related functions.
- string: String related functions.
- utf8: UTF8 related functions

## OPTIONS
- -h : Prints usage and exits.
- -C \<dir\> : Current working directory, changed before doing anything else.

## AUTHOR
Valentin Debon (valentin.debon@heylelos.org)

## SEE ALSO
lua(1).

