/** @file
  Platform PEI module include file.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <IndustryStandard/E820.h>

typedef struct {
  UINT64    BaseAddr;
  UINT64    Length;
  UINT32    Type;
  UINT32    Reserved;
} MEMMAP_ENTRY;

/**
  Create  memory range hand off block.

  @param  MemoryBase    memory base address.
  @param  MemoryLimit  memory length.

  @return  VOID
**/
VOID
AddMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit
  );

/**
  Publish PEI core memory

  @return EFI_SUCCESS     The PEIM initialized successfully.
**/
EFI_STATUS
PublishPeiMemory (
  VOID
  );

/**
  Publish system RAM and reserve memory regions

  @return  VOID
**/
VOID
InitializeRamRegions (
  VOID
  );

/**
  Publish PEI & DXE (Decompressed) Memory based FVs to let PEI
  and DXE know about them.

  @retval EFI_SUCCESS   Platform PEI FVs were initialized successfully.
**/
EFI_STATUS
PeiFvInitialization (
  VOID
  );

/**
  Gets the Virtual Memory Map of corresponding platforms.

  This Virtual Memory Map is used by initialize the MMU on corresponding
  platforms.

  @param[out]   MemoryTable    Array of EFI_MEMORY_DESCRIPTOR
                               describing a Physical-to-Virtual Memory
                               mapping. This array must be ended by a
                               zero-filled entry. The allocated memory
                               will not be freed.
**/
VOID
EFIAPI
GetMemoryMapPolicy (
  OUT EFI_MEMORY_DESCRIPTOR  **MemoryTable
  );

#endif // PLATFORM_H_
