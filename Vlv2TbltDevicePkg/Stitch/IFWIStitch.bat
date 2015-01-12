@REM @file
@REM   Windows batch file to build BIOS ROM
@REM
@REM Copyright (c) 2006   - 2014, Intel Corporation. All rights reserved.<BR>
@REM 
@REM   This program and the accompanying materials are licensed and made available under
@REM   the terms and conditions of the BSD License that accompanies this distribution.
@REM   The full text of the license may be found at
@REM   http://opensource.org/licenses/bsd-license.php.
@REM 
@REM   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM

@echo off
SetLocal EnableDelayedExpansion EnableExtensions

:: Set script defaults
set exitCode=0
set BackupRom=1
set UpdateVBios=1
set Stitch_Config=Stitch_Config.txt
copy /y nul Stitching.log >nul

:: Set default Suffix as:  YYYY_MM_DD_HHMM
set hour=%time: =0%
reg copy "HKCU\Control Panel\International" "HKCU\Control Panel\International_Temp" /f >nul
reg add "HKCU\Control Panel\International" /v sShortDate /d "yyyy_MM_dd" /f >nul
for /f "tokens=1" %%i in ("%date%") do set today=%%i
reg copy "HKCU\Control Panel\International_Temp" "HKCU\Control Panel\International" /f >nul
reg delete "HKCU\Control Panel\International_Temp" /f >nul
set IFWI_Suffix=%today%_%hour:~0,2%%time:~3,2%

:: Process input arguments
if "%~1"=="?"       goto Usage
if "%~1"=="/?"      goto Usage
if /i "%~1"=="Help" goto Usage

:OptLoop
if /i "%~1"=="/nV" (
    set UpdateVBios=0
    shift
    goto OptLoop
)
if /i "%~1"=="/nB" (
    set BackupRom=0
    shift
    goto OptLoop
)
if /i "%~1"=="/B" (
    if "%~2"==""  goto Usage
    if not exist %~2 echo BIOS not found. & goto Usage
    set BIOS_Names=%~2
    set BIOS_File_Name=%~n2
    shift & shift
    goto OptLoop
)
if /i "%~1"=="/C" (
    if "%~2"==""  goto Usage
    if not exist %~2 echo ConfigFile not found. & goto Usage
    set Stitch_Config=%~2
    shift & shift
    goto OptLoop
)
if /i "%~1"=="/S" (
    if "%~2"==""  goto Usage
    set IFWI_Suffix=%~2
    shift & shift
    goto OptLoop
)

if "%BIOS_File_Name:~0,4%"=="MNW2" (
   set Stitch_Config= MNW2_Stitch_Config.txt
)
if "%BIOS_File_Name:~3,4%"=="MNW2" (
   set Stitch_Config= MNW2_Stitch_Config.txt
)

:: if no rom specified by user, search in ./ for ROM files
if "%BIOS_Names%"=="" (
    set "BIOS_Names= "
    for /f "tokens=*" %%i in ('dir /b *.rom') do set BIOS_Names=!BIOS_Names! %%i
    if "!BIOS_Names!"==" " (
        echo NO .ROM files found !!!
        goto Usage
    )
)

:: Parse the Stitch_Config File
if not exist %Stitch_Config% (
    echo Stitch Configuration File %Stitch_Config% not found.
    goto ScriptFail
)
for /f "delims== tokens=1,2" %%i in (%Stitch_Config%) do (
    if /i "%%i"=="HEADER"      set IFWI_HEADER=%%j
    if /i "%%i"=="SEC_VERSION" set SEC_VERSION=%%j
    if /i "%%i"=="Source" (
        if /i "%%j"=="ALPHA" set Source_Prefix=A_
        if /i "%%j"=="BF" set Source_Prefix=BF_
        if /i "%%j"=="BE" set Source_Prefix=BE_
        if /i "%%j"=="PV" set Source_Prefix=PV_
        if /i "%%j"=="PR1" set Source_Prefix=PR1_
    )
)


:: **********************************************************************
:: The Main Stitching Loop
:: **********************************************************************
echo %date%  %time% >>Stitching.log 2>&1
echo %date%  %time%
echo.
for %%i in (%BIOS_Names%) do (

    REM  ----- Do NOT use :: for comments Inside of code blocks() -------
    set BIOS_Rom=%%i
    set BIOS_Name=%%~ni
    set BIOS_Version=!BIOS_Name:~-7,7!

    REM extract PlatformType from BIOS filename
    set Platform_Type=!BIOS_Name:~0,4!

    REM Special treat for BayLake FFD8
    set Temp_Name=!BIOS_Name:~0,7!


    REM Capitalize and validate the Platform_Type
    if /i "!Platform_Type!"=="MNW2" (
        set Platform_Type=MNW2
    ) else (
        echo Error - Unsupported PlatformType: !Platform_Type!
        goto Usage
    )


    REM search BIOS_Name for Arch substring:  either IA32 or X64
    if not "!BIOS_Name!"=="!BIOS_Name:_IA32_=!" (
        set Arch=IA32
    ) else if not "!BIOS_Name!"=="!BIOS_Name:_X64_=!" (
        set Arch=X64
    ) else (
        echo Error:  Could not determine Architecture for !BIOS_Rom!
        goto Usage
    )
    set IFWI_Prefix=!Platform_Type!_IFWI_%Source_Prefix%!Arch!_!!BIOS_Version!

    REM search BIOS_Name for Build_Target substring: either R or D
    if not "!BIOS_Name!"=="!BIOS_Name:_R_=!" (
        set Build_Target=Release
        set IFWI_Prefix=!IFWI_Prefix!_R
    ) else if not "!BIOS_Name!"=="!BIOS_Name:_D_=!" (
        set Build_Target=Debug
        set IFWI_Prefix=!IFWI_Prefix!_D
    ) else (
        echo Error:  Could not determine Build Target for !BIOS_Rom!
        goto Usage
    )

    REM Create a BIOS backup before Stitching
    if %BackupRom% EQU 1 (
        echo Creating backup of original BIOS rom.
        copy /y !BIOS_Rom! !BIOS_Rom!.orig >nul
    )

    echo.  >>Stitching.log
    echo ********** Stitching !BIOS_Rom! **********  >>Stitching.log
    echo.  >>Stitching.log
    echo.
    echo Stitching IFWI for !BIOS_Rom! ...
    echo ---------------------------------------------------------------------------
    echo IFWI  Header: !IFWI_HEADER!.bin,   SEC version: !SEC_VERSION!,   
    echo BIOS Version: !BIOS_Version!

    echo Platform Type: !Platform_Type!,     IFWI Prefix: %BIOS_ID%
    echo ---------------------------------------------------------------------------

    echo -----------------------------
    echo.
    echo Generating IFWI... %BIOS_ID%.bin
    echo.
    copy /b/y IFWIHeader\!IFWI_HEADER!.bin + ..\..\Vlv2MiscBinariesPkg\SEC\!SEC_VERSION!\VLV_SEC_REGION.bin + IFWIHeader\Vacant.bin + !BIOS_Rom! %BIOS_ID%.bin
    echo.
    echo ===========================================================================
)
@echo off

::**********************************************************************
:: end of main loop
::**********************************************************************

echo.
echo  -- All specified ROM files Stitched. --
echo.
goto Exit

:Usage
echo.
echo **************************************************************************************************
echo This Script is used to Stitch together BIOS, GOP Driver, Microcode Patch and TXE FW
echo into a single Integrated Firmware Image (IFWI).
echo.
echo Usage: IFWIStitch.bat [flags] [/B BIOS.ROM] [/C Stitch_Config] [/S IFWI_Suffix]
echo.
echo    This script has NO Required arguments, so that the user can just double click from the GUI.
echo    However, this requires that the BIOS.ROM file name is formatted correctly.
echo.
echo    /nG             Do NOT update the GOP driver.  (applies to all ROM files for this run)
echo    /nV             Do NOT update the VBIOS.       (applies to all ROM files for this run)
echo    /nM             Do NOT update the Microcode.   (applies to all ROM files for this run)
echo    /nB             Do NOT backup BIOS.ROMs. (Default will backup to BIOS.ROM.Orig)
echo.
echo    BIOS.ROM:       A single BIOS ROM file to use for stitching
echo                    (DEFAULT: ALL .ROM files inside the current directory)
echo    Stitch_Config:  Text file containing version info of each FW component
echo                    (DEFAULT: Stitch_Config.txt)
echo    IFWI_Suffix:    Suffix to append to the end of the IFWI filename
echo                    (DEFAULT: YYYY_MM_DD_HHMM)
echo.
echo Examples:
echo    IFIWStitch.bat                                      : Stitch all ROMs with defaults
echo    IFIWStitch.bat /B C:/MyRoms/testBIOS.rom            : Stitch single ROM with defaults
echo    IFIWStitch.bat /B ../testBIOS.rom /S test123        : Stitch single ROM and add custom suffix
echo    IFIWStitch.bat /nM /nB /B testBIOS.rom /S test456   : Stitch single ROM, keep uCode from .rom,
echo                                                          don't create backup, and add custom suffix.
echo ****************************************************************************************************
pause
exit /b 1

:ScriptFail
set exitCode=1

:Exit
echo  -- See Stitching.log for more info. --
echo.
echo %date%  %time%
echo.
if "%Platform_Type%"=="MNW2" (
  echo .
) else (
  echo only support MNW2 for this project!
pause
)
exit /b %exitCode%
EndLocal
