##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

#
# Memory Protection Libraries
#
[LibraryClasses.common]
  SetMemoryProtectionsLib|MdeModulePkg/Library/SetMemoryProtectionsLib/SetMemoryProtectionsLib.inf

[LibraryClasses.common.SMM_CORE, LibraryClasses.common.DXE_SMM_DRIVER, LibraryClasses.common.MM_CORE_STANDALONE, LibraryClasses.common.MM_STANDALONE]
  GetMemoryProtectionsLib|MdeModulePkg/Library/GetMemoryProtectionsLib/MmGetMemoryProtectionsLib.inf

[LibraryClasses.common.DXE_CORE, LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_APPLICATION, LibraryClasses.common.UEFI_DRIVER]
  GetMemoryProtectionsLib|MdeModulePkg/Library/GetMemoryProtectionsLib/DxeGetMemoryProtectionsLib.inf
