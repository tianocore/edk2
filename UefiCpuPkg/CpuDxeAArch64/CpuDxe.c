/** @file
  ARM64 CPU DXE driver.

  Copyright (c) 2024, NewFW Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AArch64/AArch64.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

#include "ArmMmuLib.h"

#define MAX_DESCRIPTORS 128
#define GIC_PAGES 128

ARM_MEMORY_REGION_DESCRIPTOR  VirtualMemoryTable[MAX_DESCRIPTORS];

VOID ArmUplUpdateMemoryMap ( VOID )
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR *Resource;

  UINTN Idx                  = 0;
  UINTN GicInerfaceBase      = (UINTN)PcdGet64 (PcdGicInterruptInterfaceBase);
  UINTN GicDistributorBase   = (UINTN)PcdGet64 (PcdGicDistributorBase);
  UINTN GicRedistributorBase = (UINTN)PcdGet64 (PcdGicRedistributorsBase);
  UINTN SerialRegBase        = (UINTN)PcdGet64 (PcdSerialRegisterBase);
  UINTN SkipGicd             = 0 ;
  UINTN SkipGicr             = 0 ;
  UINTN SkipGici             = 0 ;
  UINTN SkipSerial           = 0 ;


  ZeroMem (VirtualMemoryTable, MAX_DESCRIPTORS * sizeof (ARM_MEMORY_REGION_DESCRIPTOR));

  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);

  while (Hob.Raw != NULL) {
    Resource  = (EFI_HOB_RESOURCE_DESCRIPTOR*)Hob.Raw;

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);

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

	if (Resource->PhysicalStart == GicDistributorBase) {
      SkipGicd = 1;
	}

	if (Resource->PhysicalStart == GicRedistributorBase) {
      SkipGicr = 1;
	}

	if (Resource->PhysicalStart == GicInerfaceBase) {
      SkipGici = 1;
	}

    if (Resource->PhysicalStart == SerialRegBase) {
      SkipSerial = 1;
    }

    Idx++;

  }

  if (SkipGici == 0) {
    VirtualMemoryTable[Idx].PhysicalBase = GicInerfaceBase;
    VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[Idx].PhysicalBase;
    VirtualMemoryTable[Idx].Length       = GIC_PAGES * EFI_PAGE_SIZE;
    VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    Idx++;
  }

  if (SkipGicd == 0) {
    VirtualMemoryTable[Idx].PhysicalBase = GicDistributorBase;
    VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[Idx].PhysicalBase;
    VirtualMemoryTable[Idx].Length       = GIC_PAGES * EFI_PAGE_SIZE;
    VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    Idx++;
  }

  if (SkipGicr == 0) {
    VirtualMemoryTable[Idx].PhysicalBase = GicRedistributorBase;
    VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[Idx].PhysicalBase;
    VirtualMemoryTable[Idx].Length       = GIC_PAGES * EFI_PAGE_SIZE;
    VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    Idx++;
  }

  if (SkipSerial == 0) {
    VirtualMemoryTable[Idx].PhysicalBase = SerialRegBase;
    VirtualMemoryTable[Idx].VirtualBase  = VirtualMemoryTable[Idx].PhysicalBase;
    VirtualMemoryTable[Idx].Length       = EFI_PAGE_SIZE;
    VirtualMemoryTable[Idx].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    Idx++;
  }
}

VOID
InitMmu (
  VOID
   )
{
  VOID                          *TranslationTableBase;
  UINTN                         TranslationTableSize;
  RETURN_STATUS                 Status;

  ArmUplUpdateMemoryMap ();

  Status = ArmConfigureMmu (VirtualMemoryTable, &TranslationTableBase, &TranslationTableSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Failed to enable MMU\n"));
  }
}

extern UINTN ArmGicV3GetControlRegister(VOID);
extern VOID ArmGicV3SetControlRegister(UINTN Value);

VOID
ClearEioMode(
    VOID
    )
{
    UINTN GicCtlr = ArmGicV3GetControlRegister ();

    if ((GicCtlr >> 1 ) & 0x1) {
      GicCtlr &= 0xFFFFFFFFFFFFFFFDULL;
      ArmGicV3SetControlRegister (GicCtlr);
    }
}

/**
  Initialize the state information for the CPU Architectural Protocol.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to the System Table.

  @retval EFI_SUCCESS           Thread can be successfully created
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Cannot create the thread

**/
EFI_STATUS
EFIAPI
InitializeCpu(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  ClearEioMode();

  InitMmu();

  return EFI_SUCCESS;
}
