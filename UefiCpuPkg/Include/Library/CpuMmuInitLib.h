/** @file

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_MMU_INIT_LIB_H_
#define CPU_MMU_INIT_LIB_H_

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

/**
  Create a page table and initialize the memory management unit(MMU).

  @param[in]   MemoryTable           A pointer to a memory ragion table.
  @param[out]  TranslationTableBase  A pointer to a translation table base address.
  @param[out]  TranslationTableSize  A pointer to a translation table base size.

  @retval  EFI_SUCCESS                Configure MMU successfully.
           EFI_INVALID_PARAMETER      MemoryTable is NULL.
           EFI_UNSUPPORTED            Out of memory space or size not aligned.
**/
EFI_STATUS
EFIAPI
ConfigureMemoryManagementUnit (
  IN  EFI_MEMORY_DESCRIPTOR  *MemoryTable,
  OUT VOID                   **TranslationTableBase OPTIONAL,
  OUT UINTN                  *TranslationTableSize  OPTIONAL
  );

#endif // CPU_MMU_INIT_LIB_H_
