This is EDK style package and the following steps can help to build the package:
1. Download the newly EDK Shell project Efi-Shell-Dev-Snapshot-20090527.zip from the following URL and unzip it to
   a local directory in workspace, for example, $(WORKSPACE)\Shell: 
   https://efi-shell.tianocore.org/servlets/ProjectDocumentList?folderID=46&expandFolder=46&folderID=53
   
   Alternatively, the EDK Shell source directory can be retrieved from the following SVN address:
   https://efi-shell.tianocore.org/svn/efi-shell/trunk/Shell 
   SVN Revision r33 corresponds to Efi-Shell-Dev-Snapshot-20090527.zip development snapshot.
   
2. Update EDK_SHELL_DIR macro in the [Defines] section in EdkShellPkg.dsc to point to
   the directory containing the EDK Shell source directory, e.g. $(WORKSPACE)\Shell.

3. The EDK II style DSC file is used to validate build EDK Shell source & EDK compatibility package and can generate     the binaries in EdkShellBinPkg.
   To use this file, execute the following command under workspace to build EDK Shell source:
   build -a IA32 -a X64 -a IPF -p EdkShellPkg\EdkShellPkg.dsc