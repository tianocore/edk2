## @file
# UnitTestFrameworkPkg DSC include file for host and target based test DSC
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[LibraryClasses]
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  UnitTestPersistenceLib|UnitTestFrameworkPkg/Library/UnitTestPersistenceLibNull/UnitTestPersistenceLibNull.inf
  UnitTestResultReportLib|UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibDebugLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x17

[BuildOptions]
  MSFT:*_*_*_CC_FLAGS  = -D DISABLE_NEW_DEPRECATED_INTERFACES -D EDKII_UNIT_TEST_FRAMEWORK_ENABLED
  GCC:*_*_*_CC_FLAGS   = -D DISABLE_NEW_DEPRECATED_INTERFACES -D EDKII_UNIT_TEST_FRAMEWORK_ENABLED
  XCODE:*_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES -D EDKII_UNIT_TEST_FRAMEWORK_ENABLED
