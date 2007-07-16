@REM
@REM Copyright (c) 2006, Intel Corporation
@REM All rights reserved. This program and the accompanying materials
@REM are licensed and made available under the terms and conditions of the BSD License
@REM which accompanies this distribution.  The full text of the license may be found at
@REM http://opensource.org/licenses/bsd-license.php
@REM
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM

@REM usage: edksetup.bat [Reconfig]
@REM if the argument, skip is present, only the paths and the
@REM test and set of environment settings are performed. 

@REM ##############################################################
@REM # You should not have to modify anything below this line
@REM #

@echo off

@REM
@REM Set the WORKSPACE to the current working directory
@REM
if not defined WORKSPACE (
  @set WORKSPACE=%CD%
) else (
  @echo WORKSPACE was already set to %WORKSPACE%
)


@if /I "%1"=="-h" goto Usage
@if /I "%1"=="-help" goto Usage
@if /I "%1"=="--help" goto Usage
@if /I "%1"=="/h" goto Usage
@if /I "%1"=="/?" goto Usage
@if /I "%1"=="/help" goto Usage

if defined CYGWIN_HOME goto NewBuild
if exist c:\cygwin (
  set CYGWIN_HOME=c:\cygwin
) else (
  echo.
  echo !!! WARNING !!!! Not set CYGWIN_HOME, gcc build may not be used !!!
  echo.
)

goto NewBuild

:Usage
echo.
echo  Usage: %0 [Reconfig]
echo         Reconfig:      Reinstall target.txt, tools_def.txt, FrameworkDatabase.db. 
echo.
echo  Note that target.template, tools_def.template, FrameworkDatabase.template will be
echo  only copied to target.txt, tools_def.txt, FrameworkDatabase.db respectively if they
echo  are not existed. Using option [Reconfig] to do the force copy. 
echo.
goto end

:NewBuild
@IF DEFINED EDK_TOOLS_PATH goto RunToolSetup

@if exist %WORKSPACE%\BaseTools (
  @set EDK_TOOLS_PATH=%WORKSPACE%\BaseTools
) else (
  echo.
  echo The WORKSPACE does not contain a BaseTools directory and
  echo the EDK_TOOLS_PATH is not set
  echo.
  goto Usage
)

:RunToolSetup
@if  /I "%1"=="Reconfig" (
  @call %EDK_TOOLS_PATH%\toolsetup.bat Reconfig
) else (
  @call %EDK_TOOLS_PATH%\toolsetup.bat
)

:end
@echo on

