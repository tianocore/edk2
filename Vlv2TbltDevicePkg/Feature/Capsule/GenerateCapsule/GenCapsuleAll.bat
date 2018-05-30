@REM @file
@REM   Windows batch file to generate UEFI capsules for system firmware and
@REM   firmware for sample devices
@REM
@REM Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
@REM This program and the accompanying materials
@REM are licensed and made available under the terms and conditions of the BSD License
@REM which accompanies this distribution.  The full text of the license may be found at
@REM http://opensource.org/licenses/bsd-license.php
@REM
@REM THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
@REM WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
@REM

@echo off
setlocal
cd /d %~dp0

rmdir /s /q %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules
mkdir %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules
mkdir %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\SampleDevelopment
mkdir %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\NewCert
mkdir %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\TestCert
copy %WORKSPACE%\Build\Vlv2TbltDevicePkg\DEBUG_VS2015x86\X64\CapsuleApp.efi %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\SampleDevelopment\CapsuleApp.efi
copy %WORKSPACE%\Build\Vlv2TbltDevicePkg\RELEASE_VS2015x86\X64\CapsuleApp.efi %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\SampleDevelopment\CapsuleAppRelease.efi
copy %WORKSPACE%\Build\Vlv2TbltDevicePkg\DEBUG_VS2015x86\X64\CapsuleApp.efi %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\NewCert\CapsuleApp.efi
copy %WORKSPACE%\Build\Vlv2TbltDevicePkg\RELEASE_VS2015x86\X64\CapsuleApp.efi %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\NewCert\CapsuleAppRelease.efi
copy %WORKSPACE%\Build\Vlv2TbltDevicePkg\DEBUG_VS2015x86\X64\CapsuleApp.efi %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\TestCert\CapsuleApp.efi
copy %WORKSPACE%\Build\Vlv2TbltDevicePkg\RELEASE_VS2015x86\X64\CapsuleApp.efi %WORKSPACE%\Build\Vlv2TbltDevicePkg\Capsules\TestCert\CapsuleAppRelease.efi

call GenCapsuleMinnowMax.bat
call GenCapsuleMinnowMaxRelease.bat
call GenCapsuleSampleColor.bat Blue  149DA854-7D19-4FAA-A91E-862EA1324BE6
call GenCapsuleSampleColor.bat Green 79179BFD-704D-4C90-9E02-0AB8D968C18A
call GenCapsuleSampleColor.bat Red   72E2945A-00DA-448E-9AA7-075AD840F9D4
