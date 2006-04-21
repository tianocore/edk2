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

REM @echo off

:check_java
if "%JAVA_HOME%"=="" goto no_jdk
:check_wks
if "%WORKSPACE%"=="" goto no_wks

set ANT_HOME=%WORKSPACE%\Tools\bin\apache-ant
set PATH=%JAVA_HOME%\bin;%ANT_HOME%\bin;%WORKSPACE%\Tools\bin;%XMLBEANS_HOME%\bin;%PATH%

call "ant" -f %WORKSPACE%\Tools\Source\CreateMdkPkg\build.xml

echo DONE

goto end

:no_jdk
echo.
echo !!! Please set JAVA_HOME !!!
echo.
goto check_wks

:no_wks
echo.
echo !!! Please set WORKSPACE !!!
echo.
goto end

:end
@echo on

