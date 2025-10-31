## @file
# UnitTestFrameworkPkg
#
# Copyright (c) 2019 - 2021, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
# Copyright (c) 2022, Loongson Technology Corporation Limited. All rights reserved.<BR>
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
  SUPPORTED_ARCHITECTURES = IA32|X64|AARCH64|RISCV64|LOONGARCH64
  BUILD_TARGETS           = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgTarget.dsc.inc
!include MdePkg/MdeLibs.dsc.inc

[PcdsPatchableInModule]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x1F

[Components]
  UnitTestFrameworkPkg/Library/UnitTestLib/UnitTestLib.inf
  UnitTestFrameworkPkg/Library/UnitTestPersistenceLibNull/UnitTestPersistenceLibNull.inf
  UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibDebugLib.inf
  UnitTestFrameworkPkg/Library/UnitTestBootLibNull/UnitTestBootLibNull.inf
  UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibConOut.inf
  UnitTestFrameworkPkg/Library/UnitTestBootLibUsbClass/UnitTestBootLibUsbClass.inf
  UnitTestFrameworkPkg/Library/UnitTestPersistenceLibSimpleFileSystem/UnitTestPersistenceLibSimpleFileSystem.inf
  UnitTestFrameworkPkg/Library/UnitTestDebugAssertLib/UnitTestDebugAssertLib.inf
  UnitTestFrameworkPkg/Library/UnitTestUefiBootServicesTableLib/UnitTestUefiBootServicesTableLib.inf
  UnitTestFrameworkPkg/Library/UnitTestPeiServicesTablePointerLib/UnitTestPeiServicesTablePointerLib.inf

  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTest/SampleUnitTestDxe.inf
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTest/SampleUnitTestPei.inf
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTest/SampleUnitTestSmm.inf
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTest/SampleUnitTestUefiShell.inf

  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTestExpectFail/SampleUnitTestDxeExpectFail.inf
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTestExpectFail/SampleUnitTestPeiExpectFail.inf
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTestExpectFail/SampleUnitTestSmmExpectFail.inf
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTestExpectFail/SampleUnitTestUefiShellExpectFail.inf

  #
  # Disable warning for divide by zero to pass build of unit tests
  # that generate a divide by zero exception.
  #
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTestGenerateException/SampleUnitTestDxeGenerateException.inf {
    <BuildOptions>
      MSFT:*_*_*_CC_FLAGS = /wd4723
  }
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTestGenerateException/SampleUnitTestPeiGenerateException.inf {
    <BuildOptions>
      MSFT:*_*_*_CC_FLAGS = /wd4723
  }
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTestGenerateException/SampleUnitTestSmmGenerateException.inf {
    <BuildOptions>
      MSFT:*_*_*_CC_FLAGS = /wd4723
  }
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTestGenerateException/SampleUnitTestUefiShellGenerateException.inf {
    <BuildOptions>
      MSFT:*_*_*_CC_FLAGS = /wd4723
  }
