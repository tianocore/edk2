@REM @file
@REM   Windows batch file to find the Visual Studio set up script
@REM
@REM Copyright (c) 2013-2014, ARM Limited. All rights reserved.

@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM


@echo off
goto  :main

:set_vsvars
for /f "usebackq tokens=1* delims=: " %%i in (`%*`) do (
  if /i "%%i"=="installationPath" call "%%j\VC\Auxiliary\Build\vcvars32.bat"
)
goto :EOF

:read_vsvars
@rem Do nothing if already found, otherwise call vsvars32.bat if there
if defined VCINSTALLDIR goto :EOF
  set GET_VSVARS_BAT_CHECK_DIR=%*
  set GET_VSVARS_BAT_CHECK_DIR=%GET_VSVARS_BAT_CHECK_DIR:"=%
  if exist  "%GET_VSVARS_BAT_CHECK_DIR%\vsvars32.bat"  call "%GET_VSVARS_BAT_CHECK_DIR%\vsvars32.bat"
:vsvars_done
goto :EOF


REM NOTE: This file will find the most recent Visual Studio installation
REM       apparent from the environment.
REM       To use an older version, modify your environment set up.
REM       (Or invoke the relevant vsvars32 file beforehand).

:main
if defined VCINSTALLDIR goto :done
  if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"  call :set_vsvars "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
  if exist "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"       call :set_vsvars "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
  if defined VS140COMNTOOLS  call :read_vsvars  "%VS140COMNTOOLS%"
  if defined VS120COMNTOOLS  call :read_vsvars  "%VS120COMNTOOLS%"
  if defined VS110COMNTOOLS  call :read_vsvars  "%VS110COMNTOOLS%"
  if defined VS100COMNTOOLS  call :read_vsvars  "%VS100COMNTOOLS%"
  if defined VS90COMNTOOLS   call :read_vsvars  "%VS90COMNTOOLS%"
  if defined VS80COMNTOOLS   call :read_vsvars  "%VS80COMNTOOLS%"
  if defined VS71COMNTOOLS   call :read_vsvars  "%VS71COMNTOOLS%"

:done
set GET_VSVARS_BAT_CHECK_DIR=
