@REM ## @file
@REM #
@REM #  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
@REM #
@REM #  This program and the accompanying materials
@REM #  are licensed and made available under the terms and conditions of the BSD License
@REM #  which accompanies this distribution. The full text of the license may be found at
@REM #  http://opensource.org/licenses/bsd-license.php
@REM #  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM #  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM #
@REM #
@REM ##


@REM Read the variables from Conf/target.txt
@REM Because we can't add '=' as a delimiter in 'for', each variable is read in 2 parts:
@REM First we read the "= xyz" part of the variable assignation which we use, along with
@REM the original equal sign for our first assignation. Then we trim any left whitespaces.
@REM NB: default token delimiters for "for /f" are tab and space.

@set CONFIG_FILE=%WORKSPACE%\Conf\target.txt

@for /f "tokens=1*" %%i in ('type %CONFIG_FILE% ^| find "TOOL_CHAIN_TAG" ^| find /V "#"') do @set TOOL_CHAIN_TAG%%j
@for /f "tokens=*" %%i in ("%TOOL_CHAIN_TAG%") do @set TOOL_CHAIN_TAG=%%i

@for /f "tokens=1*" %%i in ('type %CONFIG_FILE% ^| find "TARGET" ^| find /V "#" ^| find /V "TARGET_ARCH"') do @set TARGET%%j
@for /f "tokens=*" %%i in ("%TARGET%") do @set TARGET=%%i

@for /f "tokens=1*" %%i in ('type %CONFIG_FILE% ^| find "TARGET_ARCH" ^|find /V "#"') do @set TARGET_ARCH%%j
@for /f "tokens=*" %%i in ("%TARGET_ARCH%") do @set TARGET_ARCH=%%i


@REM Set defaults if above variables are undefined in target.txt

@if "%TOOL_CHAIN_TAG%%"=="" @set TOOL_CHAIN_TAG=MYTOOLS
@if "%TARGET%"=="" @set TARGET=DEBUG
@if "%TARGET_ARCH%"=="" @set TARGET_ARCH=IA32
