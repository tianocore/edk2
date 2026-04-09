/** @file
  This file handles post bootloader stage operations for AARCH64 architecture.

  Copyright 2025 Google LLC
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <AArch64/AArch64.h>
#include <Library/ArmLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Protocol/Cpu.h>

#include "BlSupportDxe.h"

#define MAX_DESCRIPTORS  256
ARM_MEMORY_REGION_DESCRIPTOR  VirtualMemoryTable[MAX_DESCRIPTORS];

STATIC
EFI_STATUS
BlUpdateMemoryMap (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *Resource;
  VOID                         *TranslationTableBase;
  UINTN                        TranslationTableSize;
  UINTN                        Idx = 0;

  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);

  while (Hob.Raw != NULL) {
    Resource = (EFI_HOB_RESOURCE_DESCRIPTOR *)Hob.Raw;
    Hob.Raw  = GET_NEXT_HOB (Hob);
    Hob.Raw  = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);

    VirtualMemoryTable[Idx].PhysicalBase = Resource->PhysicalStart;
    VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[Idx].PhysicalBase;
    VirtualMemoryTable[Idx].Length       = ALIGN_VALUE (Resource->ResourceLength, EFI_PAGE_SIZE);

    if (Resource->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      VirtualMemoryTable[Idx].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
    } else if (Resource->ResourceType == EFI_RESOURCE_MEMORY_MAPPED_IO) {
      VirtualMemoryTable[Idx].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    } else {
      VirtualMemoryTable[Idx].Attributes = ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED;
    }

    Idx++;
    ASSERT (Idx <= MAX_DESCRIPTORS);
  }

  Status = ArmConfigureMmu (VirtualMemoryTable, &TranslationTableBase, &TranslationTableSize);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Architecture level additional operation which needs to be performed before
  launching payload.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
BlArchAdditionalOps (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  RETURN_STATUS  Status = EFI_SUCCESS;

  if (!ArmMmuEnabled ()) {
    // Enable MMU if MMU is disabled.
    Status = BlUpdateMemoryMap ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to enable MMU: %r\n", __func__, Status));
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  return Status;
}
