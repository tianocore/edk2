The binaries of EdkShellBinPkg are generated with EDK-Shell 1.05 release and build with Edk Compatibility & BaseTools Package
(r5782)

The following steps can help to re-generate these binaries for customization:
1. Check out EdkCompatibilityPkg (r5782) to $(WORKSPACE)\EdkCompatibilityPkg (svn https://edk2.tianocore.org/svn/edk2/trunk/edk2/EdkCompatibilityPkg).
2. Check out EdkShellPkg(r5782) to $(WORKSPACE)\EdkShellPkg (svn https://efi-shell.tianocore.org/servlets/ProjectDocumentList?folderID=52&expandFolder=52&folderID=45).
   This package has an SVN extern directory from EDK Shell project. It is read-only and current revison (r30) is identical to EDK shell 1.05 release. 
3. Update to the newest BaseTools package. (r5782 or later)
4. Apply a hot fix ShellHotFix.patch under EdkShellPkg directory.
   Add a Shell Hot Fix patch to solve RFC3066 language compatibility issue and "CHAR8" compatibility issue in DmpStore, Edit command and shell script execution.
   This patch will be integrated into the later official release.
5. Under workspace directory (i.e. c:\EdkII), execute: build -a IA32 -a X64 -a IPF -p EdkShellPkg\EdkShellPkg.dsc
6. Copy the binaries from Build directory to this package. Typically the EFI binary
   of EdkShellPkg\Shell\$(INF_BASENAME).inf is generated at:
   Build\EdkShellPkg\DEBUG_MYTOOLS\$(ARCH)\EdkShellPkg\Shell\$(INF_BASENAME)\OUTPUT\$(BASENAME).efi
   For example:
   The x64 EFI image of EdkShellPkg\Shell\ver\ver.inf is generated at:
   Build\EdkShellPkg\DEBUG_MYTOOLS\X64\EdkShellPkg\Shell\ver\Ver\OUTPUT\ver.efi

Note: Other\Maintained\Application\Shell\Shell.inf corresponds to Minimum shell binaries.
      Other\Maintained\Application\Shell\ShellFull.inf corresponds to Full Shell binaries.