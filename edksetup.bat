@REM @file
@REM   Windows batch file to setup a WORKSPACE environment
@REM
@REM Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
@REM (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

@REM set CYGWIN_HOME=C:\cygwin

@REM usage:
@REM   edksetup.bat [--nt32] [AntBuild] [Rebuild] [ForceRebuild] [Reconfig]
@REM if the argument, skip is present, only the paths and the
@REM test and set of environment settings are performed.

@REM ##############################################################
@REM # You should not have to modify anything below this line
@REM #

@echo off

@REM
@REM Set the WORKSPACE to the current working directory
@REM
pushd .
cd %~dp0

if not defined WORKSPACE (
  goto SetWorkSpace
)

if %WORKSPACE% == %CD% (
  @REM Workspace is not changed.
  goto ParseArgs
)

:SetWorkSpace
@REM set new workspace
if not defined WORKSPACE (
  set WORKSPACE=%CD%
)

:ParseArgs
if /I "%1"=="-h" goto Usage
if /I "%1"=="-help" goto Usage
if /I "%1"=="--help" goto Usage
if /I "%1"=="/h" goto Usage
if /I "%1"=="/?" goto Usage
if /I "%1"=="/help" goto Usage

if /I "%1"=="NewBuild" shift
if not defined EDK_TOOLS_PATH (
  goto SetEdkToolsPath
) else (
  goto checkNt32Flag
)

:SetEdkToolsPath
if %WORKSPACE:~-1% EQU \ (
  @set EDK_BASETOOLS=%WORKSPACE%BaseTools
) else (
  @set EDK_BASETOOLS=%WORKSPACE%\BaseTools
)
if exist %EDK_BASETOOLS% (
  set EDK_TOOLS_PATH=%EDK_BASETOOLS%
  set EDK_BASETOOLS=
) else (
  if defined PACKAGES_PATH (
    for %%i IN (%PACKAGES_PATH%) DO (
      if exist %%~fi\BaseTools (
        set EDK_TOOLS_PATH=%%~fi\BaseTools
        goto checkNt32Flag
      )
    )
  ) else (
    echo.
    echo !!! ERROR !!! Cannot find BaseTools !!!
    echo.
    goto BadBaseTools
  )
)

:checkNt32Flag
if exist %EDK_TOOLS_PATH%\Source set BASE_TOOLS_PATH=%EDK_TOOLS_PATH%

:checkBaseTools
IF NOT EXIST "%EDK_TOOLS_PATH%\toolsetup.bat" goto BadBaseTools
call %EDK_TOOLS_PATH%\toolsetup.bat %*
if %ERRORLEVEL% NEQ 0 goto end
if /I "%1"=="Reconfig" shift
goto check_NASM
goto check_cygwin

:BadBaseTools
  @REM
  REM Need the BaseTools Package in order to build
  @REM
  @echo.
  @echo !!! ERROR !!! The BaseTools Package was not found !!!
  @echo.
  @echo Set the system environment variable, EDK_TOOLS_PATH to the BaseTools,
  @echo For example,
  @echo   set EDK_TOOLS_PATH=C:\MyTools\BaseTools
  @echo The setup script, toolsetup.bat must reside in this folder.
  @echo.
  goto end

:check_NASM
if not defined NASM_PREFIX (
    @echo.
    @echo !!! WARNING !!! NASM_PREFIX environment variable is not set
    @if exist "C:\nasm\nasm.exe" @set "NASM_PREFIX=C:\nasm\"
    @if exist "C:\nasm\nasm.exe" @echo   Found nasm.exe, setting the environment variable to C:\nasm\
    @if not exist "C:\nasm\nasm.exe" echo   Attempting to build modules that require NASM will fail.
)

:check_CLANGPDB
@REM In Windows, set CLANG_HOST_BIN=n to use nmake command
@set CLANG_HOST_BIN=n
if not defined CLANG_BIN (
    @echo.
    @echo !!! WARNING !!! CLANG_BIN environment variable is not set
    @if exist "C:\Program Files\LLVM\bin\clang.exe" (
        @set "CLANG_BIN=C:\Program Files\LLVM\bin\"
        @echo   Found LLVM, setting CLANG_BIN environment variable to C:\Program Files\LLVM\bin\
    )
)

:check_cygwin
if defined CYGWIN_HOME (
  if not exist "%CYGWIN_HOME%" (
    @echo.
    @echo !!! WARNING !!! CYGWIN_HOME not found, gcc build may not be used !!!
    @echo.
  )
) else (
  if exist c:\cygwin (
    set CYGWIN_HOME=c:\cygwin
  ) else (
    @echo.
    @echo !!! WARNING !!! No CYGWIN_HOME set, gcc build may not be used !!!
    @echo.
  )
)

:cygwin_done
if /I "%1"=="Rebuild" shift
if /I "%1"=="ForceRebuild" shift
if /I "%1"=="VS2022" shift
if /I "%1"=="VS2019" shift
if /I "%1"=="VS2017" shift
if /I "%1"=="VS2015" shift
if "%1"=="" goto end

:Usage
  @echo.
  @echo  Usage: "%0 [-h | -help | --help | /h | /help | /?] [Reconfig] [Rebuild] [ForceRebuild] [VS2019] [VS2017] [VS2015]"
  @echo.
  @echo         Reconfig       Reinstall target.txt, tools_def.txt and build_rule.txt.
  @echo         Rebuild        Perform incremental rebuild of BaseTools binaries.
  @echo         ForceRebuild   Force a full rebuild of BaseTools binaries.
  @echo         VS2015         Set the env for VS2015 build.
  @echo         VS2017         Set the env for VS2017 build.
  @echo         VS2019         Set the env for VS2019 build.
  @echo         VS2022         Set the env for VS2022 build.
  @echo.
  @echo  Note that target.template, tools_def.template and build_rules.template
  @echo  will only be copied to target.txt, tools_def.txt and build_rule.txt
  @echo  respectively if they do not exist. Use option [Reconfig] to force the copy.
  @echo.
  goto end

:end
  popd
