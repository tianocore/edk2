============================================================================
                                 OVERVIEW
============================================================================
The binaries of ShellBinPkg are generated with ShellPkg project and built with
BaseTools Package (git version: 065a7d406cf8ebc71edb2afc66a70f11d9e83a58). The
binaries are built with no debug information by building with "RELEASE" target.

To generate Full Shell, execute:
  "build -a IA32 -a X64 -p ShellPkg\ShellPkg.dsc -b RELEASE"
To generate Minimal Shell, execute:
  "build -a IA32 -a X64 -p ShellPkg\ShellPkg.dsc -b RELEASE -D NO_SHELL_PROFILES"

============================================================================
                          KNOWN LIMITATIONS
============================================================================
1. RM can delete current working directory via other map name.
2. DrvCfg does not overlap boot manager functionality.

============================================================================
