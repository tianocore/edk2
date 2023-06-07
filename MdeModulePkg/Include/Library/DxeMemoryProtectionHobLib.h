/** @file

Library for controlling hob-backed memory protection settings

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DXE_MEMORY_PROTECTION_HOB_HELPER_LIB_H_
#define DXE_MEMORY_PROTECTION_HOB_HELPER_LIB_H_

#include <Guid/DxeMemoryProtectionSettings.h>

//
//  The global used to access current Memory Protection Settings
//
extern DXE_MEMORY_PROTECTION_SETTINGS  gDxeMps;

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
  );

#endif
