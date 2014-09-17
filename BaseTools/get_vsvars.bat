@REM @file
@REM   Windows batch file to find the Visual Studio set up script
@REM
@REM Copyright (c) 2013-2014, ARM Limited. All rights reserved.

@REM This program and the accompanying materials
@REM are licensed and made available under the terms and conditions of the BSD License
@REM which accompanies this distribution.  The full text of the license may be found at
@REM http://opensource.org/licenses/bsd-license.php
@REM
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM


@echo off
goto  :main

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
  if defined VS140COMNTOOLS  call :read_vsvars  "%VS140COMNTOOLS%"
  if defined VS130COMNTOOLS  call :read_vsvars  "%VS130COMNTOOLS%"
  if defined VS120COMNTOOLS  call :read_vsvars  "%VS120COMNTOOLS%"
  if defined VS110COMNTOOLS  call :read_vsvars  "%VS110COMNTOOLS%"
  if defined VS100COMNTOOLS  call :read_vsvars  "%VS100COMNTOOLS%"
  if defined VS90COMNTOOLS   call :read_vsvars  "%VS90COMNTOOLS%"
  if defined VS80COMNTOOLS   call :read_vsvars  "%VS80COMNTOOLS%"
  if defined VS71COMNTOOLS   call :read_vsvars  "%VS71COMNTOOLS%"

:done
set GET_VSVARS_BAT_CHECK_DIR=
