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

@if "%JAVA_HOME%"=="" goto no_jdk
@if "%WORKSPACE%"=="" goto no_wks

@if "%1"=="" goto usage
@if "%1"=="-H" (goto usage)
@if "%1"=="-h" (goto usage) else (goto all)

:usage
@echo off
@echo.
@echo Beautify XML file format
@echo.
@echo Usage: SAPretty.bat "XML_FILE" 
@echo.
goto end

:all
@ant -q -f %WORKSPACE%\Tools\Source\SurfaceArea\build.xml pretty -DSURFACE_AREA_FILE=%~f1
@goto end

:no_jdk
  @echo.
  @echo !!! Please set JAVA_HOME !!!
  @echo.
  @goto end

:no_wks
  @echo.
  @echo !!! Please set WORKSPACE !!!
  @echo.
  @goto end

:end

