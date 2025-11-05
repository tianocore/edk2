/** @file
  This file implements some PEI services about PPI.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UnitTestPeiServicesTablePointerLib.h"

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
      // Run out of room, assert.
      //
      ASSERT (Index < PpiListPointer->MaxCount);
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
UnitTestInstallPpi (
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
UnitTestReInstallPpi (
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
UnitTestLocatePpi (
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
        // Run out of room, assert.
        //
        ASSERT (CallbackNotifyIndex < CallbackNotifyListPointer->MaxCount);
      }

      CallbackNotifyListPointer->NotifyPtrs[CallbackNotifyIndex].Notify = (EFI_PEI_NOTIFY_DESCRIPTOR *)NotifyList;
      CallbackNotifyIndex++;
      CallbackNotifyListPointer->CurrentCount++;
    } else {
      ASSERT ((NotifyList->Flags & EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK) != 0);
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
UnitTestNotifyPpi (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN CONST EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyList
  )
{
  return InternalPeiNotifyPpi (PeiServices, NotifyList, FALSE);
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
