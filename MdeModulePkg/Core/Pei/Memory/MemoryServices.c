/** @file
  EFI PEI Core memory services

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMain.h"

/**

  Initialize the memory services.

  @param PrivateData     Points to PeiCore's private instance data.
  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.
  @param OldCoreData     Pointer to the PEI Core data.
                         NULL if being run in non-permanent memory mode.

**/
VOID
InitializeMemoryServices (
  IN PEI_CORE_INSTANCE           *PrivateData,
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *OldCoreData
  )
{
  PrivateData->SwitchStackSignal = FALSE;

  //
  // First entering PeiCore, following code will initialized some field
  // in PeiCore's private data according to hand off data from SEC core.
  //
  if (OldCoreData == NULL) {
    PrivateData->PeiMemoryInstalled = FALSE;
    PrivateData->HobList.Raw        = SecCoreData->PeiTemporaryRamBase;

    PeiCoreBuildHobHandoffInfoTable (
      BOOT_WITH_FULL_CONFIGURATION,
      (EFI_PHYSICAL_ADDRESS)(UINTN)SecCoreData->PeiTemporaryRamBase,
      (UINTN)SecCoreData->PeiTemporaryRamSize
      );

    //
    // Set Ps to point to ServiceTableShadow in Cache
    //
    PrivateData->Ps = &(PrivateData->ServiceTableShadow);
  }

  return;
}

/**

  This function registers the found memory configuration with the PEI Foundation.

  The usage model is that the PEIM that discovers the permanent memory shall invoke this service.
  This routine will hold discoveried memory information into PeiCore's private data,
  and set SwitchStackSignal flag. After PEIM who discovery memory is dispatched,
  PeiDispatcher will migrate temporary memory to permanent memory.

  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param MemoryBegin        Start of memory address.
  @param MemoryLength       Length of memory.

  @return EFI_SUCCESS Always success.

**/
EFI_STATUS
EFIAPI
PeiInstallPeiMemory (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    MemoryBegin,
  IN UINT64                  MemoryLength
  )
{
  PEI_CORE_INSTANCE  *PrivateData;

  DEBUG ((DEBUG_INFO, "PeiInstallPeiMemory MemoryBegin 0x%LX, MemoryLength 0x%LX\n", MemoryBegin, MemoryLength));
  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);

  //
  // PEI_SERVICE.InstallPeiMemory should only be called one time during whole PEI phase.
  // If it is invoked more than one time, ASSERT information is given for developer debugging in debug tip and
  // simply return EFI_SUCCESS in release tip to ignore it.
  //
  if (PrivateData->PeiMemoryInstalled) {
    DEBUG ((DEBUG_ERROR, "ERROR: PeiInstallPeiMemory is called more than once!\n"));
    ASSERT (FALSE);
    return EFI_SUCCESS;
  }

  PrivateData->PhysicalMemoryBegin   = MemoryBegin;
  PrivateData->PhysicalMemoryLength  = MemoryLength;
  PrivateData->FreePhysicalMemoryTop = MemoryBegin + MemoryLength;

  PrivateData->SwitchStackSignal = TRUE;

  return EFI_SUCCESS;
}

/**
  Migrate memory pages allocated in pre-memory phase.
  Copy memory pages at temporary heap top to permanent heap top.

  @param[in] Private                Pointer to the private data passed in from caller.
  @param[in] TemporaryRamMigrated   Temporary memory has been migrated to permanent memory.

**/
VOID
MigrateMemoryPages (
  IN PEI_CORE_INSTANCE  *Private,
  IN BOOLEAN            TemporaryRamMigrated
  )
{
  EFI_PHYSICAL_ADDRESS  NewMemPagesBase;
  EFI_PHYSICAL_ADDRESS  MemPagesBase;

  Private->MemoryPages.Size = (UINTN)(Private->HobList.HandoffInformationTable->EfiMemoryTop -
                                      Private->HobList.HandoffInformationTable->EfiFreeMemoryTop);
  if (Private->MemoryPages.Size == 0) {
    //
    // No any memory page allocated in pre-memory phase.
    //
    return;
  }

  Private->MemoryPages.Base = Private->HobList.HandoffInformationTable->EfiFreeMemoryTop;

  ASSERT (Private->MemoryPages.Size <= Private->FreePhysicalMemoryTop);
  NewMemPagesBase  = Private->FreePhysicalMemoryTop - Private->MemoryPages.Size;
  NewMemPagesBase &= ~(UINT64)EFI_PAGE_MASK;
  ASSERT (NewMemPagesBase >= Private->PhysicalMemoryBegin);
  //
  // Copy memory pages at temporary heap top to permanent heap top.
  //
  if (TemporaryRamMigrated) {
    //
    // Memory pages at temporary heap top has been migrated to permanent heap,
    // Here still needs to copy them from permanent heap to permanent heap top.
    //
    MemPagesBase = Private->MemoryPages.Base;
    if (Private->HeapOffsetPositive) {
      MemPagesBase += Private->HeapOffset;
    } else {
      MemPagesBase -= Private->HeapOffset;
    }

    CopyMem ((VOID *)(UINTN)NewMemPagesBase, (VOID *)(UINTN)MemPagesBase, Private->MemoryPages.Size);
  } else {
    CopyMem ((VOID *)(UINTN)NewMemPagesBase, (VOID *)(UINTN)Private->MemoryPages.Base, Private->MemoryPages.Size);
  }

  if (NewMemPagesBase >= Private->MemoryPages.Base) {
    Private->MemoryPages.OffsetPositive = TRUE;
    Private->MemoryPages.Offset         = (UINTN)(NewMemPagesBase - Private->MemoryPages.Base);
  } else {
    Private->MemoryPages.OffsetPositive = FALSE;
    Private->MemoryPages.Offset         = (UINTN)(Private->MemoryPages.Base - NewMemPagesBase);
  }

  DEBUG ((DEBUG_INFO, "Pages Offset = 0x%lX\n", (UINT64)Private->MemoryPages.Offset));

  Private->FreePhysicalMemoryTop = NewMemPagesBase;
}

/**
  Removes any FV HOBs whose base address is not in PEI installed memory.

  @param[in] Private          Pointer to PeiCore's private data structure.

**/
VOID
RemoveFvHobsInTemporaryMemory (
  IN PEI_CORE_INSTANCE  *Private
  )
{
  EFI_PEI_HOB_POINTERS     Hob;
  EFI_HOB_FIRMWARE_VOLUME  *FirmwareVolumeHob;

  DEBUG ((DEBUG_INFO, "Removing FVs in FV HOB not already migrated to permanent memory.\n"));

  for (Hob.Raw = GetHobList (); !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if ((GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_FV) || (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_FV2) || (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_FV3)) {
      FirmwareVolumeHob = Hob.FirmwareVolume;
      DEBUG ((DEBUG_INFO, "  Found FV HOB.\n"));
      DEBUG ((
        DEBUG_INFO,
        "    BA=%016lx  L=%016lx\n",
        FirmwareVolumeHob->BaseAddress,
        FirmwareVolumeHob->Length
        ));
      if (
          !(
            ((EFI_PHYSICAL_ADDRESS)(UINTN)FirmwareVolumeHob->BaseAddress >= Private->PhysicalMemoryBegin) &&
            (((EFI_PHYSICAL_ADDRESS)(UINTN)FirmwareVolumeHob->BaseAddress + (FirmwareVolumeHob->Length - 1)) < Private->FreePhysicalMemoryTop)
            )
          )
      {
        DEBUG ((DEBUG_INFO, "      Removing FV HOB to an FV in T-RAM (was not migrated).\n"));
        Hob.Header->HobType = EFI_HOB_TYPE_UNUSED;
      }
    }
  }
}

/**
  Migrate the base address in firmware volume allocation HOBs
  from temporary memory to PEI installed memory.

  @param[in] PrivateData      Pointer to PeiCore's private data structure.
  @param[in] OrgFvHandle      Address of FV Handle in temporary memory.
  @param[in] FvHandle         Address of FV Handle in permanent memory.

**/
VOID
ConvertFvHob (
  IN PEI_CORE_INSTANCE  *PrivateData,
  IN UINTN              OrgFvHandle,
  IN UINTN              FvHandle
  )
{
  EFI_PEI_HOB_POINTERS      Hob;
  EFI_HOB_FIRMWARE_VOLUME   *FirmwareVolumeHob;
  EFI_HOB_FIRMWARE_VOLUME2  *FirmwareVolume2Hob;
  EFI_HOB_FIRMWARE_VOLUME3  *FirmwareVolume3Hob;

  DEBUG ((DEBUG_INFO, "Converting FVs in FV HOB.\n"));

  for (Hob.Raw = GetHobList (); !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_FV) {
      FirmwareVolumeHob = Hob.FirmwareVolume;
      if (FirmwareVolumeHob->BaseAddress == OrgFvHandle) {
        FirmwareVolumeHob->BaseAddress = FvHandle;
      }
    } else if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_FV2) {
      FirmwareVolume2Hob = Hob.FirmwareVolume2;
      if (FirmwareVolume2Hob->BaseAddress == OrgFvHandle) {
        FirmwareVolume2Hob->BaseAddress = FvHandle;
      }
    } else if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_FV3) {
      FirmwareVolume3Hob = Hob.FirmwareVolume3;
      if (FirmwareVolume3Hob->BaseAddress == OrgFvHandle) {
        FirmwareVolume3Hob->BaseAddress = FvHandle;
      }
    }
  }
}

/**
  Migrate MemoryBaseAddress in memory allocation HOBs
  from the temporary memory to PEI installed memory.

  @param[in] PrivateData        Pointer to PeiCore's private data structure.

**/
VOID
ConvertMemoryAllocationHobs (
  IN PEI_CORE_INSTANCE  *PrivateData
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob;
  EFI_PHYSICAL_ADDRESS       OldMemPagesBase;
  UINTN                      OldMemPagesSize;

  if (PrivateData->MemoryPages.Size == 0) {
    //
    // No any memory page allocated in pre-memory phase.
    //
    return;
  }

  OldMemPagesBase = PrivateData->MemoryPages.Base;
  OldMemPagesSize = PrivateData->MemoryPages.Size;

  MemoryAllocationHob = NULL;
  Hob.Raw             = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  while (Hob.Raw != NULL) {
    MemoryAllocationHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
    if ((MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress >= OldMemPagesBase) &&
        (MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress < (OldMemPagesBase + OldMemPagesSize))
        )
    {
      if (PrivateData->MemoryPages.OffsetPositive) {
        MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress += PrivateData->MemoryPages.Offset;
      } else {
        MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress -= PrivateData->MemoryPages.Offset;
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw);
  }
}

/**
  Internal function to build a HOB for the memory allocation.
  It will search and reuse the unused(freed) memory allocation HOB,
  or build memory allocation HOB normally if no unused(freed) memory allocation HOB found.

  @param[in] BaseAddress        The 64 bit physical address of the memory.
  @param[in] Length             The length of the memory allocation in bytes.
  @param[in] MemoryType         The type of memory allocated by this HOB.

**/
VOID
InternalBuildMemoryAllocationHob (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN EFI_MEMORY_TYPE       MemoryType
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob;

  //
  // Search unused(freed) memory allocation HOB.
  //
  MemoryAllocationHob = NULL;
  Hob.Raw             = GetFirstHob (EFI_HOB_TYPE_UNUSED);
  while (Hob.Raw != NULL) {
    if (Hob.Header->HobLength == sizeof (EFI_HOB_MEMORY_ALLOCATION)) {
      MemoryAllocationHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_UNUSED, Hob.Raw);
  }

  if (MemoryAllocationHob != NULL) {
    //
    // Reuse the unused(freed) memory allocation HOB.
    //
    MemoryAllocationHob->Header.HobType = EFI_HOB_TYPE_MEMORY_ALLOCATION;
    ZeroMem (&(MemoryAllocationHob->AllocDescriptor.Name), sizeof (EFI_GUID));
    MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress = BaseAddress;
    MemoryAllocationHob->AllocDescriptor.MemoryLength      = Length;
    MemoryAllocationHob->AllocDescriptor.MemoryType        = MemoryType;
    //
    // Zero the reserved space to match HOB spec
    //
    ZeroMem (MemoryAllocationHob->AllocDescriptor.Reserved, sizeof (MemoryAllocationHob->AllocDescriptor.Reserved));
  } else {
    //
    // No unused(freed) memory allocation HOB found.
    // Build memory allocation HOB normally.
    //
    BuildMemoryAllocationHob (
      BaseAddress,
      Length,
      MemoryType
      );
  }
}

/**
  Update or split memory allocation HOB for memory pages allocate and free.

  @param[in, out] MemoryAllocationHob   Pointer to the memory allocation HOB
                                        that needs to be updated or split.
                                        On output, it will be filled with
                                        the input Memory, Bytes and MemoryType.
  @param[in]      Memory                Memory to allocate or free.
  @param[in]      Bytes                 Bytes to allocate or free.
  @param[in]      MemoryType            EfiConventionalMemory for pages free,
                                        others for pages allocate.

**/
VOID
UpdateOrSplitMemoryAllocationHob (
  IN OUT EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob,
  IN EFI_PHYSICAL_ADDRESS           Memory,
  IN UINT64                         Bytes,
  IN EFI_MEMORY_TYPE                MemoryType
  )
{
  if ((Memory + Bytes) <
      (MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress + MemoryAllocationHob->AllocDescriptor.MemoryLength))
  {
    //
    // Last pages need to be split out.
    //
    InternalBuildMemoryAllocationHob (
      Memory + Bytes,
      (MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress + MemoryAllocationHob->AllocDescriptor.MemoryLength) - (Memory + Bytes),
      MemoryAllocationHob->AllocDescriptor.MemoryType
      );
  }

  if (Memory > MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress) {
    //
    // First pages need to be split out.
    //
    InternalBuildMemoryAllocationHob (
      MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress,
      Memory - MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress,
      MemoryAllocationHob->AllocDescriptor.MemoryType
      );
  }

  //
  // Update the memory allocation HOB.
  //
  MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress = Memory;
  MemoryAllocationHob->AllocDescriptor.MemoryLength      = Bytes;
  MemoryAllocationHob->AllocDescriptor.MemoryType        = MemoryType;
}

/**
  Merge adjacent free memory ranges in memory allocation HOBs.

  @retval TRUE          There are free memory ranges merged.
  @retval FALSE         No free memory ranges merged.

**/
BOOLEAN
MergeFreeMemoryInMemoryAllocationHob (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_PEI_HOB_POINTERS       Hob2;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryHob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryHob2;
  UINT64                     Start;
  UINT64                     End;
  BOOLEAN                    Merged;

  Merged = FALSE;

  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  while (Hob.Raw != NULL) {
    if (Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiConventionalMemory) {
      MemoryHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
      Start     = MemoryHob->AllocDescriptor.MemoryBaseAddress;
      End       = MemoryHob->AllocDescriptor.MemoryBaseAddress + MemoryHob->AllocDescriptor.MemoryLength;

      Hob2.Raw = GET_NEXT_HOB (Hob);
      Hob2.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw);
      while (Hob2.Raw != NULL) {
        if (Hob2.MemoryAllocation->AllocDescriptor.MemoryType == EfiConventionalMemory) {
          MemoryHob2 = (EFI_HOB_MEMORY_ALLOCATION *)Hob2.Raw;
          if (Start == (MemoryHob2->AllocDescriptor.MemoryBaseAddress + MemoryHob2->AllocDescriptor.MemoryLength)) {
            //
            // Merge adjacent two free memory ranges.
            //
            MemoryHob2->AllocDescriptor.MemoryLength += MemoryHob->AllocDescriptor.MemoryLength;
            Merged                                    = TRUE;
            //
            // Mark MemoryHob to be unused(freed).
            //
            MemoryHob->Header.HobType = EFI_HOB_TYPE_UNUSED;
            break;
          } else if (End == MemoryHob2->AllocDescriptor.MemoryBaseAddress) {
            //
            // Merge adjacent two free memory ranges.
            //
            MemoryHob2->AllocDescriptor.MemoryBaseAddress = MemoryHob->AllocDescriptor.MemoryBaseAddress;
            MemoryHob2->AllocDescriptor.MemoryLength     += MemoryHob->AllocDescriptor.MemoryLength;
            Merged                                        = TRUE;
            //
            // Mark MemoryHob to be unused(freed).
            //
            MemoryHob->Header.HobType = EFI_HOB_TYPE_UNUSED;
            break;
          }
        }

        Hob2.Raw = GET_NEXT_HOB (Hob2);
        Hob2.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob2.Raw);
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw);
  }

  return Merged;
}

/**
  Find free memory by searching memory allocation HOBs.

  @param[in]  MemoryType        The type of memory to allocate.
  @param[in]  Pages             The number of contiguous 4 KB pages to allocate.
  @param[in]  Granularity       Page allocation granularity.
  @param[out] Memory            Pointer to a physical address. On output, the address is set to the base
                                of the page range that was allocated.

  @retval EFI_SUCCESS           The memory range was successfully allocated.
  @retval EFI_NOT_FOUND         No memory allocation HOB with big enough free memory found.

**/
EFI_STATUS
FindFreeMemoryFromMemoryAllocationHob (
  IN  EFI_MEMORY_TYPE       MemoryType,
  IN  UINTN                 Pages,
  IN  UINTN                 Granularity,
  OUT EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob;
  UINT64                     Bytes;
  EFI_PHYSICAL_ADDRESS       BaseAddress;

  Bytes = LShiftU64 (Pages, EFI_PAGE_SHIFT);

  BaseAddress         = 0;
  MemoryAllocationHob = NULL;
  Hob.Raw             = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  while (Hob.Raw != NULL) {
    if ((Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiConventionalMemory) &&
        (Hob.MemoryAllocation->AllocDescriptor.MemoryLength >= Bytes))
    {
      //
      // Found one memory allocation HOB with big enough free memory.
      //
      MemoryAllocationHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
      BaseAddress         = MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress +
                            MemoryAllocationHob->AllocDescriptor.MemoryLength - Bytes;
      //
      // Make sure the granularity could be satisfied.
      //
      BaseAddress &= ~((EFI_PHYSICAL_ADDRESS)Granularity - 1);
      if (BaseAddress >= MemoryAllocationHob->AllocDescriptor.MemoryBaseAddress) {
        break;
      }

      BaseAddress         = 0;
      MemoryAllocationHob = NULL;
    }

    //
    // Continue to find.
    //
    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw);
  }

  if (MemoryAllocationHob != NULL) {
    UpdateOrSplitMemoryAllocationHob (MemoryAllocationHob, BaseAddress, Bytes, MemoryType);
    *Memory = BaseAddress;
    return EFI_SUCCESS;
  } else {
    if (MergeFreeMemoryInMemoryAllocationHob ()) {
      //
      // Retry if there are free memory ranges merged.
      //
      return FindFreeMemoryFromMemoryAllocationHob (MemoryType, Pages, Granularity, Memory);
    }

    return EFI_NOT_FOUND;
  }
}

/**
  The purpose of the service is to publish an interface that allows
  PEIMs to allocate memory ranges that are managed by the PEI Foundation.

  Prior to InstallPeiMemory() being called, PEI will allocate pages from the heap.
  After InstallPeiMemory() is called, PEI will allocate pages within the region
  of memory provided by InstallPeiMemory() service in a best-effort fashion.
  Location-specific allocations are not managed by the PEI foundation code.

  @param  PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param  MemoryType       The type of memory to allocate.
  @param  Pages            The number of contiguous 4 KB pages to allocate.
  @param  Memory           Pointer to a physical address. On output, the address is set to the base
                           of the page range that was allocated.

  @retval EFI_SUCCESS           The memory range was successfully allocated.
  @retval EFI_OUT_OF_RESOURCES  The pages could not be allocated.
  @retval EFI_INVALID_PARAMETER Type is not equal to EfiLoaderCode, EfiLoaderData, EfiRuntimeServicesCode,
                                EfiRuntimeServicesData, EfiBootServicesCode, EfiBootServicesData,
                                EfiACPIReclaimMemory, EfiReservedMemoryType, or EfiACPIMemoryNVS.

**/
EFI_STATUS
EFIAPI
PeiAllocatePages (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN       EFI_MEMORY_TYPE       MemoryType,
  IN       UINTN                 Pages,
  OUT      EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  EFI_STATUS            Status;
  PEI_CORE_INSTANCE     *PrivateData;
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PHYSICAL_ADDRESS  *FreeMemoryTop;
  EFI_PHYSICAL_ADDRESS  *FreeMemoryBottom;
  UINTN                 RemainingPages;
  UINTN                 Granularity;
  UINTN                 Padding;

  if ((MemoryType != EfiLoaderCode) &&
      (MemoryType != EfiLoaderData) &&
      (MemoryType != EfiRuntimeServicesCode) &&
      (MemoryType != EfiRuntimeServicesData) &&
      (MemoryType != EfiBootServicesCode) &&
      (MemoryType != EfiBootServicesData) &&
      (MemoryType != EfiACPIReclaimMemory) &&
      (MemoryType != EfiReservedMemoryType) &&
      (MemoryType != EfiACPIMemoryNVS))
  {
    return EFI_INVALID_PARAMETER;
  }

  Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  Hob.Raw     = PrivateData->HobList.Raw;

  if (Hob.Raw == NULL) {
    //
    // HOB is not initialized yet.
    //
    return EFI_NOT_AVAILABLE_YET;
  }

  if ((RUNTIME_PAGE_ALLOCATION_GRANULARITY > DEFAULT_PAGE_ALLOCATION_GRANULARITY) &&
      ((MemoryType == EfiACPIReclaimMemory) ||
       (MemoryType == EfiACPIMemoryNVS) ||
       (MemoryType == EfiRuntimeServicesCode) ||
       (MemoryType == EfiRuntimeServicesData)))
  {
    Granularity = RUNTIME_PAGE_ALLOCATION_GRANULARITY;

    DEBUG ((
      DEBUG_INFO,
      "AllocatePages: aligning allocation to %d KB\n",
      Granularity / SIZE_1KB
      ));
  }

  if (!PrivateData->PeiMemoryInstalled && PrivateData->SwitchStackSignal) {
    //
    // When PeiInstallMemory is called but temporary memory has *not* been moved to permanent memory,
    // the AllocatePage will depend on the field of PEI_CORE_INSTANCE structure.
    //
    FreeMemoryTop    = &(PrivateData->FreePhysicalMemoryTop);
    FreeMemoryBottom = &(PrivateData->PhysicalMemoryBegin);
  } else {
    FreeMemoryTop    = &(Hob.HandoffInformationTable->EfiFreeMemoryTop);
    FreeMemoryBottom = &(Hob.HandoffInformationTable->EfiFreeMemoryBottom);
  }

  //
  // Check to see if on correct boundary for the memory type.
  // If not aligned, make the allocation aligned.
  //
  Padding = *(FreeMemoryTop) & (Granularity - 1);
  if ((UINTN)(*FreeMemoryTop - *FreeMemoryBottom) < Padding) {
    DEBUG ((DEBUG_ERROR, "AllocatePages failed: Out of space after padding.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  *(FreeMemoryTop) -= Padding;
  if (Padding >= EFI_PAGE_SIZE) {
    //
    // Create a memory allocation HOB to cover
    // the pages that we will lose to rounding
    //
    InternalBuildMemoryAllocationHob (
      *(FreeMemoryTop),
      Padding & ~(UINTN)EFI_PAGE_MASK,
      EfiConventionalMemory
      );
  }

  //
  // Verify that there is sufficient memory to satisfy the allocation.
  //
  RemainingPages = (UINTN)(*FreeMemoryTop - *FreeMemoryBottom) >> EFI_PAGE_SHIFT;
  //
  // The number of remaining pages needs to be greater than or equal to that of the request pages.
  //
  Pages = ALIGN_VALUE (Pages, EFI_SIZE_TO_PAGES (Granularity));
  if (RemainingPages < Pages) {
    //
    // Try to find free memory by searching memory allocation HOBs.
    //
    Status = FindFreeMemoryFromMemoryAllocationHob (MemoryType, Pages, Granularity, Memory);
    if (!EFI_ERROR (Status)) {
      return Status;
    }

    DEBUG ((DEBUG_ERROR, "AllocatePages failed: No 0x%lx Pages is available.\n", (UINT64)Pages));
    DEBUG ((DEBUG_ERROR, "There is only left 0x%lx pages memory resource to be allocated.\n", (UINT64)RemainingPages));
    return EFI_OUT_OF_RESOURCES;
  } else {
    //
    // Update the PHIT to reflect the memory usage
    //
    *(FreeMemoryTop) -= Pages * EFI_PAGE_SIZE;

    //
    // Update the value for the caller
    //
    *Memory = *(FreeMemoryTop);

    //
    // Create a memory allocation HOB.
    //
    InternalBuildMemoryAllocationHob (
      *(FreeMemoryTop),
      Pages * EFI_PAGE_SIZE,
      MemoryType
      );

    return EFI_SUCCESS;
  }
}

/**
  Mark the memory allocation HOB to be unused(freed) and update *FreeMemoryTop
  if MemoryBaseAddress == *FreeMemoryTop.

  @param[in]      PrivateData                   Pointer to PeiCore's private data structure.
  @param[in, out] MemoryAllocationHobToFree     Pointer to memory allocation HOB to be freed.

**/
VOID
FreeMemoryAllocationHob (
  IN PEI_CORE_INSTANCE              *PrivateData,
  IN OUT EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHobToFree
  )
{
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_PHYSICAL_ADDRESS       *FreeMemoryTop;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob;

  Hob.Raw = PrivateData->HobList.Raw;

  if (!PrivateData->PeiMemoryInstalled && PrivateData->SwitchStackSignal) {
    //
    // When PeiInstallMemory is called but temporary memory has *not* been moved to permanent memory,
    // use the FreePhysicalMemoryTop field of PEI_CORE_INSTANCE structure.
    //
    FreeMemoryTop = &(PrivateData->FreePhysicalMemoryTop);
  } else {
    FreeMemoryTop = &(Hob.HandoffInformationTable->EfiFreeMemoryTop);
  }

  if (MemoryAllocationHobToFree->AllocDescriptor.MemoryBaseAddress == *FreeMemoryTop) {
    //
    // Update *FreeMemoryTop.
    //
    *FreeMemoryTop += MemoryAllocationHobToFree->AllocDescriptor.MemoryLength;
    //
    // Mark the memory allocation HOB to be unused(freed).
    //
    MemoryAllocationHobToFree->Header.HobType = EFI_HOB_TYPE_UNUSED;

    MemoryAllocationHob = NULL;
    Hob.Raw             = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
    while (Hob.Raw != NULL) {
      if ((Hob.MemoryAllocation->AllocDescriptor.MemoryType == EfiConventionalMemory) &&
          (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress == *FreeMemoryTop))
      {
        //
        // Found memory allocation HOB that has EfiConventionalMemory MemoryType and
        // MemoryBaseAddress == new *FreeMemoryTop.
        //
        MemoryAllocationHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
        break;
      }

      Hob.Raw = GET_NEXT_HOB (Hob);
      Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw);
    }

    //
    // Free memory allocation HOB iteratively.
    //
    if (MemoryAllocationHob != NULL) {
      FreeMemoryAllocationHob (PrivateData, MemoryAllocationHob);
    }
  }
}

/**
  Frees memory pages.

  @param[in] PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param[in] Memory             The base physical address of the pages to be freed.
  @param[in] Pages              The number of contiguous 4 KB pages to free.

  @retval EFI_SUCCESS           The requested pages were freed.
  @retval EFI_INVALID_PARAMETER Memory is not a page-aligned address or Pages is invalid.
  @retval EFI_NOT_FOUND         The requested memory pages were not allocated with
                                AllocatePages().

**/
EFI_STATUS
EFIAPI
PeiFreePages (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    Memory,
  IN UINTN                   Pages
  )
{
  PEI_CORE_INSTANCE          *PrivateData;
  UINT64                     Bytes;
  UINT64                     Start;
  UINT64                     End;
  EFI_PEI_HOB_POINTERS       Hob;
  EFI_HOB_MEMORY_ALLOCATION  *MemoryAllocationHob;

  Bytes = LShiftU64 (Pages, EFI_PAGE_SHIFT);
  Start = Memory;
  End   = Start + Bytes - 1;

  if ((Pages == 0) || ((Start & EFI_PAGE_MASK) != 0) || (Start >= End)) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  Hob.Raw     = PrivateData->HobList.Raw;

  if (Hob.Raw == NULL) {
    //
    // HOB is not initialized yet.
    //
    return EFI_NOT_AVAILABLE_YET;
  }

  MemoryAllocationHob = NULL;
  Hob.Raw             = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  while (Hob.Raw != NULL) {
    if ((Hob.MemoryAllocation->AllocDescriptor.MemoryType != EfiConventionalMemory) &&
        (Memory >= Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress) &&
        ((Memory + Bytes) <= (Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength)))
    {
      //
      // Found the memory allocation HOB that includes the memory pages to be freed.
      //
      MemoryAllocationHob = (EFI_HOB_MEMORY_ALLOCATION *)Hob.Raw;
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw);
  }

  if (MemoryAllocationHob != NULL) {
    UpdateOrSplitMemoryAllocationHob (MemoryAllocationHob, Memory, Bytes, EfiConventionalMemory);
    FreeMemoryAllocationHob (PrivateData, MemoryAllocationHob);
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**

  Pool allocation service. Before permanent memory is discovered, the pool will
  be allocated in the heap in temporary memory. Generally, the size of the heap in temporary
  memory does not exceed 64K, so the biggest pool size could be allocated is
  64K.

  @param PeiServices               An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Size                      Amount of memory required
  @param Buffer                    Address of pointer to the buffer

  @retval EFI_SUCCESS              The allocation was successful
  @retval EFI_OUT_OF_RESOURCES     There is not enough heap to satisfy the requirement
                                   to allocate the requested size.

**/
EFI_STATUS
EFIAPI
PeiAllocatePool (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN       UINTN             Size,
  OUT      VOID              **Buffer
  )
{
  EFI_STATUS           Status;
  EFI_HOB_MEMORY_POOL  *Hob;

  //
  // If some "post-memory" PEIM wishes to allocate larger pool,
  // it should use AllocatePages service instead.
  //

  //
  // Generally, the size of heap in temporary memory does not exceed 64K,
  // HobLength is multiples of 8 bytes, so the maximum size of pool is 0xFFF8 - sizeof (EFI_HOB_MEMORY_POOL)
  //
  if (Size > (0xFFF8 - sizeof (EFI_HOB_MEMORY_POOL))) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = PeiServicesCreateHob (
             EFI_HOB_TYPE_MEMORY_POOL,
             (UINT16)(sizeof (EFI_HOB_MEMORY_POOL) + Size),
             (VOID **)&Hob
             );
  ASSERT_EFI_ERROR (Status);

  if (EFI_ERROR (Status)) {
    *Buffer = NULL;
  } else {
    *Buffer = Hob + 1;
  }

  return Status;
}
