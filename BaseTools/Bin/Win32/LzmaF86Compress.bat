@REM
@REM Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
@REM This program and the accompanying materials
@REM are licensed and made available under the terms and conditions of the BSD License
@REM which accompanies this distribution.  The full text of the license may be found at
@REM http://opensource.org/licenses/bsd-license.php
@REM
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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