## @file
# Mde DSC include file for [LibraryClasses*] section of all Architectures.
#
# This file can be included to the [LibraryClasses*] section(s) of a platform DSC file
# by using "!include MdePkg/MdeLibs.dsc.inc" to specify the library instances
# of some EDKII basic/common library classes.
#
# Copyright (c) 2021 - 2024, Intel Corporation. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[LibraryClasses]
  ArmTrngLib|MdePkg/Library/BaseArmTrngLibNull/BaseArmTrngLibNull.inf
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  SmmCpuRendezvousLib|MdePkg/Library/SmmCpuRendezvousLibNull/SmmCpuRendezvousLibNull.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  MmUnblockMemoryLib|MdePkg/Library/MmUnblockMemoryLib/MmUnblockMemoryLibNull.inf

# Stack Cookies cannot be generically applied to SEC modules because they may not define _ModuleEntryPoint and when we
# link a library in, we have to be able to define the entry point. SEC modules that do define _ModuleEntryPoint can
# apply a library class override to get StackCheckLibNull.inf
[LibraryClasses.common.PEI_CORE, LibraryClasses.common.PEIM, LibraryClasses.common.DXE_CORE, LibraryClasses.common.SMM_CORE, LibraryClasses.common.MM_CORE_STANDALONE, LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.DXE_SMM_DRIVER, LibraryClasses.common.MM_STANDALONE, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  NULL|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf
