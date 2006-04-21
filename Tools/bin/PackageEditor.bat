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

set ANT_HOME=%WORKSPACE%\Tools\bin\apache-ant
set XMLBEANS_HOME=%WORKSPACE%\Tools\bin\xmlbeans
set Framework_Tools_Path=%WORKSPACE%\Tools\bin

set PATH=%JAVA_HOME%\bin;%ANT_HOME%\bin;%WORKSPACE%\Tools\bin;%XMLBEANS_HOME%\bin;%PATH%

set CLASSPATH=%WORKSPACE%\Tools\Jars\SurfaceArea.jar;%WORKSPACE%\Tools\Jars\GenBuild.jar;%WORKSPACE%\Tools\Jars\cpptasks.jar;%WORKSPACE%\Tools\Jars\frameworktasks.jar;%XMLBEANS_HOME%\lib\jsr173_1.0_api.jar;%XMLBEANS_HOME%\lib\xbean.jar;%XMLBEANS_HOME%\lib\xbean_xpath.jar;%XMLBEANS_HOME%\lib\xmlpublic.jar;%XMLBEANS_HOME%\lib\saxon8.jar;%XMLBEANS_HOME%\lib\saxon8-jdom.jar;%XMLBEANS_HOME%\lib\saxon8-sql.jar;%XMLBEANS_HOME%\lib\resolver.jar

call "ant" -f %WORKSPACE%\Tools\Source\PackageEditor\build.xml

call "java" -jar %WORKSPACE%\Tools\bin\PackageEditor.jar

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

