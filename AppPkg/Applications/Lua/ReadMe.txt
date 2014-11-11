This is Lua 5.2.3, released on 11 Nov 2013.

For installation instructions, license details, and
further information about Lua, see doc/readme.html.
=================================================

Embedding Lua
-------------
The Lua library instance, LuaLib, is defined by StdLib.inc.  Since, currently, all applications which
embed Lua are also StdLib applications, StdLib.inc will be included by your package's .DSC file.

The header files required to use LuaLib are in the standard include path at StdLib\Include\Lua.
They may be referenced as:
  #include  <Lua/lua.h>
  #include  <Lua/lualib.h>
  #include  <Lua/lauxlib>
  #include  <Lua/luaconf.h>

Lua/luaconf.h is the Lua configuration file.  If you wish to build Lua with custom characteristics,
this is the file to modify.  Modify the file in StdLib\Include\Lua since the file in the Lua
source tree is just a stub which references the file in StdLib.


Installation on UEFI
--------------------
Install the Lua.efi file into \Efi\Tools.   This is the standalone Lua interpreter.
Create a directory, \Efi\StdLib\lib\Lua.    This is the default location for Lua scripts.

If desired, copy the files from AppPkg\Applications\Lua\scripts, in the source tree, into
\Efi\StdLib\lib\Lua.

Bugs and Other Issues
---------------------
EOF characters, ^D or ^Z, are not properly recognized by the console and can't be used to
terminate an application.  Use os.exit() to exit Lua.
