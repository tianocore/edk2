/** @file

Library for controlling hob-backed memory protection settings

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_MEMORY_PROTECTION_HOB_HELPER_LIB_H_
#define MM_MEMORY_PROTECTION_HOB_HELPER_LIB_H_

#include <Guid/MmMemoryProtectionSettings.h>

//
//  The global used to access current Memory Protection Settings
//
extern MM_MEMORY_PROTECTION_SETTINGS  gMmMps;

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
  );

#endif
