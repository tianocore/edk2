@REM 
@REM  Currently, Build system does not provide post build mechanism for module 
@REM  and platform building, so just use a bat file to do post build commands.
@REM  Originally, following post building command is for EfiLoader module.
@REM 

@set BUILD_DIR=%WORKSPACE%\Build\DuetPkg\DEBUG_MYTOOLS
@set BASETOOLS_DIR=%WORKSPACE_TOOLS_PATH%\Bin\Win32
@set PROCESSOR=""

@if "%1"=="" goto NoArch
@if "%1"=="IA32" set PROCESSOR=IA32
@if "%1"=="X64" set PROCESSOR=X64
@if %PROCESSOR%=="" goto WrongArch

@set OUTPUT_DIR=%BUILD_DIR%\%PROCESSOR%\DuetPkg\BootSector\BootSector\OUTPUT

@echo Compressing DUETEFIMainFv.FV ...
@%BASETOOLS_DIR%\TianoCompress -e -o %BUILD_DIR%\FV\DUETEFIMAINFV.z %BUILD_DIR%\FV\DUETEFIMAINFV.Fv

@echo Compressing DxeMain.efi ...
@%BASETOOLS_DIR%\TianoCompress -e -o %BUILD_DIR%\FV\DxeMain.z %BUILD_DIR%\%PROCESSOR%\DxeCore.efi

@echo Compressing DxeIpl.efi ...
@%BASETOOLS_DIR%\TianoCompress -e -o %BUILD_DIR%\FV\DxeIpl.z %BUILD_DIR%\%PROCESSOR%\DxeIpl.efi

@echo Generate Loader Image ...
@if "%PROCESSOR%"=="IA32" goto GENERATE_IMAGE_IA32
@if "%PROCESSOR%"=="X64" goto GENERATE_IMAGE_X64

:GENERATE_IMAGE_IA32
@%BASETOOLS_DIR%\EfiLdrImage.exe -o %BUILD_DIR%\FV\Efildr32 %BUILD_DIR%\%PROCESSOR%\EfiLoader.efi %BUILD_DIR%\FV\DxeIpl.z %BUILD_DIR%\FV\DxeMain.z %BUILD_DIR%\FV\DUETEFIMAINFV.z
@copy /b %OUTPUT_DIR%\Start.com+%OUTPUT_DIR%\Efi32.com2+%BUILD_DIR%\FV\Efildr32 %BUILD_DIR%\FV\Efildr
@copy /b %OUTPUT_DIR%\Start16.com+%OUTPUT_DIR%\Efi32.com2+%BUILD_DIR%\FV\Efildr32 %BUILD_DIR%\FV\Efildr16
@copy /b %OUTPUT_DIR%\Start32.com+%OUTPUT_DIR%\Efi32.com2+%BUILD_DIR%\FV\Efildr32 %BUILD_DIR%\FV\Efildr20
@goto end

:GENERATE_IMAGE_X64
@%BASETOOLS_DIR%\EfiLdrImage.exe -o %BUILD_DIR%\FV\Efildr64 %BUILD_DIR%\%PROCESSOR%\EfiLoader.efi %BUILD_DIR%\FV\DxeIpl.z %BUILD_DIR%\FV\DxeMain.z %BUILD_DIR%\FV\DUETEFIMAINFV.z
@copy /b %OUTPUT_DIR%\Start64.com+%OUTPUT_DIR%\Efi64.com2+%BUILD_DIR%\FV\Efildr64 %BUILD_DIR%\FV\EfildrPure
@%BASETOOLS_DIR%\GenPage.exe %BUILD_DIR%\FV\EfildrPure -o %BUILD_DIR%\FV\Efildr
@copy /b %OUTPUT_DIR%\St16_64.com+%OUTPUT_DIR%\Efi64.com2+%BUILD_DIR%\FV\Efildr64 %BUILD_DIR%\FV\Efildr16Pure
@%BASETOOLS_DIR%\GenPage.exe %BUILD_DIR%\FV\Efildr16Pure -o %BUILD_DIR%\FV\Efildr16
@copy /b %OUTPUT_DIR%\St32_64.com+%OUTPUT_DIR%\Efi64.com2+%BUILD_DIR%\FV\Efildr64 %BUILD_DIR%\FV\Efildr20Pure
@%BASETOOLS_DIR%\GenPage.exe %BUILD_DIR%\FV\Efildr20Pure -o %BUILD_DIR%\FV\Efildr20
@goto end


:NoArch
@echo Error! Please specific the architecture.
@goto Help

:WrongArch
@echo Error! Wrong architecture.
@goto Help

:Help
@echo Usage: "PostBuild [IA32|X64]"
:end