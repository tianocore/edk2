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

@echo off

REM ##############################################################
REM # You should not have to modify anything below this line
REM #

if /I "%1"=="-h" goto usage
if /I "%1"=="-help" goto usage
if /I "%1"=="--help" goto usage
if /I "%1"=="/h" goto usage
if /I "%1"=="/?" goto usage
if /I "%1"=="/help" goto usage

if NOT "%1"=="" set EDK_TOOLS_PATH=%1

REM
REM Check the required system environment variables
REM

:check_vc
if defined VCINSTALLDIR goto setup_workspace
if defined VS71COMNTOOLS (
  call "%VS71COMNTOOLS%\vsvars32.bat"
) else (
  echo.
  echo !!!WARNING!!! Cannot find Visual Studio !!!
  echo.
)

:setup_workspace
REM
REM check the EDK_TOOLS_PATH
REM
if not defined EDK_TOOLS_PATH goto no_tools_path
if exist %EDK_TOOLS_PATH% goto set_path
echo.
echo !!!WARNING!!! %EDK_TOOLS_PATH% doesn't exist. %WORKSPACE%\Tools will be used !!!
echo.

:no_tools_path
if exist %WORKSPACE%\BaseTools (
  set EDK_TOOLS_PATH=%WORKSPACE%\BaseTools
) else (
  echo.
  echo !!!WARNING!!! No tools path found. Please set EDK_TOOLS_PATH !!!
  echo.
  goto end
)

:set_path
if defined WORKSPACE_TOOLS_PATH goto check_path
set PATH=%EDK_TOOLS_PATH%\Bin;%EDK_TOOLS_PATH%\Bin\Win32;%PATH%
set WORKSPACE_TOOLS_PATH=%EDK_TOOLS_PATH%
goto path_ok

:check_path
if "%EDK_TOOLS_PATH%"=="%WORKSPACE_TOOLS_PATH%" goto path_ok
set PATH=%EDK_TOOLS_PATH%\Bin;%EDK_TOOLS_PATH%\Bin\Win32;%PATH%
set WORKSPACE_TOOLS_PATH=%EDK_TOOLS_PATH%
echo Resetting the PATH variable to include the EDK_TOOLS_PATH for this WORKSPACE

:path_ok
echo           PATH = %PATH%
echo.
echo      WORKSPACE = %WORKSPACE%
echo EDK_TOOLS_PATH = %EDK_TOOLS_PATH%
echo.

REM
REM copy *.template to %WORKSPACE%\Conf
REM
if NOT exist %WORKSPACE%\Conf mkdir %WORKSPACE%\Conf
if NOT exist %WORKSPACE%\Conf\FrameworkDatabase.db (
  echo copying ... FrameworkDatabase.template to %WORKSPACE%\Conf\FrameworkDatabase.db
  copy %EDK_TOOLS_PATH%\Conf\FrameworkDatabase.template %WORKSPACE%\Conf\FrameworkDatabase.db > nul
)
if NOT exist %WORKSPACE%\Conf\target.txt (
  echo copying ... target.template to %WORKSPACE%\Conf\target.txt
  copy %EDK_TOOLS_PATH%\Conf\target.template %WORKSPACE%\Conf\target.txt > nul
)
if NOT exist %WORKSPACE%\Conf\tools_def.txt (
  echo copying ... tools_def.template to %WORKSPACE%\Conf\tools_def.txt
  copy %EDK_TOOLS_PATH%\Conf\tools_def.template %WORKSPACE%\Conf\tools_def.txt > nul
)
if NOT exist %WORKSPACE%\Conf\build_rule.txt (
  echo copying ... build_rule.template to %WORKSPACE%\Conf\build_rule.txt
  copy %EDK_TOOLS_PATH%\Conf\build_rule.template %WORKSPACE%\Conf\build_rule.txt > nul
)

REM
REM copy XMLSchema to %EDK_TOOLS_PATH%\Conf\XMLSchema
REM
REM echo copying ... XMLSchema to %EDK_TOOLS_PATH%\Conf\XMLSchema
REM xcopy %WORKSPACE%\Conf\XMLSchema %EDK_TOOLS_PATH%\Conf\XMLSchema /S /I /D /F /Q > nul

REM
REM Done!!!
REM
goto end

:usage
echo.
echo  "Usage: %0 [/? | /h | /help | -h | -help | --help] [tools_path]"
echo.
echo                      tools_path       Tools' path. EDK_TOOLS_PATH will be set to this path.
echo.

:end
@echo on

