@REM @file
@REM   Windows batch file to generate UEFI capsules for system firmware and
@REM   firmware for sample devices
@REM
@REM Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
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
call GenCapsuleSampleColor.bat Blue  149da854-7d19-4faa-a91e-862ea1324be6
call GenCapsuleSampleColor.bat Green 79179bfd-704d-4c90-9e02-0ab8d968c18a
call GenCapsuleSampleColor.bat Red   72e2945a-00da-448e-9aa7-075ad840f9d4

call LvfsGenCapsuleMinnowMax.bat
call LvfsGenCapsuleMinnowMaxRelease.bat
call LvfsGenCapsuleSampleColor.bat Blue  149da854-7d19-4faa-a91e-862ea1324be6
call LvfsGenCapsuleSampleColor.bat Green 79179bfd-704d-4c90-9e02-0ab8d968c18a
call LvfsGenCapsuleSampleColor.bat Red   72e2945a-00da-448e-9aa7-075ad840f9d4
