@REM
@REM Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
@REM This program and the accompanying materials
@REM are licensed and made available under the terms and conditions of the BSD License
@REM which accompanies this distribution.  The full text of the license may be found at
@REM http://opensource.org/licenses/bsd-license.php
@REM
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM

@echo off
@if /I "%1"=="-h" goto Usage
@if /I "%1"=="-help" goto Usage
@if /I "%1"=="--help" goto Usage
@if /I "%1"=="/h" goto Usage
@if /I "%1"=="/help" goto Usage
@if /I "%1"=="/?" goto Usage

set IMPORT_TOOL=%~dp0Trim.exe
if NOT exist %IMPORT_TOOL% (
  echo.
  echo !!! Trim.exe was not found. Please make sure that it is in the same directory as this script!
  echo.
  goto End
)

if '%*'=='' (
  set FILE_LIST=*.c
) else (
  set FILE_LIST=%*
)

for /r %%i in (%FILE_LIST%) do (
  echo Converting ... %%i
  %IMPORT_TOOL% -8 -o %%i %%i
)
goto End

:Usage
  echo.
  echo  Usage: "%0 [-h | -help | --help | /h | /help | /?] [files]"
  echo.
  echo         files          File list or file pattern with wildcard, like "*.c *.h",
  echo                        seperated by space. If not specified, defaul to *.c.
echo.

:End
set FILE_LIST=
set IMPORT_TOOL=

@echo on

