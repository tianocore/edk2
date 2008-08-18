This is EDK style package:
1. It contians an SVN extern Shell directory for the SVN address:
   https://efi-shell.tianocore.org/svn/efi-shell/trunk/Shell
2. This SVN extern directory is from EDK Shell Source project and is read only.
   Currently version (r30) corresponds to EDK Shell 1.05 official release.
3. The ShellHotFix.patch file solves the follow two issues in EDK Shell 1.05 release:
   a. RFC3066 language compatibility issue in drivers command
   b. "CHAR8" compatibility issue in DmpStore command. 
   This patch will be integrated into the later official release.
4. The EDK II style DSC file is used to validate build EDK Shell source & EDK compatibility package and can generate the binaries in EdkShellBinPkg.
   To use this file, execute the following command under workspace to build EDK Shell source:
   build -a IA32 -a X64 -a IPF EdkShellPkg\EdkShellPkg.dsc