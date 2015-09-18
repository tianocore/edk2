============================================================================
                                 OVERVIEW
============================================================================
The binaries of ShellBinPkg are generated with ShellPkg project and built with 
BaseTools Package (r18507). The binaries are built with no debug information 
by building with "RELEASE" target.

The following steps can help to re-generate these binaries for customization:
1. Check out EDK II to $(WORKSPACE) 
(svn https://svn.code.sf.net/p/edk2/code/trunk/edk2).

2. Under $(WORKSPACE) directory (i.e. c:\EdkII),
   To generate Shell, execute:
        "build -a IA32 -a X64 -p ShellPkg\ShellPkg.dsc -b RELEASE"
   To generate Minimal Shell, execute:
        "build -a IA32 -a X64 -p ShellPkg\ShellPkg.dsc -b RELEASE -D NO_SHELL_PROFILES"

============================================================================
                    HOW TO INCORPORATE THIS SHELL INTO NT32
============================================================================
The instructions below are included as a sample and template on how a 
developer may integrate this code into an existing platform:

1. Update system PCDs to support this new module
   Update the PCD as follows using the Shell's PCD:
   gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdShellFile|{ 0x83, 0xA5, 
    0x04, 0x7C, 0x3E, 0x9E, 0x1C, 0x4F, 0xAD, 0x65, 0xE0, 0x52, 0x68, 0xD0, 
    0xB4, 0xD1 }

2. Remove the old shell from the NT32 Firmware list
   Remove the FILE APPLICATION section for the old shell.

3. Add this shell to the NT32 firmware list
   Add the Shell.INF to the end of the list of DXE modules.

4. Build NT32

============================================================================
                          KNOWN LIMITATIONS
============================================================================
1. RM can delete current working directory via other map name.
2. DrvCfg does not overlap boot manager functionality.
3. Shell documentation is in development and forthcoming.

============================================================================
