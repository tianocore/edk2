/** @file
  Memory Detection for Virtual Machines.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/ResourcePublicationLib.h>
#include <Register/LoongArch64/Csr.h>
#include <Uefi/UefiSpec.h>
#include "Platform.h"

#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  (128)
#define LOONGARCH_FW_RAM_TOP                BASE_256MB

/**
  Publish PEI core memory

  @return EFI_SUCCESS     The PEIM initialized successfully.
**/
EFI_STATUS
PublishPeiMemory (
  VOID
  )
{
  EFI_STATUS            Status;
  UINT64                Base;
  UINT64                Size;
  UINT64                RamTop;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  UINTN                 Processed;
  MEMMAP_ENTRY          MemoryMapEntry;

  //
  // Determine the range of memory to use during PEI
  //
  Base   = FixedPcdGet64 (PcdOvmfSecPeiTempRamBase) + FixedPcdGet32 (PcdOvmfSecPeiTempRamSize);
  RamTop = 0;

  Status = QemuFwCfgFindFile ("etc/memmap", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FwCfgSize % sizeof MemoryMapEntry != 0) {
    return EFI_PROTOCOL_ERROR;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  for (Processed = 0; Processed < FwCfgSize; Processed += sizeof MemoryMapEntry) {
    QemuFwCfgReadBytes (sizeof MemoryMapEntry, &MemoryMapEntry);
    if (MemoryMapEntry.Type != EfiAcpiAddressRangeMemory) {
      continue;
    }

    //
    // Find memory map entry where PEI temp stack is located
    //
    if ((MemoryMapEntry.BaseAddr <= Base) &&
        (Base < (MemoryMapEntry.BaseAddr + MemoryMapEntry.Length)))
    {
      RamTop = MemoryMapEntry.BaseAddr + MemoryMapEntry.Length;
      break;
    }
  }

  if (RamTop == 0) {
    DEBUG ((DEBUG_ERROR, "ERROR: No memory map entry contains temp stack \n"));
    ASSERT (FALSE);
  }

  Size = RamTop - Base;

  //
  // Publish this memory to the PEI Core
  //
  Status = PublishSystemMemory (Base, Size);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "Publish Memory Initialize done.\n"));
  return Status;
}

/**
  Peform Memory Detection
  Publish system RAM and reserve memory regions
**/
VOID
InitializeRamRegions (
  VOID
  )
{
  EFI_STATUS            Status;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  MEMMAP_ENTRY          MemoryMapEntry;
  MEMMAP_ENTRY          *StartEntry;
  MEMMAP_ENTRY          *pEntry;
  UINTN                 Processed;

  Status = QemuFwCfgFindFile ("etc/memmap", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a %d read etc/memmap error Status %d \n", __func__, __LINE__, Status));
    return;
  }

  if (FwCfgSize % sizeof MemoryMapEntry != 0) {
    DEBUG ((DEBUG_ERROR, "no MemoryMapEntry FwCfgSize:%d\n", FwCfgSize));
    return;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  StartEntry = AllocatePages (EFI_SIZE_TO_PAGES (FwCfgSize));
  QemuFwCfgReadBytes (FwCfgSize, StartEntry);
  for (Processed = 0; Processed < (FwCfgSize / sizeof MemoryMapEntry); Processed++) {
    pEntry = StartEntry + Processed;
    if (pEntry->Length == 0) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "MemmapEntry Base %p length %p  type %d\n", pEntry->BaseAddr, pEntry->Length, pEntry->Type));
    if (pEntry->Type != EfiAcpiAddressRangeMemory) {
      continue;
    }

    AddMemoryRangeHob (pEntry->BaseAddr, pEntry->BaseAddr + pEntry->Length);
  }

  //
  // When 0 address protection is enabled,
  // 0-4k memory needs to be preallocated to prevent UEFI applications from allocating use,
  // such as grub
  //
  if (PcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT0) {
    BuildMemoryAllocationHob (
      0,
      EFI_PAGE_SIZE,
      EfiBootServicesData
      );
  }
}

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
  )
{
  EFI_STATUS             Status;
  FIRMWARE_CONFIG_ITEM   FwCfgItem;
  UINTN                  FwCfgSize;
  MEMMAP_ENTRY           MemoryMapEntry;
  MEMMAP_ENTRY           *StartEntry;
  MEMMAP_ENTRY           *pEntry;
  UINTN                  Processed;
  EFI_MEMORY_DESCRIPTOR  *VirtualMemoryTable;
  UINTN                  Index = 0;

  ASSERT (MemoryTable != NULL);

  VirtualMemoryTable = AllocatePool (
                         sizeof (EFI_MEMORY_DESCRIPTOR) *
                         MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                         );

  //
  // Add the 0x10000000-0x20000000. In the virtual machine, this area use for CPU UART, flash, PIC etc.
  //
  VirtualMemoryTable[Index].PhysicalStart = BASE_256MB;
  VirtualMemoryTable[Index].VirtualStart  = VirtualMemoryTable[Index].PhysicalStart;
  VirtualMemoryTable[Index].NumberOfPages = EFI_SIZE_TO_PAGES (SIZE_256MB);
  VirtualMemoryTable[Index].Attribute     = EFI_MEMORY_UC;
  ++Index;

  Status = QemuFwCfgFindFile ("etc/memmap", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a %d read etc/memmap error Status %d \n", __func__, __LINE__, Status));
    ZeroMem (&VirtualMemoryTable[Index], sizeof (EFI_MEMORY_DESCRIPTOR));
    *MemoryTable = VirtualMemoryTable;
    return;
  }

  if (FwCfgSize % sizeof MemoryMapEntry != 0) {
    DEBUG ((DEBUG_ERROR, "no MemoryMapEntry FwCfgSize:%d\n", FwCfgSize));
  }

  QemuFwCfgSelectItem (FwCfgItem);
  StartEntry = AllocatePages (EFI_SIZE_TO_PAGES (FwCfgSize));
  QemuFwCfgReadBytes (FwCfgSize, StartEntry);
  for (Processed = 0; Processed < (FwCfgSize / sizeof MemoryMapEntry); Processed++) {
    pEntry = StartEntry + Processed;
    if (pEntry->Length == 0) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "MemmapEntry Base %p length %p  type %d\n", pEntry->BaseAddr, pEntry->Length, pEntry->Type));
    VirtualMemoryTable[Index].PhysicalStart = pEntry->BaseAddr;
    VirtualMemoryTable[Index].VirtualStart  = VirtualMemoryTable[Index].PhysicalStart;
    VirtualMemoryTable[Index].NumberOfPages = EFI_SIZE_TO_PAGES (pEntry->Length);
    VirtualMemoryTable[Index].Attribute     = EFI_MEMORY_WB;
    ++Index;
  }

  FreePages (StartEntry, EFI_SIZE_TO_PAGES (FwCfgSize));
  // End of Table
  ZeroMem (&VirtualMemoryTable[Index], sizeof (EFI_MEMORY_DESCRIPTOR));
  *MemoryTable = VirtualMemoryTable;
}
