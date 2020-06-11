## @file
# UnitTestFrameworkPkg
#
# Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME           = UnitTestFrameworkPkg
  PLATFORM_GUID           = 7420CC7E-334E-4EFF-B974-A39613455168
  PLATFORM_VERSION        = 1.00
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/UnitTestFrameworkPkg
  SUPPORTED_ARCHITECTURES = IA32|X64|ARM|AARCH64|RISCV64
  BUILD_TARGETS           = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgTarget.dsc.inc

[PcdsPatchableInModule]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x17

[Components]
  UnitTestFrameworkPkg/Library/UnitTestLib/UnitTestLib.inf
  UnitTestFrameworkPkg/Library/UnitTestPersistenceLibNull/UnitTestPersistenceLibNull.inf
  UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibDebugLib.inf
  UnitTestFrameworkPkg/Library/UnitTestBootLibNull/UnitTestBootLibNull.inf
  UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibConOut.inf
  UnitTestFrameworkPkg/Library/UnitTestBootLibUsbClass/UnitTestBootLibUsbClass.inf
  UnitTestFrameworkPkg/Library/UnitTestPersistenceLibSimpleFileSystem/UnitTestPersistenceLibSimpleFileSystem.inf
  UnitTestFrameworkPkg/Library/UnitTestDebugAssertLib/UnitTestDebugAssertLib.inf

  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTest/SampleUnitTestDxe.inf
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTest/SampleUnitTestPei.inf
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTest/SampleUnitTestSmm.inf
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTest/SampleUnitTestUefiShell.inf
