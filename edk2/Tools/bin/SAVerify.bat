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

@if "%1"=="" (goto usage)
@if "%1"=="-h" (goto usage)
@if "%1"=="-H" (goto usage)
@if "%1"=="all" (goto all)
@if "%1"=="ALL" (goto all) else (goto standalone)

:usage
@echo off
@echo.
@echo Verify SurfaceArea file(s)
@echo.
@echo Usage: 
@echo        SAVerify.bat "MSA_FILE"  - Verify SurfaceArea file named by MSA_FILE
@echo        SAVerify.bat all         - Verify all SurfaceArea files under current directory and its sub-directories
@echo        SAVerify.bat -h          - Print usage message
@echo.
goto end

:all
  @echo off
  @for /R %%a in (*.msa *.fpd *.spd) do @ant -q -f %WORKSPACE%\Tools\Java\Source\SurfaceArea\build.xml validate -DSURFACE_AREA_FILE=%%a
  @echo on
  @goto end

:standalone
  @ant -q -f %WORKSPACE%\Tools\Java\Source\SurfaceArea\build.xml validate -DSURFACE_AREA_FILE=%~f1
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
