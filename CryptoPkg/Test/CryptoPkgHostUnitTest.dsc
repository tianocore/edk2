## @file
# CryptoPkg DSC file used to build host-based unit tests.
#
# Copyright (c) Microsoft Corporation.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = CryptoPkgHostTest
  PLATFORM_GUID           = C7F97D6D-54AC-45A9-8197-CC99B20CC7EC
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/CryptoPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/UnitTestHostBaseCryptLib.inf

[LibraryClasses.AARCH64, LibraryClasses.ARM]
  RngLib|MdePkg/Library/BaseRngLibNull/BaseRngLibNull.inf

[LibraryClasses.X64, LibraryClasses.IA32]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[Components]
  #
  # Build HOST_APPLICATION that tests the SampleUnitTest
  #
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibHost.inf

[BuildOptions]
  *_*_*_CC_FLAGS       = -D DISABLE_NEW_DEPRECATED_INTERFACES
  MSFT:*_*_*_CC_FLAGS  = /D ENABLE_MD5_DEPRECATED_INTERFACES
  INTEL:*_*_*_CC_FLAGS = /D ENABLE_MD5_DEPRECATED_INTERFACES
  GCC:*_*_*_CC_FLAGS   = -D ENABLE_MD5_DEPRECATED_INTERFACES
  RVCT:*_*_*_CC_FLAGS  = -DENABLE_MD5_DEPRECATED_INTERFACES
