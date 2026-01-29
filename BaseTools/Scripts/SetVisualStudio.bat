@REM @file
@REM Windows batch file to set up the Microsoft Visual Studio environment
@REM
@REM This script is used to set up one of the Microsoft Visual Studio
@REM environments, VS2015 for
@REM building the Nt32Pkg/Nt32Pkg.dsc emulation environment to run on
@REM an X64 version of Windows.
@REM The system environment variables in this script are set by the
@rem Edk2Setup.bat script (that will be renamed to edksetup.bat).
@REM
@REM This script can also be used to build the Win32 binaries
@REM
@REM Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM
@echo off
@if defined NT32_X64 @goto CheckLatest
@if "%REBUILD_TOOLS%"=="TRUE" @goto RebuildTools

:CheckLatest
echo.
@if defined VS140COMNTOOLS (
   @set "COMMONTOOLSx64=C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\x86_amd64"
   @goto SetVs
)
@echo.
@echo No version of Microsoft Visual Studio was found on this system
@echo.
@exit /B 1

@REM Set up the X64 environment for building Nt32Pkg/Nt32Pkg.dsc to run on an X64 platform
:SetVs
if exist "%COMMONTOOLSx64%\vcvarsx86_amd64.bat" (
            @call "%COMMONTOOLSx64%\vcvarsx86_amd64.bat"
            @if errorlevel 1 (
                @echo. ERROR setting Microsoft Visual Studio %1
                @set COMMONTOOLSx64=
                @exit /B 1
            )
)
if not exist "%COMMONTOOLSx64%\vcvarsx86_amd64.bat" (
    @echo ERROR : This script does not exist: "%COMMONTOOLSx64%\vcvarsx86_amd64.bat"
    @set COMMONTOOLSx64=
    @exit /B 1
)
@set COMMONTOOLSx64=
@goto End

:RebuildTools
@call python "%BASE_TOOLS_PATH%\Scripts\UpdateBuildVersions.py"
@set "BIN_DIR=%EDK_TOOLS_PATH%\Bin\Win32"
if not exist "%BIN_DIR%" @mkdir "%BIN_DIR%"
@echo Removing temporary and binary files
@cd "%BASE_TOOLS_PATH%"
@call nmake cleanall
@echo Rebuilding the EDK II BaseTools
@cd "%BASE_TOOLS_PATH%\Source\C"
@call nmake -nologo -a -f Makefile
@if errorlevel 1 (
@echo Error building the C-based BaseTools
@cd "%WORKSPACE%"
@exit /B1
)
@cd %BASE_TOOLS_PATH%\Source\Python
@call nmake -nologo -a -f Makefile
@if errorlevel 1 (
@echo Error building the Python-based BaseTools
@cd %WORKSPACE%
@exit /B1
)
@cd %WORKSPACE%

@goto End

:VersionNotFound
@echo.
@echo This Microsoft Visual Studio version is in not installed on this system: %1
@echo.
@exit /B 1

:End
@exit /B 0
