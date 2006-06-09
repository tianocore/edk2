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

@REM set following environment in this file or in command shell
@REM set JAVA_HOME=C:\Java\jdk1.5.0_04
@REM set ANT_HOME=C:\ANT
@REM set XMLBEANS_HOME=C:\xmlbeans
@REM set CYGWIN_HOME=C:\cygwin

@REM usage: edksetup.bat [skip]
@REM if the argument, skip is present, only the paths and the
@REM test and set of environment settings are performed. 

@REM ##############################################################
@REM # You should not have to modify anything below this line
@REM #

@echo off

@REM
@REM Check the required system environment variables
@REM

:check_vc
if defined VCINSTALLDIR goto check_cygwin
if defined VS71COMNTOOLS (
  call "%VS71COMNTOOLS%vsvars32.bat"
) else (
  echo.
  echo !!! WARNING !!!! Cannot find Visual Studio !!!
  echo.
)

:check_cygwin
if defined CYGWIN_HOME goto check_java
if exist c:\cygwin (
  set CYGWIN_HOME=c:\cygwin
) else (
  echo.
  echo !!! WARNING !!!! Not set CYGWIN_HOME, gcc build may not be used !!!
  echo.
)

:check_java
if "%JAVA_HOME%"=="" goto no_jdk

:check_ant
if "%ANT_HOME%"=="" goto no_ant
if not exist %ANT_HOME%\lib\ant-contrib.jar goto no_antcontrib

:check_xmlbeans
if "%XMLBEANS_HOME%"=="" goto no_xmlbeans
if not exist %XMLBEANS_HOME%\lib\saxon8.jar goto no_saxon8

@REM
@REM Set the WORKSPACE to the current working directory
@REM
set WORKSPACE=%CD%

set FRAMEWORK_TOOLS_PATH=%WORKSPACE%\Tools\bin

if defined WORKSPACE_TOOLS_PATH goto check_path
set PATH=%FRAMEWORK_TOOLS_PATH%;%JAVA_HOME%\bin;%ANT_HOME%\bin;%XMLBEANS_HOME%\bin;%PATH%
set WORKSPACE_TOOLS_PATH=%FRAMEWORK_TOOLS_PATH%
echo Setting the PATH variable to include the Framework_Tools_Path for this WORKSPACE
goto path_ok

:check_path
if "%FRAMEWORK_TOOLS_PATH%"=="%WORKSPACE_TOOLS_PATH%" goto path_ok
set PATH=%FRAMEWORK_TOOLS_PATH%;%PATH%
set WORKSPACE_PATH=%WORKSPACE%
echo Resetting the PATH variable to include the Framework_Tools_Path for this WORKSPACE

:path_ok

if "%1"=="skip" goto skipbuild

echo.
echo WORKSPACE:     %WORKSPACE%
echo JAVA_HOME:     %JAVA_HOME%
echo ANT_HOME:      %ANT_HOME%
echo XMLBEANS_HOME: %XMLBEANS_HOME%
echo CYGWIN_HOME:   %CYGWIN_HOME%
echo PATH:          %PATH%
echo.

@REM
@REM Start to build the Framework Tools
@REM


echo.
echo Building the Framework Tools
echo.

@REM
@REM We are going to create the SurfaceArea.jar file first so that the other
@REM Java Programs can use it.
@REM It needs the XMLBEANS libraries in order to compile.
@REM
set CLASSPATH=%XMLBEANS_HOME%\lib\jsr173_1.0_api.jar;%XMLBEANS_HOME%\lib\xbean.jar
set CLASSPATH=%CLASSPATH%;%XMLBEANS_HOME%\lib\xbean_xpath.jar;%XMLBEANS_HOME%\lib\xmlpublic.jar
set CLASSPATH=%CLASSPATH%;%XMLBEANS_HOME%\lib\saxon8.jar;%XMLBEANS_HOME%\lib\resolver.jar

call ant -f %WORKSPACE%\Tools\build.xml SurfaceArea

@REM
@REM Now we can make the other Java Programs
@REM All of the remaining Java Programs require the SurfaceArea library to compile
@REM
set CLASSPATH=%CLASSPATH%;%WORKSPACE%\%Tools\Jars\SurfaceArea.jar

call ant -f %WORKSPACE%\Tools\build.xml JavaCode

@REM
@REM We have all of the Java Programs and add-in classes created, so we can start
@REM using the cpp-tasks to create our tools
@REM
set CLASSPATH=%CLASSPATH%;%WORKSPACE%\Tools\Jars\GenBuild.jar
set CLASSPATH=%CLASSPATH%;%WORKSPACE%\Tools\Jars\cpptasks.jar;%WORKSPACE%\Tools\Jars\frameworktasks.jar

call ant -f %WORKSPACE%\Tools\build.xml C_Code

@REM
@REM Done!!!
@REM
goto end

:no_jdk
echo.
echo !!! Please install Java, and set JAVA_HOME !!!
echo.
goto end

:no_ant
echo.
echo !!! Please install Apache Ant, and set ANT_HOME !!!
echo.
goto end

:no_antcontrib
echo.
echo !!! Please install Ant-contrib to ANT_HOME !!!
echo.
goto end

:no_xmlbeans
echo.
echo !!! Please install XML Beans, and set XMLBEANS_HOME !!!
echo.
goto end

:no_saxon8
echo.
echo !!! Please copy saxon8.jar file to XMLBEANS_HOME\lib !!!
echo.
goto end

:skipbuild
@REM
@REM This just sets up the CLASSPATH, the rest of the environment should have been set already.
@REM
echo.
echo WORKSPACE:     %WORKSPACE%
echo JAVA_HOME:     %JAVA_HOME%
echo ANT_HOME:      %ANT_HOME%
echo XMLBEANS_HOME: %XMLBEANS_HOME%
echo CYGWIN_HOME:   %CYGWIN_HOME%
echo PATH:          %PATH%
echo.
set CLASSPATH=%XMLBEANS_HOME%\lib\jsr173_1.0_api.jar;%XMLBEANS_HOME%\lib\xbean.jar
set CLASSPATH=%CLASSPATH%;%XMLBEANS_HOME%\lib\xbean_xpath.jar;%XMLBEANS_HOME%\lib\xmlpublic.jar
set CLASSPATH=%CLASSPATH%;%XMLBEANS_HOME%\lib\saxon8.jar;%XMLBEANS_HOME%\lib\resolver.jar
set CLASSPATH=%CLASSPATH%;%WORKSPACE%\Tools\Jars\SurfaceArea.jar;%WORKSPACE%\Tools\Jars\GenBuild.jar
set CLASSPATH=%CLASSPATH%;%WORKSPACE%\Tools\Jars\cpptasks.jar;%WORKSPACE%\Tools\Jars\frameworktasks.jar
goto end

:end
@echo on

