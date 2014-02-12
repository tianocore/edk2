@REM Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
@REM This program and the accompanying materials
@REM are licensed and made available under the terms and conditions of the BSD License
@REM which accompanies this distribution.  The full text of the license may be found at
@REM http://opensource.org/licenses/bsd-license.php
@REM
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM

@REM Example usage of this script. default is a DEBUG build
@REM b
@REM b clean
@REM b release 
@REM b release clean
@REM b -v -y build.log

ECHO OFF
@REM Setup Build environment. Sets WORKSPACE and puts build in path
CALL ..\edksetup.bat

@REM Set for tools chain. Currently ARMGCC
SET TARGET_TOOLS=ARMGCC
SET TARGET=DEBUG

@if /I "%1"=="RELEASE" (
  @REM If 1st argument is release set TARGET to RELEASE and shift arguments to remove it 
  SET TARGET=RELEASE
  shift /1
)

SET BUILD_ROOT=%WORKSPACE%\Build\BeagleBoard\%TARGET%_%TARGET_TOOLS%

@REM Build the Beagle Board firmware and creat an FD (FLASH Device) Image.
CALL build -p BeagleBoardPkg\BeagleBoardPkg.dsc -a ARM -t %TARGET_TOOLS% -b %TARGET% %1 %2 %3 %4 %5 %6 %7 %8
@if ERRORLEVEL 1 goto Exit

@if /I "%1"=="CLEAN" goto Clean

@REM
@REM Ram starts at 0x80000000
@REM OMAP 3530 TRM defines 0x80008208 as the entry point
@REM The reset vector is caught by the mask ROM in the OMAP 3530 so that is why this entry 
@REM point looks so strange. 
@REM OMAP 3430 TRM section 26.4.8 has Image header information. (missing in OMAP 3530 TRM)
@REM
@cd Tools

ECHO Building tools...
CALL nmake 

ECHO Patching image with ConfigurationHeader.dat
CALL GenerateImage -D ..\ConfigurationHeader.dat -E 0x80008208 -I %BUILD_ROOT%\FV\BEAGLEBOARD_EFI.fd -O %BUILD_ROOT%\FV\BeagleBoard_EFI_flashboot.fd

ECHO Patching ..\Debugger_scripts ...
SET DEBUGGER_SCRIPT=..\Debugger_scripts
@for /f %%a IN ('dir /b %DEBUGGER_SCRIPT%\*.inc %DEBUGGER_SCRIPT%\*.cmm') do (
  @CALL replace %DEBUGGER_SCRIPT%\%%a %BUILD_ROOT%\%%a ZZZZZZ %BUILD_ROOT% WWWWWW  %WORKSPACE%
)

cd ..
:Exit
EXIT /B

:Clean
cd Tools
CALL nmake clean
cd ..
