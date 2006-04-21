@REM set following environment in this file or in command shell
@REM set JAVA_HOME="C:\Program Files\Java\jdk1.5.0_04"
@REM set WORKSPACE=C:\MyWorkspace


@REM ##############################################################
@REM # You should not have to modify anything below this line
@REM #

@echo off

:check_java
if "%JAVA_HOME%"=="" goto no_jdk
:check_wks
if "%WORKSPACE%"=="" goto no_wks

set ANT_HOME=%WORKSPACE%\Tools\bin\apache-ant
set PATH=%JAVA_HOME%\bin;%ANT_HOME%\bin;%WORKSPACE%\Tools\bin;%XMLBEANS_HOME%\bin;%PATH%

call "ant" -f %WORKSPACE%\Tools\Source\Setup\build.xml

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

