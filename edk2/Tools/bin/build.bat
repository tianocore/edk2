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

@REM @if "%1"==""         (goto build)
@REM @if "%1"=="clean"    (goto clean)
@REM @if "%1"=="cleanall" (goto cleanall)

:build
	@echo on
	ant -f %WORKSPACE%/build.xml %1 %2 %3
	@echo off
	@goto end

:clean
	@echo on
	ant clean -f %WORKSPACE%/build.xml
	@echo off
	@goto end
	
:cleanall
	@echo on
	ant cleanall -f %WORKSPACE%/build.xml
	@echo off
	@goto end

:end