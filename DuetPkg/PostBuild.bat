#
# Currently, Build system does not provide post build mechanism for module 
# and platform building, so just use a bat file to do post build commands.
# Originally, following post building command is for EfiLoader module.
#

set BUILD_DIR=%WORKSPACE%\Build\DuetPkg\DEBUG_MYTOOLS
set BASETOOLS_DIR=%WORKSPACE_TOOLS_PATH%\Bin\Win32
set OUTPUT_DIR=%BUILD_DIR%\IA32\DuetPkg\BootSector\BootSector\OUTPUT

%BASETOOLS_DIR%\TianoCompress -e -o %BUILD_DIR%\FV\DUETEFIMAINFV.z %BUILD_DIR%\FV\DUETEFIMAINFV.Fv
%BASETOOLS_DIR%\TianoCompress -e -o %BUILD_DIR%\FV\DxeMain.z %BUILD_DIR%\IA32\DxeMain.efi
%BASETOOLS_DIR%\TianoCompress -e -o %BUILD_DIR%\FV\DxeIpl.z %BUILD_DIR%\IA32\DxeIpl.efi
%BASETOOLS_DIR%\EfiLdrImage.exe -o %BUILD_DIR%\FV\Efildr32 %BUILD_DIR%\IA32\EfiLoader.efi %BUILD_DIR%\FV\DxeIpl.z %BUILD_DIR%\FV\DUETEFIMAINFV.z
copy /b %OUTPUT_DIR%\Start.com+%OUTPUT_DIR%\Efi32.com2+%BUILD_DIR%\FV\Efildr32 %BUILD_DIR%\FV\Efildr
