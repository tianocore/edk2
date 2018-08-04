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
