@REM @file
@REM   Windows batch file to setup a WORKSPACE environment
@REM
@REM Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
@REM This program and the accompanying materials
@REM are licensed and made available under the terms and conditions of the BSD License
@REM which accompanies this distribution.  The full text of the license may be found at
@REM http://opensource.org/licenses/bsd-license.php
@REM
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
  @goto SetWorkSpace
)

if %WORKSPACE% == %CD% (
  @REM Workspace is not changed.
  @goto ParseArgs
)

:SetWorkSpace
@REM set new workspace
@REM clear EFI_SOURCE and EDK_SOURCE for the new workspace
set WORKSPACE=%CD%
set EFI_SOURCE=
set EDK_SOURCE=

:ParseArgs
@if /I "%1"=="-h" goto Usage
@if /I "%1"=="-help" goto Usage
@if /I "%1"=="--help" goto Usage
@if /I "%1"=="/h" goto Usage
@if /I "%1"=="/?" goto Usage
@if /I "%1"=="/help" goto Usage

@if /I not "%1"=="--nt32" goto no_nt32

@REM Flag, --nt32 is set
@REM The Nt32 Emluation Platform requires Microsoft Libraries
@REM and headers to interface with Windows.

if not defined VCINSTALLDIR (
  if defined VS71COMNTOOLS (
    call "%VS71COMNTOOLS%\vsvars32.bat"
  ) else (
    if defined VS80COMNTOOLS (
      call "%VS80COMNTOOLS%\vsvars32.bat"
    ) else (
      if defined VS90COMNTOOLS (
        call "%VS90COMNTOOLS%\vsvars32.bat"
      ) else (
        echo.
        echo !!! WARNING !!! Cannot find Visual Studio !!!
        echo.
      )
    )
  )
)
shift

:no_nt32
@if /I "%1"=="NewBuild" shift
@if not defined EDK_TOOLS_PATH set EDK_TOOLS_PATH=%WORKSPACE%\BaseTools
@IF NOT EXIST "%EDK_TOOLS_PATH%\toolsetup.bat" goto BadBaseTools
@call %EDK_TOOLS_PATH%\toolsetup.bat %*
@if /I "%1"=="Reconfig" shift
@goto check_cygwin

:BadBaseTools
  @REM
  @REM Need the BaseTools Package in order to build
  @REM
  echo.
  echo !!! ERROR !!! The BaseTools Package was not found !!!
  echo.
  echo Set the system environment variable, EDK_TOOLS_PATH to the BaseTools,
  echo For example,
  echo   set EDK_TOOLS_PATH=C:\MyTools\BaseTools
  echo The setup script, toolsetup.bat must reside in this folder.
  echo.
  @goto end

:check_cygwin
  @if exist c:\cygwin (
    @set CYGWIN_HOME=c:\cygwin
  ) else (
    @echo.
    @echo !!! WARNING !!! No CYGWIN_HOME set, gcc build may not be used !!!
    @echo.
  )

@if NOT "%1"=="" goto Usage
@goto end

:Usage
  @echo.
  @echo  Usage: "%0 [-h | -help | --help | /h | /help | /?] [--nt32] [Reconfig]"
  @echo         --nt32         Call vsvars32.bat for NT32 platform build.
  @echo.
  @echo         Reconfig       Reinstall target.txt, tools_def.txt and build_rule.txt.
  @echo.
  @echo  Note that target.template, tools_def.template and build_rules.template
  @echo  will be only copied to target.txt, tools_def.txt and build_rule.txt
  @echo  respectively if they do not exist. Using option [Reconfig] to force the copy. 
  @echo.
  @goto end

:end
  @popd
  @echo on

