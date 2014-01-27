@REM ## @file
@REM #
@REM # Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
@REM # This program and the accompanying materials
@REM # are licensed and made available under the terms and conditions of the BSD License
@REM # which accompanies this distribution.    The full text of the license may be found at
@REM # http://opensource.org/licenses/bsd-license.php
@REM #
@REM # THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM # WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


@echo off
setlocal
SET NMAKE_COMMAND=%1
SHIFT

:loop
if "%1"=="" goto success

ECHO Building %1
pushd %1
nmake %NMAKE_COMMAND%
if ERRORLEVEL 1 goto error
ECHO %1 built successfully (%NMAKE_COMMAND%)
ECHO.
shift
popd
goto loop

:success
goto exit

:error
popd
ENDLOCAL
ECHO Error while making %1!
VERIFY OTHER 2>NUL

:exit
