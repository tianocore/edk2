The binaries of EdkShellBinPkg are generated with Efi-Shell-Dev-Snapshot-20090527.zip and build with Edk Compatibility & BaseTools Package
(r8419)

The following steps can help to re-generate these binaries for customization:
1. Check out EdkCompatibilityPkg (r8419) to $(WORKSPACE)\EdkCompatibilityPkg (svn https://edk2.tianocore.org/svn/edk2/trunk/edk2/EdkCompatibilityPkg).
2. Check out EdkShellPkg(r8419) to $(WORKSPACE)\EdkShellPkg (svn https://edk2.tianocore.org/svn/edk2/trunk/edk2/EdkShellPkg).
3  Check out Edk Shell project source (r33) to $(WORKSPACE) (svn https://efi-shell.tianocore.org/svn/efi-shell/trunk/Shell).  It is read-only and current revison (r33) is identical to Efi-Shell-Dev-Snapshot-20090527.zip. 
4. Update to the newest BaseTools package. (r8419 or later)
5. Under workspace directory (i.e. c:\EdkII), execute: build -a IA32 -a X64 -a IPF -p EdkShellPkg\EdkShellPkg.dsc
6. Copy the binaries from Build directory to this package. Typically the EFI binary of EdkShellPkg\Shell\$(INF_BASENAME).inf is generated at:
   Build\EdkShellPkg\DEBUG_MYTOOLS\$(ARCH)\EdkShellPkg\Shell\$(INF_BASENAME)\OUTPUT\$(BASENAME).efi
   For example:
   The x64 EFI image of EdkShellPkg\Shell\ver\ver.inf is generated at:
   Build\EdkShellPkg\DEBUG_MYTOOLS\X64\EdkShellPkg\Shell\ver\Ver\OUTPUT\ver.efi

Note: Other\Maintained\Application\Shell\Shell.inf corresponds to Minimum shell binaries.
      Other\Maintained\Application\Shell\ShellFull.inf corresponds to Full Shell binaries.