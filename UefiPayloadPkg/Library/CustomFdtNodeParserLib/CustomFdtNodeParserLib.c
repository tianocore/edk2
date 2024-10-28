/** @file
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <PiPei.h>
#include <Pi/PiHob.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

/**
  Add a new HOB to the HOB List.

  @param HobType            Type of the new HOB.
  @param HobLength          Length of the new HOB to allocate.

  @return  NULL if there is no space to create a hob.
  @return  The address point to the new created hob.

**/
VOID *
EFIAPI
CreateHob (
  IN  UINT16  HobType,
  IN  UINT16  HobLength
  );

/**
  Add HOB into HOB list
  @param[in]  Hob    The HOB to be added into the HOB list.
**/
VOID
AddNewHob (
  IN EFI_PEI_HOB_POINTERS  *Hob
  );

/**
  Check the HOB and decide if it is need inside Payload
  Payload maintainer may make decision which HOB is need or needn't
  Then add the check logic in the function.
  @param[in] Hob The HOB to check
  @retval TRUE  If HOB is need inside Payload
  @retval FALSE If HOB is needn't inside Payload
**/
BOOLEAN
EFIAPI
FitIsHobNeed (
  EFI_PEI_HOB_POINTERS  Hob
  )
{
  if (FixedPcdGetBool (PcdHandOffFdtEnable)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_HANDOFF) {
      return FALSE;
    }

    if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      if (CompareGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gUniversalPayloadDeviceTreeGuid)) {
        return FALSE;
      }

      if (CompareGuid (&Hob.MemoryAllocationModule->MemoryAllocationHeader.Name, &gEfiHobMemoryAllocModuleGuid)) {
        return FALSE;
      }

      if ((Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiReservedMemoryType) ||
          (Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiBootServicesCode) ||
          (Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiBootServicesData) ||
          (Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiRuntimeServicesCode) ||
          (Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiRuntimeServicesData) ||
          (Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiACPIReclaimMemory) ||
          (Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiACPIMemoryNVS))
      {
        return FALSE;
      }
    }

    if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
        return FALSE;
      }
    }

    if (Hob.Header->HobType == EFI_HOB_TYPE_GUID_EXTENSION) {
      if (CompareGuid (&Hob.Guid->Name, &gUniversalPayloadSmbios3TableGuid)) {
        return FALSE;
      }

      if (CompareGuid (&Hob.Guid->Name, &gUniversalPayloadSerialPortInfoGuid)) {
        return FALSE;
      }

      if (CompareGuid (&Hob.Guid->Name, &gUniversalPayloadAcpiTableGuid)) {
        return FALSE;
      }

      if (CompareGuid (&Hob.Guid->Name, &gUniversalPayloadPciRootBridgeInfoGuid)) {
        return FALSE;
      }
    }
  }

  // Arrive here mean the HOB is need
  return TRUE;
}

/**
  It will Parse FDT -custom node based on information from bootloaders.
  @param[in]  FdtBase The starting memory address of FdtBase
  @param[in]  HobList The starting memory address of New Hob list.

**/
UINTN
EFIAPI
CustomFdtNodeParser (
  IN VOID  *FdtBase,
  IN VOID  *HobList
  )
{
  INT32                 Node, CustomNode;
  INT32                 TempLen;
  CONST FDT_PROPERTY    *PropertyPtr;
  UINT64                *Data64;
  UINTN                 CHobList;
  EFI_PEI_HOB_POINTERS  Hob;

  CHobList = (UINTN)HobList;

  DEBUG ((DEBUG_INFO, "%a() #1 \n", __func__));

  //
  // Look for if exists hob list node
  //
  Node = FdtSubnodeOffsetNameLen (FdtBase, 0, "options", (INT32)AsciiStrLen ("options"));
  if (Node > 0) {
    DEBUG ((DEBUG_INFO, "  Found options node (%08X)", Node));
    CustomNode = FdtSubnodeOffsetNameLen (FdtBase, Node, "upl-custom", (INT32)AsciiStrLen ("upl-custom"));
    if (CustomNode > 0) {
      DEBUG ((DEBUG_INFO, "  Found upl-custom node (%08X)", CustomNode));
      PropertyPtr = FdtGetProperty (FdtBase, CustomNode, "hoblistptr", &TempLen);
      if (PropertyPtr != NULL) {
        Data64   = (UINT64 *)(PropertyPtr->Data);
        CHobList = (UINTN)Fdt64ToCpu (ReadUnaligned64 (Data64));
        DEBUG ((DEBUG_INFO, "  Found hob list node (%08X) -pointer  %016lX\n", CustomNode, CHobList));
      } else {
        DEBUG ((DEBUG_INFO, "  Not Found hob list node\n"));
        return CHobList;
      }
    }
  }

  Hob.Raw = (UINT8 *)CHobList;

  //
  // Since payload created new Hob, move all hobs except PHIT from boot loader hob list.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    if (FitIsHobNeed (Hob)) {
      // Add this hob to payload HOB
      AddNewHob (&Hob);
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  return CHobList;
}
