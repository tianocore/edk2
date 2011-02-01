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

@REM Set for tools chain. Currently RVCT31
SET TARGET_TOOLS=RVCT31
SET TARGET=DEBUG

@if /I "%1"=="RELEASE" (
  @REM If 1st argument is release set TARGET to RELEASE and shift arguments to remove it 
  SET TARGET=RELEASE
  shift /1
)

SET BUILD_ROOT=%WORKSPACE%\Build\ArmRealViewEb\%TARGET%_%TARGET_TOOLS%

@REM Build the ARM RealView EB firmware and creat an FD (FLASH Device) Image.
CALL build -p ArmRealViewEbPkg\ArmRealViewEbPkg.dsc -a ARM -t RVCT31 -b %TARGET% %1 %2 %3 %4 %5 %6 %7 %8
@if ERRORLEVEL 1 goto Exit

@if /I "%1"=="CLEAN" goto Clean

:Exit
EXIT /B

:Clean
