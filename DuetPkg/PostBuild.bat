@echo off
@REM ## @file
@REM #
@REM #  Post build script that will be automatically run after build.
@REM #
@REM #  Copyright (c) 2010 - 2016, Intel Corporation. All rights reserved.<BR>
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

set BASETOOLS_DIR=%EDK_TOOLS_BIN%
set BOOTSECTOR_BIN_DIR=%WORKSPACE%\DuetPkg\BootSector\bin

:SetDefault
set TARGET_ARCH=
set TARGET=
set TOOL_CHAIN_TAG=

:ParseParamsLoop
if "%1"=="" goto EndPParseParamsLoop
if /I "%1"=="-p" goto ParseParamsLoopNext
if /I "%1"=="-a" set TARGET_ARCH=%2& goto ParseParamsLoopNext
if /I "%1"=="-b" set TARGET=%2& goto ParseParamsLoopNext
if /I "%1"=="-t" set TOOL_CHAIN_TAG=%2& goto ParseParamsLoopNext
if /I "%1"=="-h" goto Help

:ParseParamsLoopNext
shift
shift
goto ParseParamsLoop

:EndPParseParamsLoop
if "%TARGET_ARCH%"=="" goto Help
if "%TARGET%"=="" goto Help
if "%TOOL_CHAIN_TAG%"=="" goto Help

set PROCESSOR=%TARGET_ARCH%
set BUILD_DIR=%WORKSPACE%\Build\DuetPkg%PROCESSOR%\%TARGET%_%TOOL_CHAIN_TAG%

@REM Store environment variables used by CreateBootDisk.bat
echo set TARGET=%TARGET%> %WORKSPACE%\DuetPkg\SetEnv_%PROCESSOR%.bat
echo set TOOL_CHAIN_TAG=%TOOL_CHAIN_TAG%>> %WORKSPACE%\DuetPkg\SetEnv_%PROCESSOR%.bat

echo Compressing DUETEFIMainFv.FV ...
%BASETOOLS_DIR%\LzmaCompress -e -o %BUILD_DIR%\FV\DUETEFIMAINFV.z %BUILD_DIR%\FV\DUETEFIMAINFV.Fv

echo Compressing DxeMain.efi ...
%BASETOOLS_DIR%\LzmaCompress -e -o %BUILD_DIR%\FV\DxeMain.z %BUILD_DIR%\%PROCESSOR%\DxeCore.efi

echo Compressing DxeIpl.efi ...
%BASETOOLS_DIR%\LzmaCompress -e -o %BUILD_DIR%\FV\DxeIpl.z %BUILD_DIR%\%PROCESSOR%\DxeIpl.efi

echo Generate Loader Image ...
if "%PROCESSOR%"=="IA32" goto GENERATE_IMAGE_IA32
if "%PROCESSOR%"=="X64" goto GENERATE_IMAGE_X64

:GENERATE_IMAGE_IA32
%BASETOOLS_DIR%\EfiLdrImage.exe -o %BUILD_DIR%\FV\Efildr32 %BUILD_DIR%\%PROCESSOR%\EfiLoader.efi %BUILD_DIR%\FV\DxeIpl.z %BUILD_DIR%\FV\DxeMain.z %BUILD_DIR%\FV\DUETEFIMAINFV.z
copy /b %BOOTSECTOR_BIN_DIR%\Start.com+%BOOTSECTOR_BIN_DIR%\Efi32.com2+%BUILD_DIR%\FV\Efildr32 %BUILD_DIR%\FV\Efildr
copy /b %BOOTSECTOR_BIN_DIR%\Start16.com+%BOOTSECTOR_BIN_DIR%\Efi32.com2+%BUILD_DIR%\FV\Efildr32 %BUILD_DIR%\FV\Efildr16
copy /b %BOOTSECTOR_BIN_DIR%\Start32.com+%BOOTSECTOR_BIN_DIR%\Efi32.com2+%BUILD_DIR%\FV\Efildr32 %BUILD_DIR%\FV\Efildr20
goto end

:GENERATE_IMAGE_X64
%BASETOOLS_DIR%\EfiLdrImage.exe -o %BUILD_DIR%\FV\Efildr64 %BUILD_DIR%\%PROCESSOR%\EfiLoader.efi %BUILD_DIR%\FV\DxeIpl.z %BUILD_DIR%\FV\DxeMain.z %BUILD_DIR%\FV\DUETEFIMAINFV.z
copy /b %BOOTSECTOR_BIN_DIR%\Start64.com+%BOOTSECTOR_BIN_DIR%\Efi64.com2+%BUILD_DIR%\FV\Efildr64 %BUILD_DIR%\FV\EfildrPure
%BASETOOLS_DIR%\GenPage.exe %BUILD_DIR%\FV\EfildrPure -o %BUILD_DIR%\FV\Efildr
copy /b %BOOTSECTOR_BIN_DIR%\St16_64.com+%BOOTSECTOR_BIN_DIR%\Efi64.com2+%BUILD_DIR%\FV\Efildr64 %BUILD_DIR%\FV\Efildr16Pure
%BASETOOLS_DIR%\GenPage.exe %BUILD_DIR%\FV\Efildr16Pure -o %BUILD_DIR%\FV\Efildr16
copy /b %BOOTSECTOR_BIN_DIR%\St32_64.com+%BOOTSECTOR_BIN_DIR%\Efi64.com2+%BUILD_DIR%\FV\Efildr64 %BUILD_DIR%\FV\Efildr20Pure
%BASETOOLS_DIR%\GenPage.exe %BUILD_DIR%\FV\Efildr20Pure -o %BUILD_DIR%\FV\Efildr20
goto end

:Help
echo Usage: This script will be run automatically after build.
:end
