@echo off
setlocal
SET NMAKE_COMMAND=%1
SHIFT

:loop
if "%1"=="" goto success

ECHO Building %1
pushd %1
nmake %NMAKE_COMMAND%
if ERRORLEVEL 1 goto error
ECHO %1 built successfully (%NMAKE_COMMAND%)
ECHO.
shift
popd
goto loop

:success
goto exit

:error
popd
ENDLOCAL
ECHO Error while making %1!
VERIFY OTHER 2>NUL

:exit
