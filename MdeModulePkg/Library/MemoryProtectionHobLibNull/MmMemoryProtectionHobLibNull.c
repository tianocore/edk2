/** @file
Library defines the gMmMps global

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/MmMemoryProtectionHobLib.h>

// According to the C Specification, a global variable
// which is uninitialized will be zero. The net effect
// is memory protections will be OFF.
MM_MEMORY_PROTECTION_SETTINGS  gMmMps;

/**
  Gets the input EFI_MEMORY_TYPE from the input MM_HEAP_GUARD_MEMORY_TYPES bitfield

  @param[in]  MemoryType            Memory type to check.
  @param[in]  HeapGuardMemoryType   MM_HEAP_GUARD_MEMORY_TYPES bitfield

  @return TRUE  The given EFI_MEMORY_TYPE is TRUE in the given MM_HEAP_GUARD_MEMORY_TYPES
  @return FALSE The given EFI_MEMORY_TYPE is FALSE in the given MM_HEAP_GUARD_MEMORY_TYPES
**/
BOOLEAN
EFIAPI
GetMmMemoryTypeSettingFromBitfield (
  IN EFI_MEMORY_TYPE             MemoryType,
  IN MM_HEAP_GUARD_MEMORY_TYPES  HeapGuardMemoryType
  )
{
  return FALSE;
}
