This is EDK style package and the following steps can help to build the package:
1. Download the newly EDK Shell project release EdkShell1.05.zip from the following URL and unzip it to
   a local directory in workspace, for example, $(WORKSPACE)\Shell: 
   https://efi-shell.tianocore.org/servlets/ProjectDocumentList?folderID=52&expandFolder=52&folderID=0
   
   Alternatively, the EDK Shell source directory can be retrieved from the following SVN address:
   https://efi-shell.tianocore.org/svn/efi-shell/trunk/Shell 
   SVN Revision r30 corresponds to EDK Shell 1.05 official release.
   
2. Update EDK_SHELL_DIR macro in the [Defines] section in EdkShellPkg.dsc to point to
   the directory containing the EDK Shell source directory, e.g. Shell.

3. The ShellHotFix.patch file solves the follow two issues in EDK Shell 1.05 release:
   a. RFC3066 language compatibility issue in drivers command.
   b. "CHAR8" compatibility issue in DmpStore, Edit command and shell script execution. 
   This patch will be integrated into the later official release.

4. The EDK II style DSC file is used to validate build EDK Shell source & EDK compatibility package and can generate the binaries in EdkShellBinPkg.
   To use this file, execute the following command under workspace to build EDK Shell source:
   build -a IA32 -a X64 -a IPF -p EdkShellPkg\EdkShellPkg.dsc