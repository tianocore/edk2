@REM @file
@REM This script will exec Brotli tool with -e/-d options.
@REM
@REM Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

@echo off
@setlocal

set QLT=-q 9 -w 22
set ARGS=

:Begin
if "%1"=="" goto End

if "%1"=="-d" (
  set ARGS=%ARGS% %1
  shift
  goto Begin
)

if "%1"=="-e" (
  shift
  goto Begin
)

if "%1"=="-g" (
  set ARGS=%ARGS% %1 %2
  shift
  shift
  goto Begin
)

if "%1"=="-o" (
  set ARGS=%ARGS% %1 %2
  shift
  shift
  goto Begin
)

if "%1"=="-q" (
  set QLT=%1 %2
  shift
  shift
  goto Begin
)

set ARGS=%ARGS% %1
shift
goto Begin

:End
Brotli %QLT% %ARGS%
@echo on
