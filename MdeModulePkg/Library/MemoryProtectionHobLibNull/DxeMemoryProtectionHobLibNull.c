/** @file
Library defines the gDxeMps global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DxeMemoryProtectionHobLib.h>

// According to the C Specification, a global variable
// which is uninitialized will be zero. The net effect
// is memory protections will be OFF.
DXE_MEMORY_PROTECTION_SETTINGS  gDxeMps;

/**
  Gets the input EFI_MEMORY_TYPE from the input DXE_HEAP_GUARD_MEMORY_TYPES bitfield

  @param[in]  MemoryType            Memory type to check.
  @param[in]  HeapGuardMemoryType   DXE_HEAP_GUARD_MEMORY_TYPES bitfield

  @return TRUE  The given EFI_MEMORY_TYPE is TRUE in the given DXE_HEAP_GUARD_MEMORY_TYPES
  @return FALSE The given EFI_MEMORY_TYPE is FALSE in the given DXE_HEAP_GUARD_MEMORY_TYPES
**/
BOOLEAN
EFIAPI
GetDxeMemoryTypeSettingFromBitfield (
  IN EFI_MEMORY_TYPE              MemoryType,
  IN DXE_HEAP_GUARD_MEMORY_TYPES  HeapGuardMemoryType
  )
{
  return FALSE;
}
