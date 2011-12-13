============================================================================
                                 OVERVIEW
============================================================================
The UEFI 2.0 shell provides a standard pre-boot command line processor.
It is similar to the EDK EFI Shell or a *nix command line parser.

============================================================================
                    HOW TO INCORPORATE THIS SHELL INTO NT32
============================================================================
The instructions below are included as a sample and template on how a 
developer may integrate this code into an existing platform:

1. Add this shell build to the NT32 build:
   Add the shell.inf to the [components] section as it is in the ShellPkg.dsc.

2. Update system PCDs to support this new module
   Update the PCD as follows using the Shell's PCD:
   gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdShellFile|{ 0x83, 0xA5, 0x04, 0x7C, 0x3E, 0x9E, 0x1C, 0x4F, 0xAD, 0x65, 0xE0, 0x52, 0x68, 0xD0, 0xB4, 0xD1 }

3. Remove the old shell from the NT32 Firmware list
   Remove the FILE APPLICATION section for the old shell.

4. Add this shell to the NT32 firmware list
   Add the Shell.INF to the end of the list of DXE modules.

5. Build NT32


============================================================================
