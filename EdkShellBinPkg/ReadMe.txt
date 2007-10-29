The binaries of EdkShellBinPkg are generated with EDK-Shell 1.03 release and build with Edk Compatibility Package
(r4226)


The following steps are can help to re-generate these binaries for customization:
1. Check out EdkCompatibilityPkg (r4226) to a directory EdkCompatibilityPkg in workspace (svn https://edk2.tianocore.org/svn/edk2/trunk/edk2/EdkCompatibilityPkg). 
2. Download EfiShell 1.03.zip from EDK Shell official release https://efi-shell.tianocore.org/servlets/ProjectDocumentList?folderID=52&expandFolder=52&folderID=45
3. Unzip it to a local folder in EdkCompatibilityPkg , e.g. c:\EdkII\EdkCompatibilityPkg\Other\Maintained\Application\Shell
4. Add the INF file under [components] section of platform DSC files:
  EdkCompatibilityPkg\Sample\Platform\Ia32\Build\Ia32.dsc
  EdkCompatibilityPkg\Sample\Platform\X64\Build\X64.dsc
  EdkCompatibilityPkg\Sample\Platform\Ipf\Build\Ipf.dsc
4.Set environment variable of EDK_SOURCE, e.g. EDK_SOURCE=c:\EdkII\EdkCompatibilityPkg
5.Go to build directory of different architecture and enter "build", e.g. EdkCompatibilityPkg\Sample\Platform\Ia32\Build 

Note: Other\Maintained\Application\Shell\Shell.inf corresponds to Minimum shell binaries.
      Other\Maintained\Application\Shell\ShellFull.inf corresponds to Full Shell binaries.