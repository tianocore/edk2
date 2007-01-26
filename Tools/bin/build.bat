@REM
@REM Copyright (c) 2006, Intel Corporation
@REM All rights reserved. This program and the accompanying materials
@REM are licensed and made available under the terms and conditions of the BSD License
@REM which accompanies this distribution.  The full text of the license may be found at
@REM http://opensource.org/licenses/bsd-license.php
@REM 
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM
@echo off

set _ARGS= 
:check_arg
if ""%1""=="""" goto arg_end
if ""%1""==""-q"" goto ant_arg
if ""%1""==""-v"" goto ant_arg
if ""%1""==""-d"" goto ant_arg
if ""%1""==""-e"" goto ant_arg
if ""%1""==""-emacs"" goto ant_arg

goto ant_target

:ant_arg    
    set _ARGS=%_ARGS% %1
    shift
    goto check_arg

:ant_target
    set _ARGS=%_ARGS% -DBUILD_TARGET=%1
    shift
    goto check_arg

:arg_end
ant -logger org.tianocore.build.global.GenBuildLogger -f %WORKSPACE%/build.xml %_ARGS%

set _ARGS=
@echo on

