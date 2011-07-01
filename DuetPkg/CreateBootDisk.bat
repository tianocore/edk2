@REM ## @file
@REM #
@REM #  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
@REM #
@REM #  This program and the accompanying materials
@REM #  are licensed and made available under the terms and conditions of the BSD License
@REM #  which accompanies this distribution. The full text of the license may be found at
@REM #  http://opensource.org/licenses/bsd-license.php
@REM #  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM #  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM #
@REM #
@REM ##

@REM Set up environment at first.

@set BASETOOLS_DIR=%WORKSPACE_TOOLS_PATH%\Bin\Win32
@set BOOTSECTOR_BIN_DIR=%WORKSPACE%\DuetPkg\BootSector\bin
@set DISK_LABEL=DUET
@set PROCESSOR=""
@set STEP=1
@call %WORKSPACE%\DuetPkg\GetVariables.bat

@echo on

@if "%1"=="" goto Help
@if "%2"=="" goto Help
@if "%3"=="" goto Help
@if "%4"=="" goto Set_BootDisk
@if "%4"=="step2" (@set STEP=2) else @set TARGET_ARCH=%4
@if "%5"=="step2" @set STEP=2
:Set_BootDisk
@set EFI_BOOT_DISK=%2
@if "%TARGET_ARCH%"=="IA32" set PROCESSOR=IA32
@if "%TARGET_ARCH%"=="X64" set PROCESSOR=X64
@if %PROCESSOR%=="" goto WrongArch
@set BUILD_DIR=%WORKSPACE%\Build\DuetPkg%PROCESSOR%\%TARGET%_%TOOL_CHAIN_TAG%

@if "%1"=="floppy" goto CreateFloppy
@if "%1"=="file" goto CreateFile
@if "%1"=="usb" goto CreateUsb
@if "%1"=="ide" goto CreateIde

goto Help

:CreateFloppy
@if NOT "%3"=="FAT12" goto WrongFATType
@echo Start to create floppy boot disk ...
@echo Format %EFI_BOOT_DISK% ...
@echo.> FormatCommandInput.txt
@echo.n>> FormatCommandInput.txt
@format /v:%DISK_LABEL% /q %EFI_BOOT_DISK% < FormatCommandInput.txt > NUL
@del FormatCommandInput.txt
@echo Create boot sector ...
%BASETOOLS_DIR%\Genbootsector.exe -i %EFI_BOOT_DISK% -o FDBs.com
%BASETOOLS_DIR%\Bootsectimage.exe -g FDBs.com %BOOTSECTOR_BIN_DIR%\Bootsect.com -f
@REM @del FDBS.com
%BASETOOLS_DIR%\Genbootsector.exe -o %EFI_BOOT_DISK% -i %BOOTSECTOR_BIN_DIR%\Bootsect.com
@echo Done.
copy %BUILD_DIR%\FV\EfiLdr %EFI_BOOT_DISK%
mkdir %EFI_BOOT_DISK%\efi\boot
@if "%PROCESSOR%"=="IA32" goto CreateBootFileForIA32
@if "%PROCESSOR%"=="X64" goto CreateBootFileForX64
@goto end

:CreateFile
@if NOT "%3"=="FAT12" goto WrongFATType
@echo Start to create file boot disk ...
@echo Create boot sector ...
%BASETOOLS_DIR%\Genbootsector.exe -i %EFI_BOOT_DISK% -o FDBs.com
%BASETOOLS_DIR%\Bootsectimage.exe -g FDBs.com %BOOTSECTOR_BIN_DIR%\Bootsect.com -f
@REM @del FDBS.com
%BASETOOLS_DIR%\Genbootsector.exe -o %EFI_BOOT_DISK% -i %BOOTSECTOR_BIN_DIR%\Bootsect.com
@echo Done.
@goto end

:CreateUsb
@echo Start to create usb boot disk ...
@if "%3"=="FAT16" goto CreateUsb_FAT16
@if "%3"=="FAT32" goto CreateUsb_FAT32
@if "%3"=="FAT12" goto WrongFATType

:CreateUsb_FAT16
@if "%STEP%"=="2" goto CreateUsb_FAT16_step2
@echo Format %EFI_BOOT_DISK% ...
@echo.> FormatCommandInput.txt
@format /FS:FAT /v:%DISK_LABEL% /q %EFI_BOOT_DISK% < FormatCommandInput.txt > NUL
@del FormatCommandInput.txt
@echo Create boot sector ...
@%BASETOOLS_DIR%\Genbootsector.exe -i %EFI_BOOT_DISK% -o UsbBs16.com
@%BASETOOLS_DIR%\Bootsectimage.exe -g UsbBs16.com %BOOTSECTOR_BIN_DIR%\Bs16.com -f
@%BASETOOLS_DIR%\Genbootsector.exe -o %EFI_BOOT_DISK% -i %BOOTSECTOR_BIN_DIR%\Bs16.com
@%BASETOOLS_DIR%\Genbootsector.exe -m -o %EFI_BOOT_DISK% -i %BOOTSECTOR_BIN_DIR%\Mbr.com
@echo Done.
@echo PLEASE UNPLUG USB, THEN PLUG IT AGAIN!
@goto end

:CreateUsb_FAT16_step2
@copy %BUILD_DIR%\FV\EfiLdr16 %EFI_BOOT_DISK%
@mkdir %EFI_BOOT_DISK%\efi\boot
@if "%PROCESSOR%"=="IA32" goto CreateBootFileForIA32
@if "%PROCESSOR%"=="X64" goto CreateBootFileForX64
@goto end

:CreateUsb_FAT32
@if "%STEP%"=="2" goto CreateUsb_FAT32_step2
@echo Format %EFI_BOOT_DISK% ...
@echo.> FormatCommandInput.txt
@format /FS:FAT32 /v:%DISK_LABEL% /q %EFI_BOOT_DISK% < FormatCommandInput.txt > NUL
@del FormatCommandInput.txt
@echo Create boot sector ...
@%BASETOOLS_DIR%\Genbootsector.exe -i %EFI_BOOT_DISK% -o UsbBs32.com
@%BASETOOLS_DIR%\Bootsectimage.exe -g UsbBs32.com %BOOTSECTOR_BIN_DIR%\Bs32.com -f
@del UsbBs32.com
@%BASETOOLS_DIR%\Genbootsector.exe -o %EFI_BOOT_DISK% -i %BOOTSECTOR_BIN_DIR%\Bs32.com
@%BASETOOLS_DIR%\Genbootsector.exe -m -o %EFI_BOOT_DISK% -i %BOOTSECTOR_BIN_DIR%\Mbr.com
@echo Done.
@echo PLEASE UNPLUG USB, THEN PLUG IT AGAIN!
@goto end  

:CreateUsb_FAT32_step2
@copy %BUILD_DIR%\FV\EfiLdr20 %EFI_BOOT_DISK%
@mkdir %EFI_BOOT_DISK%\efi\boot
@if "%PROCESSOR%"=="IA32" goto CreateBootFileForIA32
@if "%PROCESSOR%"=="X64" goto CreateBootFileForX64
@goto end

:CreateIde
@goto end

:CreateBootFileForIA32
copy %WORKSPACE%\EdkShellBinPkg\MinimumShell\IA32\Shell.efi %EFI_BOOT_DISK%\efi\boot\bootia32.efi /y
@goto end

:CreateBootFileForX64
copy %WORKSPACE%\EdkShellBinPkg\MinimumShell\X64\Shell.efi %EFI_BOOT_DISK%\efi\boot\bootx64.efi /y
@goto end

:WrongFATType
@echo Wrong FAT type %3 for %1
@goto end

:WrongArch
@echo Error! Wrong architecture.
@goto Help

:Help
@echo "Usage: CreateBootDisk [usb|floppy|ide] DiskNumber [FAT12|FAT16|FAT32] [IA32|X64]"
:end
@echo on