@echo off
REM libprep.bat
REM
REM SYNTAX: libprep <path-to-new-lib-dir>
REM
SETLOCAL

set dest=%1

echo Copying files to %dest%.
echo Existing files will be overwritten.
echo.
PAUSE

REM Copy Distro then PyMod files to the destination
XCOPY Lib %dest% /S /I /Y
XCOPY PyMod-2.7.10\Lib %dest% /S /I /Y

echo DONE
ENDLOCAL
