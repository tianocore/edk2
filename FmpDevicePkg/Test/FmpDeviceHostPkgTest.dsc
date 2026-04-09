## @file
# FmpDevicePkg DSC file used to build host-based unit tests.
#
# Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = FmpDevicePkgHostTest
  PLATFORM_GUID           = 80B12646-9A30-4516-AABF-DE1919B573E3
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/FmpDevicePkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  FmpDependencyLib|FmpDevicePkg/Library/FmpDependencyLib/FmpDependencyLib.inf

[Components]
  #
  # Build HOST_APPLICATION that tests the FmpDependencyLib
  #
  FmpDevicePkg/Test/UnitTest/Library/FmpDependencyLib/FmpDependencyLibUnitTestsHost.inf
