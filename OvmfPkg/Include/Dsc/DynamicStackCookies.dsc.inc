## @file DynamicStackCookies.dsc.inc
# DSC inc to include dynamic stack cookies
# in an OVMF platform DSC.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  DEFINE CUSTOM_STACK_CHECK_LIB  = TRUE

[LibraryClasses.common.SEC, LibraryClasses.common.PEI_CORE]
  # don't check stack cookies in SEC and PEI_CORE because we don't have exception handlers installed at this point and
  # will triple fault and reboot if we hit a stack cookie failure
  NULL|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf

[LibraryClasses.common.PEIM]
  # because some PEIMs run pre-memory, globally have all PEIMs use the static stack cookie
  NULL|MdePkg/Library/StackCheckLib/StackCheckLib.inf

[LibraryClasses.common.DXE_CORE, LibraryClasses.common.SMM_CORE, LibraryClasses.common.DXE_SMM_DRIVER, LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  # for all other types, use the dynamic stack cookie
  NULL|MdePkg/Library/StackCheckLib/StackCheckLib.inf
  StackCheckEntryPointLib|MdePkg/Library/StackCheckLib/StackCheckLibDynamicInit.inf
