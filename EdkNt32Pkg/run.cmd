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

:check_fd
if exist %WORKSPACE%\EdkNt32Pkg\build\Debug\Fv\Fv_Recovery.fd goto start_secmain

:create_fd
if not exist Build\Debug\Fv mkdir Build\Debug\Fv
copy Build\Debug\Msft\Fv\FV_RECOVERY.fv /B + Build\Debug\Msft\Fv\NV_STORAGE.fv /B Build\Debug\Fv\Fv_Recovery.fd /B

:start_secmain
pushd .
cd Build\DEBUG\MSFT\IA32
SecMain.exe
popd
@echo on

