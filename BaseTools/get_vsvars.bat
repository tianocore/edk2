@REM @file
@REM   Windows batch file to find the Visual Studio set up script
@REM
@REM Copyright (c) 2013-2014, ARM Limited. All rights reserved.

@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM


@echo off
set SCRIPT_ERROR=0
if "%1"=="" goto main
if /I "%1"=="VS2019" goto VS2019Vars
if /I "%1"=="VS2017" goto VS2017Vars
if /I "%1"=="VS2015" goto VS2015Vars
if /I "%1"=="VS2013" goto VS2013Vars
if /I "%1"=="VS2012" goto VS2012Vars

:set_vsvars
if defined VCINSTALLDIR goto :EOF
  call %* > vswhereInfo
  for /f "usebackq tokens=1* delims=: " %%i in (vswhereInfo) do (
    if /i "%%i"=="installationPath" (
      call "%%j\VC\Auxiliary\Build\vcvars32.bat"
    )
  )
  del vswhereInfo
goto :EOF

:read_vsvars
@rem Do nothing if already found, otherwise call vsvars32.bat if there
if defined VCINSTALLDIR goto :EOF
  set GET_VSVARS_BAT_CHECK_DIR=%*
  set GET_VSVARS_BAT_CHECK_DIR=%GET_VSVARS_BAT_CHECK_DIR:"=%
  if exist  "%GET_VSVARS_BAT_CHECK_DIR%\vsvars32.bat"  call "%GET_VSVARS_BAT_CHECK_DIR%\vsvars32.bat"
:vsvars_done
goto :EOF


:ToolNotInstall
set SCRIPT_ERROR=1
goto :EOF

REM NOTE: This file will find the most recent Visual Studio installation
REM       apparent from the environment.
REM       To use an older version, modify your environment set up.
REM       (Or invoke the relevant vsvars32 file beforehand).

:main
if defined VCINSTALLDIR goto :done
  :VS2019Vars
  if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools" (
      call :set_vsvars "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -products Microsoft.VisualStudio.Product.BuildTools -version 16,17
    ) else (
      call :set_vsvars "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -version 16,17
    )
  )
  if exist "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2019\BuildTools" (
      call :set_vsvars "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" -products Microsoft.VisualStudio.Product.BuildTools -version 16,17
    ) else (
      call :set_vsvars "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" -version 16,17
    )
  )
  if /I "%1"=="VS2019" goto ToolNotInstall

  :VS2017Vars
  if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\BuildTools" (
      call :set_vsvars "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -products Microsoft.VisualStudio.Product.BuildTools -version 15,16
    ) else (
      call :set_vsvars "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -version 15,16
    )
  )
  if exist "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" (
    if exist "%ProgramFiles%\Microsoft Visual Studio\2017\BuildTools" (
      call :set_vsvars "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" -products Microsoft.VisualStudio.Product.BuildTools -version 15,16
    ) else (
      call :set_vsvars "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" -version 15,16
    )
  )
  if /I "%1"=="VS2017" goto ToolNotInstall

  :VS2015Vars
  if defined VS140COMNTOOLS (call :read_vsvars  "%VS140COMNTOOLS%") else (if /I "%1"=="VS2015" goto ToolNotInstall)

  :VS2013Vars
  if defined VS120COMNTOOLS ( call :read_vsvars  "%VS120COMNTOOLS%") else (if /I "%1"=="VS2013" goto ToolNotInstall)

  :VS2012Vars
  if defined VS110COMNTOOLS (call :read_vsvars  "%VS110COMNTOOLS%") else (if /I "%1"=="VS2012" goto ToolNotInstall)

  if defined VS100COMNTOOLS  call :read_vsvars  "%VS100COMNTOOLS%"
  if defined VS90COMNTOOLS   call :read_vsvars  "%VS90COMNTOOLS%"
  if defined VS80COMNTOOLS   call :read_vsvars  "%VS80COMNTOOLS%"
  if defined VS71COMNTOOLS   call :read_vsvars  "%VS71COMNTOOLS%"

:done
set GET_VSVARS_BAT_CHECK_DIR=
