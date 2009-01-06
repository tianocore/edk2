@REM

@REM Set up environment at fisrt.

@REM set BASETOOLS_DIR=%WORKSPACE_TOOLS_PATH%\Bin\Win32
@set BASETOOLS_DIR=n:\BaseTools\Bin\Win32
@set BUILD_DIR=%WORKSPACE%\Build\DuetPkg\DEBUG_MYTOOLS
@set DISK_LABEL=DUET
@echo on


@if "%1"=="" goto Help
@if "%2"=="" goto Help
@if "%3"=="" goto Help
@set EFI_BOOT_DISK=%2
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
%BASETOOLS_DIR%\Bootsectimage.exe -g FDBs.com %BUILD_DIR%\IA32\DuetPkg\BootSector\BootSector\OUTPUT\Bootsect.com -f
@REM @del FDBS.com
%BASETOOLS_DIR%\Genbootsector.exe -o %EFI_BOOT_DISK% -i %BUILD_DIR%\IA32\DuetPkg\BootSector\BootSector\OUTPUT\Bootsect.com
@echo Done.
copy %BUILD_DIR%\FV\EfiLdr %EFI_BOOT_DISK%
mkdir %EFI_BOOT_DISK%\efi\boot
copy %WORKSPACE%\EdkShellBinPkg\bin\ia32\Shell.efi %EFI_BOOT_DISK%\efi\boot\bootia32.efi /y
@goto end

:CreateFile
@if NOT "%3"=="FAT12" goto WrongFATType
@echo Start to create file boot disk ...
@echo Create boot sector ...
%BASETOOLS_DIR%\Genbootsector.exe -i %EFI_BOOT_DISK% -o FDBs.com
%BASETOOLS_DIR%\Bootsectimage.exe -g FDBs.com %BUILD_DIR%\IA32\DuetPkg\BootSector\BootSector\OUTPUT\Bootsect.com -f
@REM @del FDBS.com
%BASETOOLS_DIR%\Genbootsector.exe -o %EFI_BOOT_DISK% -i %BUILD_DIR%\IA32\DuetPkg\BootSector\BootSector\OUTPUT\Bootsect.com
@echo Done.
@goto end

:CreateUsb
@echo Start to create usb boot disk ...
@if "%3"=="FAT16" goto CreateUsb_FAT16
@if "%3"=="FAT32" goto CreateUsb_FAT32
@if "%3"=="FAT12" goto WrongFATType

:CreateUsb_FAT16
@if "%4"=="step2" goto CreateUsb_FAT16_step2
@echo Format %EFI_BOOT_DISK% ...
@echo.> FormatCommandInput.txt
@format /FS:FAT /v:%DISK_LABEL% /q %EFI_BOOT_DISK% < FormatCommandInput.txt > NUL
@del FormatCommandInput.txt
@echo Create boot sector ...
@%BASETOOLS_DIR%\Genbootsector.exe -i %EFI_BOOT_DISK% -o UsbBs16.com
@%BASETOOLS_DIR%\Bootsectimage.exe -g UsbBs16.com %BUILD_DIR%\IA32\DuetPkg\BootSector\BootSector\OUTPUT\Bs16.com -f
@%BASETOOLS_DIR%\Genbootsector.exe -o %EFI_BOOT_DISK% -i %BUILD_DIR%\IA32\DuetPkg\BootSector\BootSector\OUTPUT\Bs16.com
@%BASETOOLS_DIR%\Genbootsector.exe -m -o %EFI_BOOT_DISK% -i %BUILD_DIR%\IA32\DuetPkg\BootSector\BootSector\OUTPUT\Mbr.com
@echo Done.
@echo PLEASE UNPLUG USB, THEN PLUG IT AGAIN!
@goto end

:CreateUsb_FAT16_step2
@copy %BUILD_DIR%\FV\EfiLdr16 %EFI_BOOT_DISK%
@mkdir %EFI_BOOT_DISK%\efi\boot
@copy %WORKSPACE%\EdkShellBinPkg\bin\ia32\Shell.efi %EFI_BOOT_DISK%\efi\boot\bootia32.efi /y
@goto end

:CreateUsb_FAT32
@echo Format %EFI_BOOT_DISK% ...
@echo.> FormatCommandInput.txt
@format /FS:FAT32 /v:%DISK_LABEL% /q %EFI_BOOT_DISK% < FormatCommandInput.txt > NUL
@del FormatCommandInput.txt
@echo Create boot sector ...
@%BASETOOLS_DIR%\Genbootsector.exe -i %EFI_BOOT_DISK% -o UsbBs32.com
@%BASETOOLS_DIR%\Bootsectimage.exe -g UsbBs32.com %BUILD_DIR%\IA32\DuetPkg\BootSector\BootSector\OUTPUT\Bs32.com -f
@del UsbBs32.com
@%BASETOOLS_DIR%\Genbootsector.exe -o %EFI_BOOT_DISK% -i %BUILD_DIR%\IA32\DuetPkg\BootSector\BootSector\OUTPUT\Bs32.com
@%BASETOOLS_DIR%\Genbootsector.exe -m -o %EFI_BOOT_DISK% -i %BUILD_DIR%\IA32\DuetPkg\BootSector\BootSector\OUTPUT\Mbr.com
@copy %BUILD_DIR%\FV\EfiLdr20 %EFI_BOOT_DISK%
@mkdir %EFI_BOOT_DISK%\efi\boot
@copy %WORKSPACE%\EdkShellBinPkg\bin\ia32\Shell.efi %EFI_BOOT_DISK%\efi\boot\bootia32.efi /y
@echo Done.
@echo PLEASE UNPLUG USB, THEN PLUG IT AGAIN!
@goto end  

:CreateIde
@goto end

:WrongFATType
@echo Wrong FAT type %3 for %1
@goto end

:Help
@echo "Usage: CreateBootDisk [usb|floppy|ide] DiskNumber [FAT12|FAT16|FAT32]"
:end
@echo on