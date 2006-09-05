/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Ppi.c

Abstract:

  EFI PEI Core PPI services

Revision History

--*/

#include <PeiMain.h>

VOID
InitializePpiServices (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_CORE_INSTANCE *OldCoreData
  )
/*++

Routine Description:

  Initialize PPI services.

Arguments:

  PeiServices - The PEI core services table.
  OldCoreData - Pointer to the PEI Core data.
                NULL if being run in non-permament memory mode.

Returns:
  Nothing

--*/
{
  PEI_CORE_INSTANCE                    *PrivateData;
  
  if (OldCoreData == NULL) {
    PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);

    PrivateData->PpiData.NotifyListEnd = MAX_PPI_DESCRIPTORS-1;
    PrivateData->PpiData.DispatchListEnd = MAX_PPI_DESCRIPTORS-1;
    PrivateData->PpiData.LastDispatchedNotify = MAX_PPI_DESCRIPTORS-1;
  }
 
  return;   
}

VOID
ConvertPpiPointers (
  IN EFI_PEI_SERVICES                     **PeiServices,
  IN EFI_HOB_HANDOFF_INFO_TABLE    *OldHandOffHob,
  IN EFI_HOB_HANDOFF_INFO_TABLE    *NewHandOffHob
  )
/*++

Routine Description:

  Migrate the Hob list from the CAR stack to PEI installed memory.

Arguments:

  PeiServices   - The PEI core services table.
  OldHandOffHob - The old handoff HOB list.
  NewHandOffHob - The new handoff HOB list.

Returns:
            
--*/
{
  PEI_CORE_INSTANCE     *PrivateData;
  UINT8                 Index;
  PEI_PPI_LIST_POINTERS *PpiPointer;
  UINTN                 Fixup;

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);

  Fixup = (UINTN)NewHandOffHob - (UINTN)OldHandOffHob;
  
  for (Index = 0; Index < MAX_PPI_DESCRIPTORS; Index++) {
    if (Index < PrivateData->PpiData.PpiListEnd ||
        Index > PrivateData->PpiData.NotifyListEnd) {
      PpiPointer = &PrivateData->PpiData.PpiListPtrs[Index];
      
      if (((UINTN)PpiPointer->Raw < (UINTN)OldHandOffHob->EfiFreeMemoryBottom) && 
          ((UINTN)PpiPointer->Raw >= (UINTN)OldHandOffHob)) {
        //
        // Convert the pointer to the PEIM descriptor from the old HOB heap
        // to the relocated HOB heap.
        //
        PpiPointer->Raw = (VOID *) ((UINTN)PpiPointer->Raw + Fixup);

        //
        // Only when the PEIM descriptor is in the old HOB should it be necessary
        // to try to convert the pointers in the PEIM descriptor
        //
        
        if (((UINTN)PpiPointer->Ppi->Guid < (UINTN)OldHandOffHob->EfiFreeMemoryBottom) && 
            ((UINTN)PpiPointer->Ppi->Guid >= (UINTN)OldHandOffHob)) {
          //
          // Convert the pointer to the GUID in the PPI or NOTIFY descriptor
          // from the old HOB heap to the relocated HOB heap.
          //
          PpiPointer->Ppi->Guid = (VOID *) ((UINTN)PpiPointer->Ppi->Guid + Fixup);
        }

        //
        // Assume that no code is located in the temporary memory, so the pointer to
        // the notification function in the NOTIFY descriptor needs not be converted.
        //
        if (Index < PrivateData->PpiData.PpiListEnd &&
            (UINTN)PpiPointer->Ppi->Ppi < (UINTN)OldHandOffHob->EfiFreeMemoryBottom &&
            (UINTN)PpiPointer->Ppi->Ppi >= (UINTN)OldHandOffHob) {
            //
            // Convert the pointer to the PPI interface structure in the PPI descriptor
            // from the old HOB heap to the relocated HOB heap.
            //
            PpiPointer->Ppi->Ppi = (VOID *) ((UINTN)PpiPointer->Ppi->Ppi+ Fixup);   
        }
      }
    }
  }
}



EFI_STATUS
EFIAPI
PeiInstallPpi (
  IN EFI_PEI_SERVICES        **PeiServices,
  IN EFI_PEI_PPI_DESCRIPTOR  *PpiList
  )
/*++

Routine Description:

  Install PPI services.

Arguments:

  PeiServices - Pointer to the PEI Service Table
  PpiList     - Pointer to a list of PEI PPI Descriptors.

Returns:

    EFI_SUCCESS             - if all PPIs in PpiList are successfully installed.
    EFI_INVALID_PARAMETER   - if PpiList is NULL pointer
    EFI_INVALID_PARAMETER   - if any PPI in PpiList is not valid
    EFI_OUT_OF_RESOURCES    - if there is no more memory resource to install PPI

--*/
{
  PEI_CORE_INSTANCE *PrivateData;
  INTN              Index;
  INTN              LastCallbackInstall;


  if (PpiList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);

  Index = PrivateData->PpiData.PpiListEnd;
  LastCallbackInstall = Index;

  //
  // This is loop installs all PPI descriptors in the PpiList.  It is terminated
  // by the EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST being set in the last
  // EFI_PEI_PPI_DESCRIPTOR in the list.
  //
    
  for (;;) {
    //
    // Since PpiData is used for NotifyList and InstallList, max resource
    // is reached if the Install reaches the NotifyList
    //
    if (Index == PrivateData->PpiData.NotifyListEnd + 1) {
      return  EFI_OUT_OF_RESOURCES;
    }
    //
    // Check if it is a valid PPI. 
    // If not, rollback list to exclude all in this list.
    // Try to indicate which item failed.
    //
    if ((PpiList->Flags & EFI_PEI_PPI_DESCRIPTOR_PPI) == 0) {
      PrivateData->PpiData.PpiListEnd = LastCallbackInstall;
      DEBUG((EFI_D_ERROR, "ERROR -> InstallPpi: %g %x\n", PpiList->Guid, PpiList->Ppi));
      return  EFI_INVALID_PARAMETER;
    }

    DEBUG((EFI_D_INFO, "Install PPI: %g\n", PpiList->Guid)); 
    PrivateData->PpiData.PpiListPtrs[Index].Ppi = PpiList;    
    PrivateData->PpiData.PpiListEnd++;
    
    //
    // Continue until the end of the PPI List.
    //
    if ((PpiList->Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) ==  
        EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) {
      break;
    }
    PpiList++;
    Index++;
  }

  //
  // Dispatch any callback level notifies for newly installed PPIs.
  //
  DispatchNotify (
    PeiServices,
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    LastCallbackInstall,
    PrivateData->PpiData.PpiListEnd,
    PrivateData->PpiData.DispatchListEnd,                 
    PrivateData->PpiData.NotifyListEnd
    );


  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
PeiReInstallPpi (
  IN EFI_PEI_SERVICES        **PeiServices,
  IN EFI_PEI_PPI_DESCRIPTOR  *OldPpi,
  IN EFI_PEI_PPI_DESCRIPTOR  *NewPpi
  )
/*++

Routine Description:

  Re-Install PPI services.

Arguments:

  PeiServices - Pointer to the PEI Service Table
  OldPpi      - Pointer to the old PEI PPI Descriptors.
  NewPpi      - Pointer to the new PEI PPI Descriptors.

Returns:

  EFI_SUCCESS           - if the operation was successful
  EFI_INVALID_PARAMETER - if OldPpi or NewPpi is NULL
  EFI_INVALID_PARAMETER - if NewPpi is not valid
  EFI_NOT_FOUND         - if the PPI was not in the database

--*/
{
  PEI_CORE_INSTANCE   *PrivateData;
  INTN                Index;


  if ((OldPpi == NULL) || (NewPpi == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((NewPpi->Flags & EFI_PEI_PPI_DESCRIPTOR_PPI) == 0) {
    return  EFI_INVALID_PARAMETER;
  }

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);

  //
  // Find the old PPI instance in the database.  If we can not find it,
  // return the EFI_NOT_FOUND error.
  //
  for (Index = 0; Index < PrivateData->PpiData.PpiListEnd; Index++) {
    if (OldPpi == PrivateData->PpiData.PpiListPtrs[Index].Ppi) {
      break;
    }
  }
  if (Index == PrivateData->PpiData.PpiListEnd) {
    return EFI_NOT_FOUND;
  }

  //
  // Remove the old PPI from the database, add the new one.
  // 
  DEBUG((EFI_D_INFO, "Reinstall PPI: %g\n", NewPpi->Guid));
  PrivateData->PpiData.PpiListPtrs[Index].Ppi = NewPpi;

  //
  // Dispatch any callback level notifies for the newly installed PPI.
  //
  DispatchNotify (
    PeiServices,
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    Index,
    Index+1,
    PrivateData->PpiData.DispatchListEnd,                 
    PrivateData->PpiData.NotifyListEnd
    );


  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
PeiLocatePpi (
  IN EFI_PEI_SERVICES        **PeiServices,
  IN EFI_GUID                *Guid,
  IN UINTN                   Instance,
  IN OUT EFI_PEI_PPI_DESCRIPTOR  **PpiDescriptor,
  IN OUT VOID                **Ppi
  )
/*++

Routine Description:

  Locate a given named PPI.

Arguments:

  PeiServices   - Pointer to the PEI Service Table
  Guid          - Pointer to GUID of the PPI.
  Instance      - Instance Number to discover.
  PpiDescriptor - Pointer to reference the found descriptor. If not NULL,
                returns a pointer to the descriptor (includes flags, etc)
  Ppi           - Pointer to reference the found PPI

Returns:

  Status -  EFI_SUCCESS   if the PPI is in the database           
            EFI_NOT_FOUND if the PPI is not in the database
--*/
{
  PEI_CORE_INSTANCE   *PrivateData;
  INTN                Index;
  EFI_GUID            *CheckGuid;
  EFI_PEI_PPI_DESCRIPTOR  *TempPtr;

  
  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);

  //
  // Search the data base for the matching instance of the GUIDed PPI.
  //
  for (Index = 0; Index < PrivateData->PpiData.PpiListEnd; Index++) {
    TempPtr = PrivateData->PpiData.PpiListPtrs[Index].Ppi;
    CheckGuid = TempPtr->Guid;

    //
    // Don't use CompareGuid function here for performance reasons.
    // Instead we compare the GUID as INT32 at a time and branch
    // on the first failed comparison.
    //
    if ((((INT32 *)Guid)[0] == ((INT32 *)CheckGuid)[0]) &&
        (((INT32 *)Guid)[1] == ((INT32 *)CheckGuid)[1]) &&
        (((INT32 *)Guid)[2] == ((INT32 *)CheckGuid)[2]) &&
        (((INT32 *)Guid)[3] == ((INT32 *)CheckGuid)[3])) {
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


EFI_STATUS
EFIAPI
PeiNotifyPpi (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyList
  )
/*++

Routine Description:

  Install a notification for a given PPI.

Arguments:

  PeiServices - Pointer to the PEI Service Table
  NotifyList  - Pointer to list of Descriptors to notify upon.

Returns:

  Status - EFI_SUCCESS           if successful
           EFI_OUT_OF_RESOURCES  if no space in the database
           EFI_INVALID_PARAMETER if not a good decriptor

--*/
{
  PEI_CORE_INSTANCE                *PrivateData;
  INTN                             Index;
  INTN                             NotifyIndex;
  INTN                             LastCallbackNotify;
  EFI_PEI_NOTIFY_DESCRIPTOR        *NotifyPtr;
  UINTN                            NotifyDispatchCount;


  NotifyDispatchCount = 0;

  if (NotifyList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);

  Index = PrivateData->PpiData.NotifyListEnd;
  LastCallbackNotify = Index;

  //
  // This is loop installs all Notify descriptors in the NotifyList.  It is
  // terminated by the EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST being set in the last
  // EFI_PEI_NOTIFY_DESCRIPTOR in the list.
  //

  for (;;) {
    //
    // Since PpiData is used for NotifyList and InstallList, max resource
    // is reached if the Install reaches the PpiList
    //
    if (Index == PrivateData->PpiData.PpiListEnd - 1) {
      return  EFI_OUT_OF_RESOURCES;
    }
    
    //
    // If some of the PPI data is invalid restore original Notify PPI database value
    //
    if ((NotifyList->Flags & EFI_PEI_PPI_DESCRIPTOR_NOTIFY_TYPES) == 0) {
        PrivateData->PpiData.NotifyListEnd = LastCallbackNotify;
        DEBUG((EFI_D_ERROR, "ERROR -> InstallNotify: %g %x\n", NotifyList->Guid, NotifyList->Notify));
      return  EFI_INVALID_PARAMETER;
    }
     
    if ((NotifyList->Flags & EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH) != 0) {
      NotifyDispatchCount ++; 
    }        
    
    PrivateData->PpiData.PpiListPtrs[Index].Notify = NotifyList;      
   
    PrivateData->PpiData.NotifyListEnd--;
    DEBUG((EFI_D_INFO, "Register PPI Notify: %g\n", NotifyList->Guid));
    if ((NotifyList->Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) ==
        EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) {
      break;
    }
    //
    // Go the next descriptor. Remember the NotifyList moves down.
    //
    NotifyList++;
    Index--;
  }
 
  //
  // If there is Dispatch Notify PPI installed put them on the bottom 
  //
  if (NotifyDispatchCount > 0) {
    for (NotifyIndex = LastCallbackNotify; NotifyIndex > PrivateData->PpiData.NotifyListEnd; NotifyIndex--) {             
      if ((PrivateData->PpiData.PpiListPtrs[NotifyIndex].Notify->Flags & EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH) != 0) {
        NotifyPtr = PrivateData->PpiData.PpiListPtrs[NotifyIndex].Notify;
        
        for (Index = NotifyIndex; Index < PrivateData->PpiData.DispatchListEnd; Index++){
          PrivateData->PpiData.PpiListPtrs[Index].Notify = PrivateData->PpiData.PpiListPtrs[Index + 1].Notify;
        }
        PrivateData->PpiData.PpiListPtrs[Index].Notify = NotifyPtr;
        PrivateData->PpiData.DispatchListEnd--;                
      }
    }
    
    LastCallbackNotify -= NotifyDispatchCount;        
  }
  
  //
  // Dispatch any callback level notifies for all previously installed PPIs.
  //
  DispatchNotify (
    PeiServices,
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    0,
    PrivateData->PpiData.PpiListEnd,
    LastCallbackNotify,
    PrivateData->PpiData.NotifyListEnd
    );
  
  
  return  EFI_SUCCESS;
}


VOID
ProcessNotifyList (
  IN EFI_PEI_SERVICES    **PeiServices
  )
/*++

Routine Description:

  Process the Notify List at dispatch level.

Arguments:

  PeiServices - Pointer to the PEI Service Table

Returns:

--*/

{
  PEI_CORE_INSTANCE       *PrivateData;
  INTN                    TempValue;

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);

 
  while (TRUE) {
    //
    // Check if the PEIM that was just dispatched resulted in any
    // Notifies getting installed.  If so, go process any dispatch
    // level Notifies that match the previouly installed PPIs.
    // Use "while" instead of "if" since DispatchNotify can modify 
    // DispatchListEnd (with NotifyPpi) so we have to iterate until the same.
    //
    while (PrivateData->PpiData.LastDispatchedNotify != PrivateData->PpiData.DispatchListEnd) {
      TempValue = PrivateData->PpiData.DispatchListEnd;
      DispatchNotify (
        PeiServices,
        EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH,
        0,
        PrivateData->PpiData.LastDispatchedInstall,
        PrivateData->PpiData.LastDispatchedNotify,
        PrivateData->PpiData.DispatchListEnd
        );
      PrivateData->PpiData.LastDispatchedNotify = TempValue;
    }
    
    
    //
    // Check if the PEIM that was just dispatched resulted in any
    // PPIs getting installed.  If so, go process any dispatch
    // level Notifies that match the installed PPIs.
    // Use "while" instead of "if" since DispatchNotify can modify 
    // PpiListEnd (with InstallPpi) so we have to iterate until the same.
    //
    while (PrivateData->PpiData.LastDispatchedInstall != PrivateData->PpiData.PpiListEnd) {
      TempValue = PrivateData->PpiData.PpiListEnd;
      DispatchNotify (
        PeiServices,
        EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH,
        PrivateData->PpiData.LastDispatchedInstall,
        PrivateData->PpiData.PpiListEnd,
        MAX_PPI_DESCRIPTORS-1,
        PrivateData->PpiData.DispatchListEnd
        );
      PrivateData->PpiData.LastDispatchedInstall = TempValue;
    }
    
    if (PrivateData->PpiData.LastDispatchedNotify == PrivateData->PpiData.DispatchListEnd) {
      break;
    }
  } 
  return;
}

VOID
DispatchNotify (
  IN EFI_PEI_SERVICES    **PeiServices,
  IN UINTN               NotifyType,
  IN INTN                InstallStartIndex,
  IN INTN                InstallStopIndex,
  IN INTN                NotifyStartIndex,
  IN INTN                NotifyStopIndex
  )
/*++

Routine Description:

  Dispatch notifications.

Arguments:

  PeiServices         - Pointer to the PEI Service Table
  NotifyType          - Type of notify to fire.
  InstallStartIndex   - Install Beginning index.
  InstallStopIndex    - Install Ending index.
  NotifyStartIndex    - Notify Beginning index.
  NotifyStopIndex    - Notify Ending index.

Returns:  None

--*/

{
  PEI_CORE_INSTANCE       *PrivateData;
  INTN                   Index1;
  INTN                   Index2;
  EFI_GUID                *SearchGuid;
  EFI_GUID                *CheckGuid;
  EFI_PEI_NOTIFY_DESCRIPTOR   *NotifyDescriptor;

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);

  //
  // Remember that Installs moves up and Notifies moves down.
  //
  for (Index1 = NotifyStartIndex; Index1 > NotifyStopIndex; Index1--) {
    NotifyDescriptor = PrivateData->PpiData.PpiListPtrs[Index1].Notify;

    CheckGuid = NotifyDescriptor->Guid;

    for (Index2 = InstallStartIndex; Index2 < InstallStopIndex; Index2++) {
      SearchGuid = PrivateData->PpiData.PpiListPtrs[Index2].Ppi->Guid;
      //
      // Don't use CompareGuid function here for performance reasons.
      // Instead we compare the GUID as INT32 at a time and branch
      // on the first failed comparison.
      //
      if ((((INT32 *)SearchGuid)[0] == ((INT32 *)CheckGuid)[0]) &&
          (((INT32 *)SearchGuid)[1] == ((INT32 *)CheckGuid)[1]) &&
          (((INT32 *)SearchGuid)[2] == ((INT32 *)CheckGuid)[2]) &&
          (((INT32 *)SearchGuid)[3] == ((INT32 *)CheckGuid)[3])) {
        DEBUG ((EFI_D_INFO, "Notify: PPI Guid: %g, Peim notify entry point: %x\n", 
          SearchGuid, 
          NotifyDescriptor->Notify
          ));
        NotifyDescriptor->Notify (
                            PeiServices,
                            NotifyDescriptor,
                            (PrivateData->PpiData.PpiListPtrs[Index2].Ppi)->Ppi
                            );
      }
    }
  }

  return;
}

