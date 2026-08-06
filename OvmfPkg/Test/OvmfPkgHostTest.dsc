## @file
# OvmfPkg DSC file used to build host-based unit tests.
#
# Copyright (c) 2026, Canonical Ltd. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME           = OvmfPkgHostTest
  PLATFORM_GUID           = 7C2A5F3D-6B41-4E8A-9F0C-2D1B8E4A6C57
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/OvmfPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf

[Components]
  #
  # Build OvmfPkg HOST_APPLICATION Tests
  #
  OvmfPkg/QemuKernelLoaderFsDxe/UnitTest/QemuKernelLoaderFsDxeUnitTest.inf
