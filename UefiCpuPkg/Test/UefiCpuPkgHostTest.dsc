## @file
# UefiCpuPkg DSC file used to build host-based unit tests.
#
# Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = UefiCpuPkgHostTest
  PLATFORM_GUID           = E00B9599-5B74-4FF7-AB9F-8183FB13B2F9
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/UefiCpuPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  CpuPageTableLib|UefiCpuPkg/Library/CpuPageTableLib/CpuPageTableLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/UnitTestHostBaseCryptLib.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[PcdsPatchableInModule]
  gUefiCpuPkgTokenSpaceGuid.PcdCpuNumberOfReservedVariableMtrrs|0

[Components]
  #
  # Build HOST_APPLICATION that tests the MtrrLib
  #
  UefiCpuPkg/Library/MtrrLib/UnitTest/MtrrLibUnitTestHost.inf

  #
  # Build HOST_APPLICATION that tests the CpuPageTableLib
  #
  UefiCpuPkg/Library/CpuPageTableLib/UnitTest/CpuPageTableLibUnitTestHost.inf
