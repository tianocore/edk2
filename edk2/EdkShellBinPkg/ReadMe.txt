The binaries of EdkShellBinPkg are generated with Efi-Shell-Dev-Snapshot-20090527.zip and build with Edk Compatibility & BaseTools Package
(r8419)

The following steps can help to re-generate these binaries for customization:
1. Check out EdkCompatibilityPkg (r8419) to $(WORKSPACE)\EdkCompatibilityPkg (svn https://edk2.tianocore.org/svn/edk2/trunk/edk2/EdkCompatibilityPkg).
2. Check out EdkShellPkg(r8419) to $(WORKSPACE)\EdkShellPkg (svn https://edk2.tianocore.org/svn/edk2/trunk/edk2/EdkShellPkg).
3  Check out Edk Shell project source (r33) to $(WORKSPACE) (svn https://efi-shell.tianocore.org/svn/efi-shell/trunk/Shell).  It is read-only and current revison (r33) is identical to Efi-Shell-Dev-Snapshot-20090527.zip. 
4. Update to the newest BaseTools package. (r8419 or later)
5. Under workspace directory (i.e. c:\EdkII), 
   To generate Minimum Shell, execute: "build -a IA32 -a X64 -a IPF -p EdkShellPkg\EdkShellPkg.dsc -m EdkShellPkg\Shell\Shell.inf" 
   To generate Full Shell, execute: "build -a IA32 -a X64 -a IPF -p EdkShellPkg\EdkShellPkg.dsc -m EdkShellPkg\Shell\ShellFull.inf" 
6. In EdkShellBinPkg\Bin, we only provides the binary files of those applications which haven't been contained in Minmum Shell but in Full Shell.
   To generate them, execute: "build -a IA32 -a X64 -a IPF -p EdkShellPkg\EdkShellPkg.dsc -m EdkShellPkg\Xxx\Xxx.inf". Xxx means the corresponding module name.