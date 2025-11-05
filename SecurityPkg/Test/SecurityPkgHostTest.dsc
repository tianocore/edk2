## @file
# SecurityPkg DSC file used to build host-based unit tests.
#
# Copyright (C) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = SecurityPkgHostTest
  PLATFORM_GUID           = 9D78A9B4-00CD-477E-A5BF-90CC793EEFB0
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/SecurityPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf

[Components]
  SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockUefiRuntimeServicesTableLib.inf
  SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockPlatformPKProtectionLib.inf
  SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockUefiLib.inf
  SecurityPkg/Test/Mock/Library/GoogleTest/MockPlatformPKProtectionLib/MockPlatformPKProtectionLib.inf
  SecurityPkg/Library/DxeTpm2MeasureBootLib/InternalUnitTest/DxeTpm2MeasureBootLibSanitizationTestHost.inf
  SecurityPkg/Library/DxeTpmMeasureBootLib/InternalUnitTest/DxeTpmMeasureBootLibSanitizationTestHost.inf

  #
  # Build SecurityPkg HOST_APPLICATION Tests
  #
  SecurityPkg/Library/SecureBootVariableLib/UnitTest/SecureBootVariableLibUnitTest.inf {
    <LibraryClasses>
      SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
      UefiRuntimeServicesTableLib|SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockUefiRuntimeServicesTableLib.inf
      PlatformPKProtectionLib|SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockPlatformPKProtectionLib.inf
      UefiLib|SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockUefiLib.inf
  }
  SecurityPkg/Library/SecureBootVariableLib/GoogleTest/SecureBootVariableLibGoogleTest.inf {
    <LibraryClasses>
      SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
      UefiRuntimeServicesTableLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
      PlatformPKProtectionLib|SecurityPkg/Test/Mock/Library/GoogleTest/MockPlatformPKProtectionLib/MockPlatformPKProtectionLib.inf
      UefiLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiLib/MockUefiLib.inf
  }
  SecurityPkg/Library/DxeImageVerificationLib/GoogleTest/DxeImageVerificationLibGoogleTest.inf {
    <LibraryClasses>
      UefiRuntimeServicesTableLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
      UefiBootServicesTableLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiBootServicesTableLib/MockUefiBootServicesTableLib.inf
      DxeImageVerificationLib|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/UnitTestHostBaseCryptLib.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
      UefiLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiLib/MockUefiLib.inf
      DevicePathLib|MdePkg/Test/Mock/Library/GoogleTest/MockDevicePathLib/MockDevicePathLib.inf
      SecurityManagementLib|MdeModulePkg/Test/Mock/Library/GoogleTest/MockSecurityManagementLib/MockSecurityManagementLib.inf
      PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
      PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
      TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  }

[PcdsPatchableInModule]
  gEfiSecurityPkgTokenSpaceGuid.PcdOptionRomImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdRemovableMediaImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdFixedMediaImageVerificationPolicy|0x04
