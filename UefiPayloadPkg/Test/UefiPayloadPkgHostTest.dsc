## @file
# UefiPayloadPkg host-based unit tests.
#
# Copyright (c) 2026, Star Labs Systems. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME           = UefiPayloadPkgHostTest
  PLATFORM_GUID           = 890D9F9C-1D24-45FE-B88B-8E3D06CF5795
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/UefiPayloadPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[Components]
  UefiPayloadPkg/Library/FmpDeviceSmmLib/FmpDeviceSmmLibUnitTestHost.inf
