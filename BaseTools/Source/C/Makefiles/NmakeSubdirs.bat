@REM ## @file
@REM # Makefile
@REM #
@REM # Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
@REM # SPDX-License-Identifier: BSD-2-Clause-Patent
@REM #

@echo off
setlocal
set TOOL_ERROR=0
SET NMAKE_COMMAND=%1
SHIFT

:loop
if "%1"=="" goto success

ECHO Building %1
pushd %1
nmake %NMAKE_COMMAND%
if ERRORLEVEL 1 (
  set /A TOOL_ERROR= %TOOL_ERROR% + %ERRORLEVEL%
  goto error
)
ECHO %1 built successfully (%NMAKE_COMMAND%)
ECHO.
shift
popd
goto loop

:success
goto exit

:error
popd
set /A TOOL_ERROR=%TOOL_ERROR%+%ERRORLEVEL%
ECHO Error while making %1!
VERIFY OTHER 2>NUL

:exit
exit /B %TOOL_ERROR%
