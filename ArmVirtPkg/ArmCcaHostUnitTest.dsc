## @file
#  Arm CCA Host based unit test DSC file.
#
#  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME           = ArmCcaHostTest
  PLATFORM_GUID           = 94e5bb2a-4cfd-4bee-83bd-b6a10d2dd93e
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x0001001B
  OUTPUT_DIRECTORY        = Build/ArmVirtPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64|AARCH64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT


  UNIT_TESTING_CODE_COVERAGE_ENABLE     = FALSE
  UNIT_TESTING_ADDRESS_SANITIZER_ENABLE = FALSE

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/UnitTestHostBaseCryptLib.inf
  ArmCcaBootSyncCryptoLib|ArmVirtPkg/Library/ArmCcaBootSyncCryptoLib/ArmCcaBootSyncCryptoLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[LibraryClasses.X64, LibraryClasses.IA32]
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[LibraryClasses.AARCH64.HOST_APPLICATION]
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  RngLib|UnitTestFrameworkPkg/Library/UnitTestHostRngLib/UnitTestHostRngLib.inf

[Components]
  # libqcbor is used for parsing an Arm CCA Atestation Token.
  ArmVirtPkg/ArmCcaBootSync/QcborLib/QcborLib.inf

  #
  # Build HOST_APPLICATION that tests the User Context side
  # of the Boot Sync Blocks Protocol.
  #
  ArmVirtPkg/ArmCcaBootSync/TestUserContextService.inf

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES

[BuildOptions.common.EDKII.HOST_APPLICATION]
# The following linker option is required when linking libqcbor with the
# HostTestService.
# The DLINK2_FLAGS are reset in the by the unit test framework dsc.inc
# file UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc
# Furthermore, the Linker options when passed as the module options in
# HostTestService.inc get overridden, see ApplyBuildOption(), line 625
# in file BaseTools/Source/Python/AutoGen/ModuleAutoGenHelper.py
# Therefore, specify the options as the module type (HOST_APPLICATION)
# option in the DSC file.
#
  GCC:*_*_*_DLINK2_FLAGS = -lqcbor -L$(BIN_DIR)/$(MODULE_RELATIVE_DIR)/QcborLib/QcborLib/OUTPUT

