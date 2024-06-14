/** @file
  CPU Memory Map Unit Initialization library header.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiSpec.h>

/**
  Create a page table and initialize the memory management unit(MMU).

  @param[in]   MemoryTable           A pointer to a memory ragion table.

  @retval  EFI_SUCCESS                Configure MMU successfully.
           EFI_INVALID_PARAMETER      MemoryTable is NULL.
           EFI_UNSUPPORTED            MemoryRegionMap failed or out of memory space or size not aligned
                                      or MaxLivel out of bound.
**/
EFI_STATUS
EFIAPI
ConfigureMemoryManagementUnit (
  IN  EFI_MEMORY_DESCRIPTOR  *MemoryTable
  );
