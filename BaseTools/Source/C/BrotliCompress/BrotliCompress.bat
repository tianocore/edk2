@REM @file
@REM This script will exec Brotli tool with -e/-d options.
@REM
@REM Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
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

set LVL=--quality 9

:Begin
if "%1"=="" goto End

if "%1"=="-d" (
  set ARGS=%ARGS% --decompress
  shift
  goto Begin
)

if "%1"=="-e" (
  shift
  goto Begin
)

if "%1"=="-g" (
  set ARGS=%ARGS% --gap %2
  shift
  shift
  goto Begin
)

if "%1"=="-l" (
  set LVL=--quality %2
  shift
  shift
  goto Begin
)

if "%1"=="-o" (
  set ARGS=%ARGS% --output %2
  set INTMP=%2
  shift
  shift
  goto Begin
)

set ARGS=%ARGS% --input %1
shift
goto Begin

:End
Brotli %ARGS% %LVL%
@echo on
