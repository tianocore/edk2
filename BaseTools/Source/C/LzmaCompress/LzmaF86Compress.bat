@REM @file
@REM This script will exec LzmaCompress tool with --f86 option that enables
@REM converter for x86 code.
@REM
@REM Copyright (c) 2012 - 2018, Intel Corporation. All rights reserved.<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

@echo off
@setlocal

:Begin
if "%1"=="" goto End
if "%1"=="-e" (
  set FLAG=--f86
)
if "%1"=="-d" (
  set FLAG=--f86
)
set ARGS=%ARGS% %1
shift
goto Begin

:End
LzmaCompress %ARGS% %FLAG%
@echo on
