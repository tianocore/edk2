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
@REM set WORKSPACE=C:\mdk


@REM ##############################################################
@REM # You should not have to modify anything below this line
@REM #

@echo off

:check_vc
if defined VCINSTALLDIR goto check_java
if defined VS71COMNTOOLS (
  call "%VS71COMNTOOLS%vsvars32.bat"
) else (
  echo.
  echo !!! WARNING !!!! Cannot find Visual Studio !!!
  echo.
)

:check_java
if "%JAVA_HOME%"=="" goto no_jdk
echo.
echo JAVA_HOME:     %JAVA_HOME%

@REM Set the WORKSPACE to the Current Working Directory
set WORKSPACE=%~dp0
echo WORKSPACE:     %WORKSPACE%

:set_cygwin
if not defined CYGWIN_HOME (
  if exist c:\cygwin (set CYGWIN_HOME=c:\cygwin) else (
    echo.
    echo !!! Not set CYGWIN_HOME, gcc build may not be used !!!
    echo.
  )
) else (
  echo CYGWIN_HOME:   %CYGWIN_HOME%
)

if "%ANT_HOME%"=="" goto no_ant
echo ANT_HOME:      %ANT_HOME%
if not exist %ANT_HOME%\lib\ant-contrib.jar goto no_antcontrib

if "%XMLBEANS_HOME%"=="" goto no_xmlbeans
echo XMLBEANS_HOME: %XMLBEANS_HOME%

set Framework_Tools_Path=%WORKSPACE%\Tools\bin


if "%PATHBACKUP%"=="" set PATHBACKUP=%PATH%
set PATH=%JAVA_HOME%\bin;%ANT_HOME%\bin;%WORKSPACE%\Tools\bin;%XMLBEANS_HOME%\bin;%PATHBACKUP%;%CYGWIN_HOME%\bin

echo PATH:      %PATH%
echo.

if not exist %XMLBEANS_HOME%\lib\saxon8.jar goto no_saxon8

echo Building the Tiano Tools

@REM We are going to create the SurfaceArea.jar file first so that other Java Program can use it
set CLASSPATH=%XMLBEANS_HOME%\lib\jsr173_1.0_api.jar;%XMLBEANS_HOME%\lib\xbean.jar;%XMLBEANS_HOME%\lib\xbean_xpath.jar;%XMLBEANS_HOME%\lib\xmlpublic.jar;%XMLBEANS_HOME%\lib\saxon8.jar

call ant -f %WORKSPACE%Tools\build.xml SurfaceArea

@REM Now we can make the other Java Programs
set CLASSPATH=%WORKSPACE%Tools\Jars\SurfaceArea.jar;%XMLBEANS_HOME%\lib\jsr173_1.0_api.jar;%XMLBEANS_HOME%\lib\xbean.jar;%XMLBEANS_HOME%\lib\xbean_xpath.jar;%XMLBEANS_HOME%\lib\xmlpublic.jar;%XMLBEANS_HOME%\lib\saxon8.jar

call ant -f %WORKSPACE%Tools\build.xml JavaCode

@REM We have all of the Java Programs and add-in classes created, so we can start using the cpp-tasks to create our tools
set CLASSPATH=%WORKSPACE%Tools\Jars\SurfaceArea.jar;%WORKSPACE%Tools\Jars\GenBuild.jar;%WORKSPACE%Tools\Jars\cpptasks.jar;%WORKSPACE%Tools\Jars\frameworktasks.jar;%XMLBEANS_HOME%\lib\jsr173_1.0_api.jar;%XMLBEANS_HOME%\lib\xbean.jar;%XMLBEANS_HOME%\lib\xbean_xpath.jar;%XMLBEANS_HOME%\lib\xmlpublic.jar;%XMLBEANS_HOME%\lib\saxon8.jar

call ant -f %WORKSPACE%Tools\build.xml C_Code

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

:end
@echo on

