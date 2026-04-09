## @file
# PrmPkg DSC file used to build host-based unit tests.
#
# Copyright (c) Microsoft Corporation.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = PrmPkgHostTest
  PLATFORM_GUID           = 5BCCFC54-2162-42FB-ABCA-5B8D0DC4013C
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/$(PLATFORM_NAME)/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PrmContextBufferLib|PrmPkg/Library/DxePrmContextBufferLib/DxePrmContextBufferLib.inf
  PrmModuleDiscoveryLib|PrmPkg/Library/DxePrmModuleDiscoveryLib/DxePrmModuleDiscoveryLib.inf
  PrmPeCoffLib|PrmPkg/Library/DxePrmPeCoffLib/DxePrmPeCoffLib.inf

[Components]
  #
  # Unit test host applications
  #
  PrmPkg/Library/DxePrmContextBufferLib/UnitTest/DxePrmContextBufferLibUnitTestHost.inf
  PrmPkg/Library/DxePrmModuleDiscoveryLib/UnitTest/DxePrmModuleDiscoveryLibUnitTestHost.inf
