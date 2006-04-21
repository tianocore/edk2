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
if "%JAVA_HOME%"=="" goto no_jdk
if "%WORKSPACE%"=="" goto no_wks

if "%1"=="" (goto usage)
if "%1"=="-h" (goto usage)
if "%1"=="-H" (goto usage) else (goto all)

:usage
echo.
echo Generate SurfaceArea Java Beans from schema
echo.
echo Usage: 
echo        SABeans.bat SurfaceArea.jar
echo            generate compiled SurfaceArea Java Beans only
echo.
echo        SABeans.bat SurfaceArea.java
echo            generate source code of SurfaceArea Java Beans only
echo.
echo        SABeans.bat SurfaceArea
echo            generate both compiled and source code of SurfaceArea Java Beans
echo.
goto end

:all
  ant -f %WORKSPACE%\Tools\Source\SurfaceArea\build.xml %1
  goto end

:no_jdk
  echo.
  echo !!! Please set JAVA_HOME !!!
  echo.
  goto end

:no_wks
  echo.
  echo !!! Please set WORKSPACE !!!
  echo.
  goto end

:end
@echo on

