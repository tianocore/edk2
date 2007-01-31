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

:check_java
if "%JAVA_HOME%"=="" goto no_jdk
:check_wks
if "%WORKSPACE%"=="" goto no_wks
:check_ant
if "%ANT_HOME%"=="" goto no_ant
:check_xmlbeans
if "%XMLBEANS_HOME%"=="" goto no_xmlbeans
:check_surfacearea
if not exist %WORKSPACE%\Tools\Jars\SurfaceArea.jar (
  goto no_surfacearea
)
:check_frameworkwizard
if not exist %WORKSPACE%\Tools\bin\FrameworkWizard.jar (
  goto no_frameworkwizard
)

@REM Run Framework Wizard
call "java" -Xmx256m org.tianocore.frameworkwizard.FrameworkWizardUI %%1

goto end

:no_jdk
@echo.
@echo !!! Please set JAVA_HOME !!!
@echo.
goto check_wks

:no_wks
@echo.
@echo !!! Please set WORKSPACE !!!
@echo.
goto check_ant

:no_ant
@echo.
@echo !!! Please set ANT_HOME !!!
@echo.
goto check_xmlbeans

:no_xmlbeans
@echo.
@echo !!! Please set XMLBEANS_HOME !!!
@echo.
goto end

:no_surfacearea
@echo.
@echo !!! Please run edksetup.bat to build SurfaceArea.jar !!!
@echo.
goto end

:no_frameworkwizard
@echo.
@echo !!! Please run edksetup.bat to build FrameworkWizard.jar !!!
@echo.
goto end

:end
@echo on

