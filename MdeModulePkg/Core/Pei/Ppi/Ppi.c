/** @file
  EFI PEI Core PPI services

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMain.h"

/**

  Migrate Pointer from the temporary memory to PEI installed memory.

  @param Pointer         Pointer to the Pointer needs to be converted.
  @param TempBottom      Base of old temporary memory
  @param TempTop         Top of old temporary memory
  @param Offset          Offset of new memory to old temporary memory.
  @param OffsetPositive  Positive flag of Offset value.

**/
VOID
ConvertPointer (
  IN OUT VOID  **Pointer,
  IN UINTN     TempBottom,
  IN UINTN     TempTop,
  IN UINTN     Offset,
  IN BOOLEAN   OffsetPositive
  )
{
  if (((UINTN)*Pointer < TempTop) &&
      ((UINTN)*Pointer >= TempBottom))
  {
    if (OffsetPositive) {
      *Pointer = (VOID *)((UINTN)*Pointer + Offset);
    } else {
      *Pointer = (VOID *)((UINTN)*Pointer - Offset);
    }
  }
}

/**

  Migrate Pointer in ranges of the temporary memory to PEI installed memory.

  @param SecCoreData     Points to a data structure containing SEC to PEI handoff data, such as the size
                         and location of temporary RAM, the stack location and the BFV location.
  @param PrivateData     Pointer to PeiCore's private data structure.
  @param Pointer         Pointer to the Pointer needs to be converted.

**/
VOID
ConvertPointerInRanges (
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *PrivateData,
  IN OUT VOID                    **Pointer
  )
{
  UINT8  IndexHole;

  if (PrivateData->MemoryPages.Size != 0) {
    //
    // Convert PPI pointer in old memory pages
    // It needs to be done before Convert PPI pointer in old Heap
    //
    ConvertPointer (
      Pointer,
      (UINTN)PrivateData->MemoryPages.Base,
      (UINTN)PrivateData->MemoryPages.Base + PrivateData->MemoryPages.Size,
      PrivateData->MemoryPages.Offset,
      PrivateData->MemoryPages.OffsetPositive
      );
  }

  //
  // Convert PPI pointer in old Heap
  //
  ConvertPointer (
    Pointer,
    (UINTN)SecCoreData->PeiTemporaryRamBase,
    (UINTN)SecCoreData->PeiTemporaryRamBase + SecCoreData->PeiTemporaryRamSize,
    PrivateData->HeapOffset,
    PrivateData->HeapOffsetPositive
    );

  //
  // Convert PPI pointer in old Stack
  //
  ConvertPointer (
    Pointer,
    (UINTN)SecCoreData->StackBase,
    (UINTN)SecCoreData->StackBase + SecCoreData->StackSize,
    PrivateData->StackOffset,
    PrivateData->StackOffsetPositive
    );

  //
  // Convert PPI pointer in old TempRam Hole
  //
  for (IndexHole = 0; IndexHole < HOLE_MAX_NUMBER; IndexHole++) {
    if (PrivateData->HoleData[IndexHole].Size == 0) {
      continue;
    }

    ConvertPointer (
      Pointer,
      (UINTN)PrivateData->HoleData[IndexHole].Base,
      (UINTN)PrivateData->HoleData[IndexHole].Base + PrivateData->HoleData[IndexHole].Size,
      PrivateData->HoleData[IndexHole].Offset,
      PrivateData->HoleData[IndexHole].OffsetPositive
      );
  }
}

/**

  Migrate Single PPI Pointer from the temporary memory to PEI installed memory.

  @param SecCoreData     Points to a data structure containing SEC to PEI handoff data, such as the size
                         and location of temporary RAM, the stack location and the BFV location.
  @param PrivateData     Pointer to PeiCore's private data structure.
  @param PpiPointer      Pointer to Ppi

**/
VOID
ConvertSinglePpiPointer (
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *PrivateData,
  IN PEI_PPI_LIST_POINTERS       *PpiPointer
  )
{
  //
  // 1. Convert the pointer to the PPI descriptor from the old TempRam
  //    to the relocated physical memory.
  // It (for the pointer to the PPI descriptor) needs to be done before 2 (for
  // the pointer to the GUID) and 3 (for the pointer to the PPI interface structure).
  //
  ConvertPointerInRanges (SecCoreData, PrivateData, &PpiPointer->Raw);
  //
  // 2. Convert the pointer to the GUID in the PPI or NOTIFY descriptor
  //    from the old TempRam to the relocated physical memory.
  //
  ConvertPointerInRanges (SecCoreData, PrivateData, (VOID **)&PpiPointer->Ppi->Guid);
  //
  // 3. Convert the pointer to the PPI interface structure in the PPI descriptor
  //    from the old TempRam to the relocated physical memory.
  //
  ConvertPointerInRanges (SecCoreData, PrivateData, (VOID **)&PpiPointer->Ppi->Ppi);
}

/**

  Migrate PPI Pointers from the temporary memory to PEI installed memory.

  @param SecCoreData     Points to a data structure containing SEC to PEI handoff data, such as the size
                         and location of temporary RAM, the stack location and the BFV location.
  @param PrivateData     Pointer to PeiCore's private data structure.

**/
VOID
ConvertPpiPointers (
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *PrivateData
  )
{
  UINT8  Index;

  //
  // Convert normal PPIs.
  //
  for (Index = 0; Index < PrivateData->PpiData.PpiList.CurrentCount; Index++) {
    ConvertSinglePpiPointer (
      SecCoreData,
      PrivateData,
      &PrivateData->PpiData.PpiList.PpiPtrs[Index]
      );
  }

  //
  // Convert Callback Notification PPIs.
  //
  for (Index = 0; Index < PrivateData->PpiData.CallbackNotifyList.CurrentCount; Index++) {
    ConvertSinglePpiPointer (
      SecCoreData,
      PrivateData,
      &PrivateData->PpiData.CallbackNotifyList.NotifyPtrs[Index]
      );
  }

  //
  // Convert Dispatch Notification PPIs.
  //
  for (Index = 0; Index < PrivateData->PpiData.DispatchNotifyList.CurrentCount; Index++) {
    ConvertSinglePpiPointer (
      SecCoreData,
      PrivateData,
      &PrivateData->PpiData.DispatchNotifyList.NotifyPtrs[Index]
      );
  }
}

/**

  Migrate Notify Pointers inside an FV from temporary memory to permanent memory.

  @param PrivateData      Pointer to PeiCore's private data structure.
  @param OrgFvHandle      Address of FV Handle in temporary memory.
  @param FvHandle         Address of FV Handle in permanent memory.
  @param FvSize           Size of the FV.

**/
VOID
ConvertPpiPointersFv (
  IN  PEI_CORE_INSTANCE  *PrivateData,
  IN  UINTN              OrgFvHandle,
  IN  UINTN              FvHandle,
  IN  UINTN              FvSize
  )
{
  UINT8                             Index;
  UINTN                             Offset;
  BOOLEAN                           OffsetPositive;
  EFI_PEI_FIRMWARE_VOLUME_INFO_PPI  *FvInfoPpi;
  UINT8                             GuidIndex;
  EFI_GUID                          *Guid;
  EFI_GUID                          *GuidCheckList[2];

  GuidCheckList[0] = &gEfiPeiFirmwareVolumeInfoPpiGuid;
  GuidCheckList[1] = &gEfiPeiFirmwareVolumeInfo2PpiGuid;

  if (FvHandle > OrgFvHandle) {
    OffsetPositive = TRUE;
    Offset         = FvHandle - OrgFvHandle;
  } else {
    OffsetPositive = FALSE;
    Offset         = OrgFvHandle - FvHandle;
  }

  DEBUG ((DEBUG_VERBOSE, "Converting PPI pointers in FV.\n"));
  DEBUG ((
    DEBUG_VERBOSE,
    "  OrgFvHandle at 0x%08x. FvHandle at 0x%08x. FvSize = 0x%x\n",
    (UINTN)OrgFvHandle,
    (UINTN)FvHandle,
    FvSize
    ));
  DEBUG ((
    DEBUG_VERBOSE,
    "    OrgFvHandle range: 0x%08x - 0x%08x\n",
    OrgFvHandle,
    OrgFvHandle + FvSize
    ));

  for (Index = 0; Index < PrivateData->PpiData.CallbackNotifyList.CurrentCount; Index++) {
    ConvertPointer (
      (VOID **)&PrivateData->PpiData.CallbackNotifyList.NotifyPtrs[Index].Raw,
      OrgFvHandle,
      OrgFvHandle + FvSize,
      Offset,
      OffsetPositive
      );
    ConvertPointer (
      (VOID **)&PrivateData->PpiData.CallbackNotifyList.NotifyPtrs[Index].Notify->Guid,
      OrgFvHandle,
      OrgFvHandle + FvSize,
      Offset,
      OffsetPositive
      );
    ConvertPointer (
      (VOID **)&PrivateData->PpiData.CallbackNotifyList.NotifyPtrs[Index].Notify->Notify,
      OrgFvHandle,
      OrgFvHandle + FvSize,
      Offset,
      OffsetPositive
      );
  }

  for (Index = 0; Index < PrivateData->PpiData.DispatchNotifyList.CurrentCount; Index++) {
    ConvertPointer (
      (VOID **)&PrivateData->PpiData.DispatchNotifyList.NotifyPtrs[Index].Raw,
      OrgFvHandle,
      OrgFvHandle + FvSize,
      Offset,
      OffsetPositive
      );
    ConvertPointer (
      (VOID **)&PrivateData->PpiData.DispatchNotifyList.NotifyPtrs[Index].Notify->Guid,
      OrgFvHandle,
      OrgFvHandle + FvSize,
      Offset,
      OffsetPositive
      );
    ConvertPointer (
      (VOID **)&PrivateData->PpiData.DispatchNotifyList.NotifyPtrs[Index].Notify->Notify,
      OrgFvHandle,
      OrgFvHandle + FvSize,
      Offset,
      OffsetPositive
      );
  }

  for (Index = 0; Index < PrivateData->PpiData.PpiList.CurrentCount; Index++) {
    ConvertPointer (
      (VOID **)&PrivateData->PpiData.PpiList.PpiPtrs[Index].Raw,
      OrgFvHandle,
      OrgFvHandle + FvSize,
      Offset,
      OffsetPositive
      );
    ConvertPointer (
      (VOID **)&PrivateData->PpiData.PpiList.PpiPtrs[Index].Ppi->Guid,
      OrgFvHandle,
      OrgFvHandle + FvSize,
      Offset,
      OffsetPositive
      );
    ConvertPointer (
      (VOID **)&PrivateData->PpiData.PpiList.PpiPtrs[Index].Ppi->Ppi,
      OrgFvHandle,
      OrgFvHandle + FvSize,
      Offset,
      OffsetPositive
      );

    Guid = PrivateData->PpiData.PpiList.PpiPtrs[Index].Ppi->Guid;
    for (GuidIndex = 0; GuidIndex < ARRAY_SIZE (GuidCheckList); ++GuidIndex) {
      //
      // Don't use CompareGuid function here for performance reasons.
      // Instead we compare the GUID as INT32 at a time and branch
      // on the first failed comparison.
      //
      if ((((INT32 *)Guid)[0] == ((INT32 *)GuidCheckList[GuidIndex])[0]) &&
          (((INT32 *)Guid)[1] == ((INT32 *)GuidCheckList[GuidIndex])[1]) &&
          (((INT32 *)Guid)[2] == ((INT32 *)GuidCheckList[GuidIndex])[2]) &&
          (((INT32 *)Guid)[3] == ((INT32 *)GuidCheckList[GuidIndex])[3]))
      {
        FvInfoPpi = PrivateData->PpiData.PpiList.PpiPtrs[Index].Ppi->Ppi;
        DEBUG ((DEBUG_VERBOSE, "      FvInfo: %p -> ", FvInfoPpi->FvInfo));
        if ((UINTN)FvInfoPpi->FvInfo == OrgFvHandle) {
          ConvertPointer (
            (VOID **)&FvInfoPpi->FvInfo,
            OrgFvHandle,
            OrgFvHandle + FvSize,
            Offset,
            OffsetPositive
            );
          DEBUG ((DEBUG_VERBOSE, "%p", FvInfoPpi->FvInfo));
        }

        DEBUG ((DEBUG_VERBOSE, "\n"));
        break;
      }
    }
  }
}

/**

  Dumps the PPI lists to debug output.

  @param PrivateData     Points to PeiCore's private instance data.

**/
VOID
DumpPpiList (
  IN PEI_CORE_INSTANCE  *PrivateData
  )
{
  DEBUG_CODE_BEGIN ();
  UINTN  Index;

  if (PrivateData == NULL) {
    return;
  }

  for (Index = 0; Index < PrivateData->PpiData.CallbackNotifyList.CurrentCount; Index++) {
    DEBUG ((
      DEBUG_VERBOSE,
      "CallbackNotify[%2d] {%g} at 0x%x (%a)\n",
      Index,
      PrivateData->PpiData.CallbackNotifyList.NotifyPtrs[Index].Notify->Guid,
      (UINTN)PrivateData->PpiData.CallbackNotifyList.NotifyPtrs[Index].Raw,
      (
       !(
         ((EFI_PHYSICAL_ADDRESS)(UINTN)PrivateData->PpiData.CallbackNotifyList.NotifyPtrs[Index].Raw >= PrivateData->PhysicalMemoryBegin) &&
         (((EFI_PHYSICAL_ADDRESS)((UINTN)PrivateData->PpiData.CallbackNotifyList.NotifyPtrs[Index].Raw) + sizeof (EFI_PEI_NOTIFY_DESCRIPTOR)) < PrivateData->FreePhysicalMemoryTop)
         )
        ? "CAR" : "Post-Memory"
      )
      ));
  }

  for (Index = 0; Index < PrivateData->PpiData.DispatchNotifyList.CurrentCount; Index++) {
    DEBUG ((
      DEBUG_VERBOSE,
      "DispatchNotify[%2d] {%g} at 0x%x (%a)\n",
      Index,
      PrivateData->PpiData.DispatchNotifyList.NotifyPtrs[Index].Notify->Guid,
      (UINTN)PrivateData->PpiData.DispatchNotifyList.NotifyPtrs[Index].Raw,
      (
       !(
         ((EFI_PHYSICAL_ADDRESS)(UINTN)PrivateData->PpiData.DispatchNotifyList.NotifyPtrs[Index].Raw >= PrivateData->PhysicalMemoryBegin) &&
         (((EFI_PHYSICAL_ADDRESS)((UINTN)PrivateData->PpiData.DispatchNotifyList.NotifyPtrs[Index].Raw) + sizeof (EFI_PEI_NOTIFY_DESCRIPTOR)) < PrivateData->FreePhysicalMemoryTop)
         )
      ? "CAR" : "Post-Memory"
      )
      ));
  }

  for (Index = 0; Index < PrivateData->PpiData.PpiList.CurrentCount; Index++) {
    DEBUG ((
      DEBUG_VERBOSE,
      "PPI[%2d] {%g} at 0x%x (%a)\n",
      Index,
      PrivateData->PpiData.PpiList.PpiPtrs[Index].Ppi->Guid,
      (UINTN)PrivateData->PpiData.PpiList.PpiPtrs[Index].Raw,
      (
       !(
         ((EFI_PHYSICAL_ADDRESS)(UINTN)PrivateData->PpiData.PpiList.PpiPtrs[Index].Raw >= PrivateData->PhysicalMemoryBegin) &&
         (((EFI_PHYSICAL_ADDRESS)((UINTN)PrivateData->PpiData.PpiList.PpiPtrs[Index].Raw) + sizeof (EFI_PEI_PPI_DESCRIPTOR)) < PrivateData->FreePhysicalMemoryTop)
         )
      ? "CAR" : "Post-Memory"
      )
      ));
  }

  DEBUG_CODE_END ();
}

/**

  This function installs an interface in the PEI PPI database by GUID.
  The purpose of the service is to publish an interface that other parties
  can use to call additional PEIMs.

  @param PeiServices                An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param PpiList                    Pointer to a list of PEI PPI Descriptors.
  @param Single                     TRUE if only single entry in the PpiList.
                                    FALSE if the PpiList is ended with an entry which has the
                                    EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST flag set in its Flags field.

  @retval EFI_SUCCESS              if all PPIs in PpiList are successfully installed.
  @retval EFI_INVALID_PARAMETER    if PpiList is NULL pointer
                                   if any PPI in PpiList is not valid
  @retval EFI_OUT_OF_RESOURCES     if there is no more memory resource to install PPI

**/
EFI_STATUS
InternalPeiInstallPpi (
  IN CONST EFI_PEI_SERVICES        **PeiServices,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *PpiList,
  IN BOOLEAN                       Single
  )
{
  PEI_CORE_INSTANCE  *PrivateData;
  PEI_PPI_LIST       *PpiListPointer;
  UINTN              Index;
  UINTN              LastCount;
  VOID               *TempPtr;

  if (PpiList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);

  PpiListPointer = &PrivateData->PpiData.PpiList;
  Index          = PpiListPointer->CurrentCount;
  LastCount      = Index;

  //
  // This is loop installs all PPI descriptors in the PpiList.  It is terminated
  // by the EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST being set in the last
  // EFI_PEI_PPI_DESCRIPTOR in the list.
  //

  for ( ; ;) {
    //
    // Check if it is a valid PPI.
    // If not, rollback list to exclude all in this list.
    // Try to indicate which item failed.
    //
    if ((PpiList->Flags & EFI_PEI_PPI_DESCRIPTOR_PPI) == 0) {
      PpiListPointer->CurrentCount = LastCount;
      DEBUG ((DEBUG_ERROR, "ERROR -> InstallPpi: %g %p\n", PpiList->Guid, PpiList->Ppi));
      return EFI_INVALID_PARAMETER;
    }

    if (Index >= PpiListPointer->MaxCount) {
      //
      // Run out of room, grow the buffer.
      //
      TempPtr = AllocateZeroPool (
                  sizeof (PEI_PPI_LIST_POINTERS) * (PpiListPointer->MaxCount + PPI_GROWTH_STEP)
                  );
      ASSERT (TempPtr != NULL);
      CopyMem (
        TempPtr,
        PpiListPointer->PpiPtrs,
        sizeof (PEI_PPI_LIST_POINTERS) * PpiListPointer->MaxCount
        );
      PpiListPointer->PpiPtrs  = TempPtr;
      PpiListPointer->MaxCount = PpiListPointer->MaxCount + PPI_GROWTH_STEP;
    }

    DEBUG ((DEBUG_INFO, "Install PPI: %g\n", PpiList->Guid));
    PpiListPointer->PpiPtrs[Index].Ppi = (EFI_PEI_PPI_DESCRIPTOR *)PpiList;
    Index++;
    PpiListPointer->CurrentCount++;

    if (Single) {
      //
      // Only single entry in the PpiList.
      //
      break;
    } else if ((PpiList->Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) ==
               EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST)
    {
      //
      // Continue until the end of the PPI List.
      //
      break;
    }

    //
    // Go to the next descriptor.
    //
    PpiList++;
  }

  //
  // Process any callback level notifies for newly installed PPIs.
  //
  ProcessNotify (
    PrivateData,
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    LastCount,
    PpiListPointer->CurrentCount,
    0,
    PrivateData->PpiData.CallbackNotifyList.CurrentCount
    );

  return EFI_SUCCESS;
}

/**

  This function installs an interface in the PEI PPI database by GUID.
  The purpose of the service is to publish an interface that other parties
  can use to call additional PEIMs.

  @param PeiServices                An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param PpiList                    Pointer to a list of PEI PPI Descriptors.

  @retval EFI_SUCCESS              if all PPIs in PpiList are successfully installed.
  @retval EFI_INVALID_PARAMETER    if PpiList is NULL pointer
                                   if any PPI in PpiList is not valid
  @retval EFI_OUT_OF_RESOURCES     if there is no more memory resource to install PPI

**/
EFI_STATUS
EFIAPI
PeiInstallPpi (
  IN CONST EFI_PEI_SERVICES        **PeiServices,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *PpiList
  )
{
  return InternalPeiInstallPpi (PeiServices, PpiList, FALSE);
}

/**

  This function reinstalls an interface in the PEI PPI database by GUID.
  The purpose of the service is to publish an interface that other parties can
  use to replace an interface of the same name in the protocol database with a
  different interface.

  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param OldPpi                 Pointer to the old PEI PPI Descriptors.
  @param NewPpi                 Pointer to the new PEI PPI Descriptors.

  @retval EFI_SUCCESS           if the operation was successful
  @retval EFI_INVALID_PARAMETER if OldPpi or NewPpi is NULL
  @retval EFI_INVALID_PARAMETER if NewPpi is not valid
  @retval EFI_NOT_FOUND         if the PPI was not in the database

**/
EFI_STATUS
EFIAPI
PeiReInstallPpi (
  IN CONST EFI_PEI_SERVICES        **PeiServices,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *OldPpi,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *NewPpi
  )
{
  PEI_CORE_INSTANCE  *PrivateData;
  UINTN              Index;

  if ((OldPpi == NULL) || (NewPpi == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((NewPpi->Flags & EFI_PEI_PPI_DESCRIPTOR_PPI) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);

  //
  // Find the old PPI instance in the database.  If we can not find it,
  // return the EFI_NOT_FOUND error.
  //
  for (Index = 0; Index < PrivateData->PpiData.PpiList.CurrentCount; Index++) {
    if (OldPpi == PrivateData->PpiData.PpiList.PpiPtrs[Index].Ppi) {
      break;
    }
  }

  if (Index == PrivateData->PpiData.PpiList.CurrentCount) {
    return EFI_NOT_FOUND;
  }

  //
  // Replace the old PPI with the new one.
  //
  DEBUG ((DEBUG_INFO, "Reinstall PPI: %g\n", NewPpi->Guid));
  PrivateData->PpiData.PpiList.PpiPtrs[Index].Ppi = (EFI_PEI_PPI_DESCRIPTOR *)NewPpi;

  //
  // Process any callback level notifies for the newly installed PPI.
  //
  ProcessNotify (
    PrivateData,
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    Index,
    Index+1,
    0,
    PrivateData->PpiData.CallbackNotifyList.CurrentCount
    );

  return EFI_SUCCESS;
}

/**

  Locate a given named PPI.


  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Guid               Pointer to GUID of the PPI.
  @param Instance           Instance Number to discover.
  @param PpiDescriptor      Pointer to reference the found descriptor. If not NULL,
                            returns a pointer to the descriptor (includes flags, etc)
  @param Ppi                Pointer to reference the found PPI

  @retval EFI_SUCCESS   if the PPI is in the database
  @retval EFI_NOT_FOUND if the PPI is not in the database

**/
EFI_STATUS
EFIAPI
PeiLocatePpi (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN CONST EFI_GUID              *Guid,
  IN UINTN                       Instance,
  IN OUT EFI_PEI_PPI_DESCRIPTOR  **PpiDescriptor,
  IN OUT VOID                    **Ppi
  )
{
  PEI_CORE_INSTANCE       *PrivateData;
  UINTN                   Index;
  EFI_GUID                *CheckGuid;
  EFI_PEI_PPI_DESCRIPTOR  *TempPtr;

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);

  //
  // Search the data base for the matching instance of the GUIDed PPI.
  //
  for (Index = 0; Index < PrivateData->PpiData.PpiList.CurrentCount; Index++) {
    TempPtr   = PrivateData->PpiData.PpiList.PpiPtrs[Index].Ppi;
    CheckGuid = TempPtr->Guid;

    //
    // Don't use CompareGuid function here for performance reasons.
    // Instead we compare the GUID as INT32 at a time and branch
    // on the first failed comparison.
    //
    if ((((INT32 *)Guid)[0] == ((INT32 *)CheckGuid)[0]) &&
        (((INT32 *)Guid)[1] == ((INT32 *)CheckGuid)[1]) &&
        (((INT32 *)Guid)[2] == ((INT32 *)CheckGuid)[2]) &&
        (((INT32 *)Guid)[3] == ((INT32 *)CheckGuid)[3]))
    {
      if (Instance == 0) {
        if (PpiDescriptor != NULL) {
          *PpiDescriptor = TempPtr;
        }

        if (Ppi != NULL) {
          *Ppi = TempPtr->Ppi;
        }

        return EFI_SUCCESS;
      }

      Instance--;
    }
  }

  return EFI_NOT_FOUND;
}

/**

  This function installs a notification service to be called back when a given
  interface is installed or reinstalled. The purpose of the service is to publish
  an interface that other parties can use to call additional PPIs that may materialize later.

  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param NotifyList         Pointer to list of Descriptors to notify upon.
  @param Single             TRUE if only single entry in the NotifyList.
                            FALSE if the NotifyList is ended with an entry which has the
                            EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST flag set in its Flags field.

  @retval EFI_SUCCESS           if successful
  @retval EFI_OUT_OF_RESOURCES  if no space in the database
  @retval EFI_INVALID_PARAMETER if not a good descriptor

**/
EFI_STATUS
InternalPeiNotifyPpi (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN CONST EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyList,
  IN BOOLEAN                          Single
  )
{
  PEI_CORE_INSTANCE         *PrivateData;
  PEI_CALLBACK_NOTIFY_LIST  *CallbackNotifyListPointer;
  UINTN                     CallbackNotifyIndex;
  UINTN                     LastCallbackNotifyCount;
  PEI_DISPATCH_NOTIFY_LIST  *DispatchNotifyListPointer;
  UINTN                     DispatchNotifyIndex;
  UINTN                     LastDispatchNotifyCount;
  VOID                      *TempPtr;

  if (NotifyList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);

  CallbackNotifyListPointer = &PrivateData->PpiData.CallbackNotifyList;
  CallbackNotifyIndex       = CallbackNotifyListPointer->CurrentCount;
  LastCallbackNotifyCount   = CallbackNotifyIndex;

  DispatchNotifyListPointer = &PrivateData->PpiData.DispatchNotifyList;
  DispatchNotifyIndex       = DispatchNotifyListPointer->CurrentCount;
  LastDispatchNotifyCount   = DispatchNotifyIndex;

  //
  // This is loop installs all Notify descriptors in the NotifyList.  It is
  // terminated by the EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST being set in the last
  // EFI_PEI_NOTIFY_DESCRIPTOR in the list.
  //

  for ( ; ;) {
    //
    // If some of the PPI data is invalid restore original Notify PPI database value
    //
    if ((NotifyList->Flags & EFI_PEI_PPI_DESCRIPTOR_NOTIFY_TYPES) == 0) {
      CallbackNotifyListPointer->CurrentCount = LastCallbackNotifyCount;
      DispatchNotifyListPointer->CurrentCount = LastDispatchNotifyCount;
      DEBUG ((DEBUG_ERROR, "ERROR -> NotifyPpi: %g %p\n", NotifyList->Guid, NotifyList->Notify));
      return EFI_INVALID_PARAMETER;
    }

    if ((NotifyList->Flags & EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK) != 0) {
      if (CallbackNotifyIndex >= CallbackNotifyListPointer->MaxCount) {
        //
        // Run out of room, grow the buffer.
        //
        TempPtr = AllocateZeroPool (
                    sizeof (PEI_PPI_LIST_POINTERS) * (CallbackNotifyListPointer->MaxCount + CALLBACK_NOTIFY_GROWTH_STEP)
                    );
        ASSERT (TempPtr != NULL);
        CopyMem (
          TempPtr,
          CallbackNotifyListPointer->NotifyPtrs,
          sizeof (PEI_PPI_LIST_POINTERS) * CallbackNotifyListPointer->MaxCount
          );
        CallbackNotifyListPointer->NotifyPtrs = TempPtr;
        CallbackNotifyListPointer->MaxCount   = CallbackNotifyListPointer->MaxCount + CALLBACK_NOTIFY_GROWTH_STEP;
      }

      CallbackNotifyListPointer->NotifyPtrs[CallbackNotifyIndex].Notify = (EFI_PEI_NOTIFY_DESCRIPTOR *)NotifyList;
      CallbackNotifyIndex++;
      CallbackNotifyListPointer->CurrentCount++;
    } else {
      if (DispatchNotifyIndex >= DispatchNotifyListPointer->MaxCount) {
        //
        // Run out of room, grow the buffer.
        //
        TempPtr = AllocateZeroPool (
                    sizeof (PEI_PPI_LIST_POINTERS) * (DispatchNotifyListPointer->MaxCount + DISPATCH_NOTIFY_GROWTH_STEP)
                    );
        ASSERT (TempPtr != NULL);
        CopyMem (
          TempPtr,
          DispatchNotifyListPointer->NotifyPtrs,
          sizeof (PEI_PPI_LIST_POINTERS) * DispatchNotifyListPointer->MaxCount
          );
        DispatchNotifyListPointer->NotifyPtrs = TempPtr;
        DispatchNotifyListPointer->MaxCount   = DispatchNotifyListPointer->MaxCount + DISPATCH_NOTIFY_GROWTH_STEP;
      }

      DispatchNotifyListPointer->NotifyPtrs[DispatchNotifyIndex].Notify = (EFI_PEI_NOTIFY_DESCRIPTOR *)NotifyList;
      DispatchNotifyIndex++;
      DispatchNotifyListPointer->CurrentCount++;
    }

    DEBUG ((DEBUG_INFO, "Register PPI Notify: %g\n", NotifyList->Guid));

    if (Single) {
      //
      // Only single entry in the NotifyList.
      //
      break;
    } else if ((NotifyList->Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) ==
               EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST)
    {
      //
      // Continue until the end of the Notify List.
      //
      break;
    }

    //
    // Go to the next descriptor.
    //
    NotifyList++;
  }

  //
  // Process any callback level notifies for all previously installed PPIs.
  //
  ProcessNotify (
    PrivateData,
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    0,
    PrivateData->PpiData.PpiList.CurrentCount,
    LastCallbackNotifyCount,
    CallbackNotifyListPointer->CurrentCount
    );

  return EFI_SUCCESS;
}

/**

  This function installs a notification service to be called back when a given
  interface is installed or reinstalled. The purpose of the service is to publish
  an interface that other parties can use to call additional PPIs that may materialize later.

  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param NotifyList         Pointer to list of Descriptors to notify upon.

  @retval EFI_SUCCESS           if successful
  @retval EFI_OUT_OF_RESOURCES  if no space in the database
  @retval EFI_INVALID_PARAMETER if not a good descriptor

**/
EFI_STATUS
EFIAPI
PeiNotifyPpi (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN CONST EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyList
  )
{
  return InternalPeiNotifyPpi (PeiServices, NotifyList, FALSE);
}

/**

  Process the Notify List at dispatch level.

  @param PrivateData  PeiCore's private data structure.

**/
VOID
ProcessDispatchNotifyList (
  IN PEI_CORE_INSTANCE  *PrivateData
  )
{
  UINTN  TempValue;

  while (TRUE) {
    //
    // Check if the PEIM that was just dispatched resulted in any
    // Notifies getting installed.  If so, go process any dispatch
    // level Notifies that match the previously installed PPIs.
    // Use "while" instead of "if" since ProcessNotify can modify
    // DispatchNotifyList.CurrentCount (with NotifyPpi) so we have
    // to iterate until the same.
    //
    while (PrivateData->PpiData.DispatchNotifyList.LastDispatchedCount != PrivateData->PpiData.DispatchNotifyList.CurrentCount) {
      TempValue = PrivateData->PpiData.DispatchNotifyList.CurrentCount;
      ProcessNotify (
        PrivateData,
        EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH,
        0,
        PrivateData->PpiData.PpiList.LastDispatchedCount,
        PrivateData->PpiData.DispatchNotifyList.LastDispatchedCount,
        PrivateData->PpiData.DispatchNotifyList.CurrentCount
        );
      PrivateData->PpiData.DispatchNotifyList.LastDispatchedCount = TempValue;
    }

    //
    // Check if the PEIM that was just dispatched resulted in any
    // PPIs getting installed.  If so, go process any dispatch
    // level Notifies that match the installed PPIs.
    // Use "while" instead of "if" since ProcessNotify can modify
    // PpiList.CurrentCount (with InstallPpi) so we have to iterate
    // until the same.
    //
    while (PrivateData->PpiData.PpiList.LastDispatchedCount != PrivateData->PpiData.PpiList.CurrentCount) {
      TempValue = PrivateData->PpiData.PpiList.CurrentCount;
      ProcessNotify (
        PrivateData,
        EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH,
        PrivateData->PpiData.PpiList.LastDispatchedCount,
        PrivateData->PpiData.PpiList.CurrentCount,
        0,
        PrivateData->PpiData.DispatchNotifyList.LastDispatchedCount
        );
      PrivateData->PpiData.PpiList.LastDispatchedCount = TempValue;
    }

    if (PrivateData->PpiData.DispatchNotifyList.LastDispatchedCount == PrivateData->PpiData.DispatchNotifyList.CurrentCount) {
      break;
    }
  }

  return;
}

/**

  Process notifications.

  @param PrivateData        PeiCore's private data structure
  @param NotifyType         Type of notify to fire.
  @param InstallStartIndex  Install Beginning index.
  @param InstallStopIndex   Install Ending index.
  @param NotifyStartIndex   Notify Beginning index.
  @param NotifyStopIndex    Notify Ending index.

**/
VOID
ProcessNotify (
  IN PEI_CORE_INSTANCE  *PrivateData,
  IN UINTN              NotifyType,
  IN INTN               InstallStartIndex,
  IN INTN               InstallStopIndex,
  IN INTN               NotifyStartIndex,
  IN INTN               NotifyStopIndex
  )
{
  INTN                       Index1;
  INTN                       Index2;
  EFI_GUID                   *SearchGuid;
  EFI_GUID                   *CheckGuid;
  EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor;

  for (Index1 = NotifyStartIndex; Index1 < NotifyStopIndex; Index1++) {
    if (NotifyType == EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK) {
      NotifyDescriptor = PrivateData->PpiData.CallbackNotifyList.NotifyPtrs[Index1].Notify;
    } else {
      NotifyDescriptor = PrivateData->PpiData.DispatchNotifyList.NotifyPtrs[Index1].Notify;
    }

    CheckGuid = NotifyDescriptor->Guid;

    for (Index2 = InstallStartIndex; Index2 < InstallStopIndex; Index2++) {
      SearchGuid = PrivateData->PpiData.PpiList.PpiPtrs[Index2].Ppi->Guid;
      //
      // Don't use CompareGuid function here for performance reasons.
      // Instead we compare the GUID as INT32 at a time and branch
      // on the first failed comparison.
      //
      if ((((INT32 *)SearchGuid)[0] == ((INT32 *)CheckGuid)[0]) &&
          (((INT32 *)SearchGuid)[1] == ((INT32 *)CheckGuid)[1]) &&
          (((INT32 *)SearchGuid)[2] == ((INT32 *)CheckGuid)[2]) &&
          (((INT32 *)SearchGuid)[3] == ((INT32 *)CheckGuid)[3]))
      {
        DEBUG ((
          DEBUG_INFO,
          "Notify: PPI Guid: %g, Peim notify entry point: %p\n",
          SearchGuid,
          NotifyDescriptor->Notify
          ));
        NotifyDescriptor->Notify (
                            (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                            NotifyDescriptor,
                            (PrivateData->PpiData.PpiList.PpiPtrs[Index2].Ppi)->Ppi
                            );
      }
    }
  }
}

/**
  Process PpiList from SEC phase.

  @param PeiServices    An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param PpiList        Points to a list of one or more PPI descriptors to be installed initially by the PEI core.
                        These PPI's will be installed and/or immediately signaled if they are notification type.

**/
VOID
ProcessPpiListFromSec (
  IN CONST EFI_PEI_SERVICES        **PeiServices,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *PpiList
  )
{
  EFI_STATUS              Status;
  EFI_SEC_HOB_DATA_PPI    *SecHobDataPpi;
  EFI_HOB_GENERIC_HEADER  *SecHobList;

  for ( ; ;) {
    if ((PpiList->Flags & EFI_PEI_PPI_DESCRIPTOR_NOTIFY_TYPES) != 0) {
      //
      // It is a notification PPI.
      //
      Status = InternalPeiNotifyPpi (PeiServices, (CONST EFI_PEI_NOTIFY_DESCRIPTOR *)PpiList, TRUE);
      ASSERT_EFI_ERROR (Status);
    } else {
      //
      // It is a normal PPI.
      //
      Status = InternalPeiInstallPpi (PeiServices, PpiList, TRUE);
      ASSERT_EFI_ERROR (Status);
    }

    if ((PpiList->Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) == EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) {
      //
      // Continue until the end of the PPI List.
      //
      break;
    }

    PpiList++;
  }

  //
  // If the EFI_SEC_HOB_DATA_PPI is in the list of PPIs passed to the PEI entry point,
  // the PEI Foundation will call the GetHobs() member function and install all HOBs
  // returned into the HOB list. It does this after installing all PPIs passed from SEC
  // into the PPI database and before dispatching any PEIMs.
  //
  Status = PeiLocatePpi (PeiServices, &gEfiSecHobDataPpiGuid, 0, NULL, (VOID **)&SecHobDataPpi);
  if (!EFI_ERROR (Status)) {
    Status = SecHobDataPpi->GetHobs (SecHobDataPpi, &SecHobList);
    if (!EFI_ERROR (Status)) {
      Status = PeiInstallSecHobData (PeiServices, SecHobList);
      ASSERT_EFI_ERROR (Status);
    }
  }
}

/**

  Migrate PPI Pointers of PEI_CORE from temporary memory to permanent memory.

  @param PrivateData      Pointer to PeiCore's private data structure.
  @param CoreFvHandle     Address of PEI_CORE FV Handle in temporary memory.

**/
VOID
ConvertPeiCorePpiPointers (
  IN  PEI_CORE_INSTANCE   *PrivateData,
  IN  PEI_CORE_FV_HANDLE  *CoreFvHandle
  )
{
  EFI_FV_FILE_INFO      FileInfo;
  EFI_PHYSICAL_ADDRESS  OrgImageBase;
  EFI_PHYSICAL_ADDRESS  MigratedImageBase;
  UINTN                 PeiCoreModuleSize;
  EFI_PEI_FILE_HANDLE   PeiCoreFileHandle;
  VOID                  *PeiCoreImageBase;
  VOID                  *PeiCoreEntryPoint;
  EFI_STATUS            Status;

  PeiCoreFileHandle = NULL;

  //
  // Find the PEI Core in the BFV in temporary memory.
  //
  Status =  CoreFvHandle->FvPpi->FindFileByType (
                                   CoreFvHandle->FvPpi,
                                   EFI_FV_FILETYPE_PEI_CORE,
                                   CoreFvHandle->FvHandle,
                                   &PeiCoreFileHandle
                                   );
  ASSERT_EFI_ERROR (Status);

  if (!EFI_ERROR (Status)) {
    Status = CoreFvHandle->FvPpi->GetFileInfo (CoreFvHandle->FvPpi, PeiCoreFileHandle, &FileInfo);
    ASSERT_EFI_ERROR (Status);

    Status = PeiGetPe32Data (PeiCoreFileHandle, &PeiCoreImageBase);
    ASSERT_EFI_ERROR (Status);

    //
    // Find PEI Core EntryPoint in the BFV in temporary memory.
    //
    Status = PeCoffLoaderGetEntryPoint ((VOID *)(UINTN)PeiCoreImageBase, &PeiCoreEntryPoint);
    ASSERT_EFI_ERROR (Status);

    OrgImageBase      = (UINTN)PeiCoreImageBase;
    MigratedImageBase = (UINTN)_ModuleEntryPoint - ((UINTN)PeiCoreEntryPoint - (UINTN)PeiCoreImageBase);

    //
    // Size of loaded PEI_CORE in permanent memory.
    //
    PeiCoreModuleSize = (UINTN)FileInfo.BufferSize - ((UINTN)OrgImageBase - (UINTN)FileInfo.Buffer);

    //
    // Migrate PEI_CORE PPI pointers from temporary memory to newly
    // installed PEI_CORE in permanent memory.
    //
    ConvertPpiPointersFv (PrivateData, (UINTN)OrgImageBase, (UINTN)MigratedImageBase, PeiCoreModuleSize);
  }
}
