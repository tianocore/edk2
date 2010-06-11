/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  MemoryStatusCode.c
   
Abstract:

  Lib to provide memory journal status code reporting Routines.

--*/

#include "MemoryStatusCode.h"
#include "PeiLib.h"
#include "MonoStatusCode.h"

//
// Global variable.  Not accessible while running from flash.
// After we relocate ourselves into memory, we update this
// and use it to determine if we are running from flash or memory.
//
BOOLEAN                       mRunningFromMemory = FALSE;

//
// Global variable used to replace the PPI once we start running from memory.
//
PEI_STATUS_CODE_MEMORY_PPI    mStatusCodeMemoryPpi = { 0, 0, 0, 0 };

//
// PPI descriptor for the MonoStatusCode PEIM, see MonoStatusCode.c
//
extern EFI_PEI_PPI_DESCRIPTOR mPpiListStatusCode;

VOID
EFIAPI
MemoryInitializeStatusCode (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:

  Initialization routine.
  Allocates heap space for storing Status Codes.
  Installs a PPI to point to that heap space.
  Installs a callback to switch to memory.
  Installs a callback to 

Arguments: 

  FfsHeader   - FV this PEIM was loaded from.
  PeiServices - General purpose services available to every PEIM.

Returns: 

  None

--*/
{
  EFI_STATUS                  Status;
  MEMORY_STATUS_CODE_INSTANCE *PrivateData;
  PEI_STATUS_CODE_MEMORY_PPI  *StatusCodeMemoryPpi;
  PEI_STATUS_CODE_PPI         *ReportStatusCodePpi;
  EFI_PHYSICAL_ADDRESS        Buffer;
  VOID                        *StartPointer;
  UINTN                       Length;
  UINTN                       LastEntry;
  EFI_PEI_PPI_DESCRIPTOR      *ReportStatusCodeDescriptor;
  EFI_PEI_PPI_DESCRIPTOR      *StatusCodeMemoryDescriptor;

  //
  // Determine if we are being called after relocation into memory.
  //
  if (!mRunningFromMemory) {
    //
    // If we are not running from memory, we need to allocate some heap and
    // install the PPI
    //
    //
    // Allocate heap storage for the journal
    //
    Status = (*PeiServices)->AllocatePool (
                              PeiServices,
                              PEI_STATUS_CODE_HEAP_LENGTH,
                              (VOID **) &StartPointer
                              );

    //
    // This is not a required feature to boot.
    //
    if (EFI_ERROR (Status)) {
      return ;
    }
    //
    // Allocate heap storage for private data
    // The private data contains the FFS header for this PEIM,
    // a PPI containing information about the status code journal, and
    // a notification for the LoadFile service, to relocate the PEIM into
    // memory.
    //
    Status = (*PeiServices)->AllocatePool (
                              PeiServices,
                              sizeof (MEMORY_STATUS_CODE_INSTANCE),
                              (VOID **) &PrivateData
                              );

    //
    // This is not a required feature to boot.
    //
    if (EFI_ERROR (Status)) {
      return ;
    }
    //
    // Update the contents of the private data.
    //
    PrivateData->Signature                      = MEMORY_STATUS_CODE_SIGNATURE;
    PrivateData->This = PrivateData;
    PrivateData->FfsHeader = FfsHeader;
    PrivateData->PpiDescriptor.Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    PrivateData->PpiDescriptor.Guid = &gPeiStatusCodeMemoryPpiGuid;
    PrivateData->PpiDescriptor.Ppi = &PrivateData->StatusCodeMemoryPpi;
    PrivateData->StatusCodeMemoryPpi.FirstEntry = 0;
    PrivateData->StatusCodeMemoryPpi.LastEntry = 0;
    PrivateData->StatusCodeMemoryPpi.Address = (EFI_PHYSICAL_ADDRESS) (UINTN) StartPointer;
    PrivateData->StatusCodeMemoryPpi.Length = PEI_STATUS_CODE_HEAP_LENGTH;
#if (PI_SPECIFICATION_VERSION < 0x00010000)
    PrivateData->NotifyDescriptor.Flags =
      (
        EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK |
        EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST
      );
    PrivateData->NotifyDescriptor.Guid    = &gPeiFvFileLoaderPpiGuid;
    PrivateData->NotifyDescriptor.Notify  = LoadImageCallback;
#endif
    //
    // Publish the PPI
    //
    Status = (*PeiServices)->InstallPpi (PeiServices, &PrivateData->PpiDescriptor);
    if (EFI_ERROR (Status)) {
      return ;
    }
    //
    // Post a callback to relocate to memory
    //
#if (PI_SPECIFICATION_VERSION < 0x00010000)
    Status = (**PeiServices).NotifyPpi (PeiServices, &PrivateData->NotifyDescriptor);
    if (EFI_ERROR (Status)) {
      return ;
    }
#endif
  } else {
    //
    // If we are running from memory, we need to copy from the heap to a RT
    // memory buffer.
    //
    //
    // Locate Journal
    //
    Status = (*PeiServices)->LocatePpi (
                              PeiServices,
                              &gPeiStatusCodeMemoryPpiGuid,
                              0,
                              &StatusCodeMemoryDescriptor,
                              (VOID **) &StatusCodeMemoryPpi
                              );
    if (EFI_ERROR (Status)) {
      return ;
    }
    //
    // Get private data
    //
    PrivateData = MEMORY_STATUS_CODE_FROM_DESCRIPTOR_THIS (StatusCodeMemoryDescriptor);

    //
    // At this point, we need to fix up any addresses that we have as the heap
    // has moved.
    //
    PrivateData->PpiDescriptor.Ppi  = &PrivateData->StatusCodeMemoryPpi;
    PrivateData->PpiDescriptor.Guid = &gPeiStatusCodeMemoryPpiGuid;
    PrivateData->StatusCodeMemoryPpi.Address = PrivateData->StatusCodeMemoryPpi.Address +
      (UINTN) PrivateData - (UINTN) PrivateData->This;
    PrivateData->This                     = PrivateData;
#if (PI_SPECIFICATION_VERSION < 0x00010000)
    PrivateData->NotifyDescriptor.Guid    = &gPeiFvFileLoaderPpiGuid;
    PrivateData->NotifyDescriptor.Notify  = LoadImageCallback;
#endif

    //
    // Allocate RT memory.
    //
    Status = (*PeiServices)->AllocatePages (
                              PeiServices,
                              EfiRuntimeServicesData,
                              PEI_STATUS_CODE_RT_PAGES,
                              &Buffer
                              );
    if (EFI_ERROR (Status)) {
      return ;
    }

    DEBUG_CODE (
      EfiCommonLibZeroMem ((VOID *) (UINTN) Buffer, PEI_STATUS_CODE_RT_LENGTH);
    )
    //
    // Copy the heap to the allocated memory.
    // Unwind the rolling queue to start at 0 in the new space.  We need to do
    // this because the new queue is much bigger than the heap allocation.
    //
    if (PEI_STATUS_CODE_RT_LENGTH <= PEI_STATUS_CODE_HEAP_LENGTH) {
      return ;
    }

    if (StatusCodeMemoryPpi->LastEntry >= StatusCodeMemoryPpi->FirstEntry) {
      LastEntry = StatusCodeMemoryPpi->LastEntry - StatusCodeMemoryPpi->FirstEntry;
      StartPointer = (VOID *) ((UINTN) StatusCodeMemoryPpi->Address + (StatusCodeMemoryPpi->FirstEntry * sizeof (EFI_STATUS_CODE_ENTRY)));
      Length = (StatusCodeMemoryPpi->LastEntry - StatusCodeMemoryPpi->FirstEntry) * sizeof (EFI_STATUS_CODE_ENTRY);
      (*PeiServices)->CopyMem ((VOID *) (UINTN) Buffer, StartPointer, Length);
    } else {
      //
      // The last entry will be the new last entry after moving heap to buffer
      //
      LastEntry = (PEI_STATUS_CODE_MAX_HEAP_ENTRY - StatusCodeMemoryPpi->FirstEntry) + StatusCodeMemoryPpi->LastEntry;
      //
      // Copy from the first entry to the end of the heap
      //
      StartPointer = (VOID *) ((UINTN) StatusCodeMemoryPpi->Address + (StatusCodeMemoryPpi->FirstEntry * sizeof (EFI_STATUS_CODE_ENTRY)));
      Length = PEI_STATUS_CODE_HEAP_LENGTH - (StatusCodeMemoryPpi->FirstEntry * sizeof (EFI_STATUS_CODE_ENTRY));
      (*PeiServices)->CopyMem ((VOID *) (UINTN) Buffer, StartPointer, Length);
      //
      // Copy from the start to the heap to the last entry
      //
      StartPointer = (VOID *) (UINTN) StatusCodeMemoryPpi->Address;
      (*PeiServices)->CopyMem (
                        (VOID *) (UINTN) (Buffer + Length),
                        StartPointer,
                        (StatusCodeMemoryPpi->LastEntry * sizeof (EFI_STATUS_CODE_ENTRY))
                        );
    };

    //
    // Update the PPI to NULL, so it will not be used.
    //
    StatusCodeMemoryPpi->FirstEntry = 0;
    StatusCodeMemoryPpi->LastEntry  = 0;
    StatusCodeMemoryPpi->Address    = 0;
    StatusCodeMemoryPpi->Length     = 0;

    //
    // Update in memory version of PPI that will be used.
    //
    mStatusCodeMemoryPpi.FirstEntry = 0;
    mStatusCodeMemoryPpi.LastEntry  = LastEntry;
    mStatusCodeMemoryPpi.Address    = (EFI_PHYSICAL_ADDRESS) (UINTN) Buffer;
    mStatusCodeMemoryPpi.Length     = PEI_STATUS_CODE_RT_LENGTH;

    //
    // Reinstall the report status code function
    //
    //
    // Locate status code PPI
    //
    Status = (*PeiServices)->LocatePpi (
                              PeiServices,
                              &gPeiStatusCodePpiGuid,
                              0,
                              &ReportStatusCodeDescriptor,
                              (VOID **) &ReportStatusCodePpi
                              );
    if (EFI_ERROR (Status)) {
      return ;
    }
    //
    // Reinstall the ReportStatusCode interface using the memory-based
    // descriptor
    //
    Status = (*PeiServices)->ReInstallPpi (
                              PeiServices,
                              ReportStatusCodeDescriptor,
                              &mPpiListStatusCode
                              );
    if (EFI_ERROR (Status)) {
      EFI_DEADLOOP ();
    }
    //
    // Publish a GUIDed HOB that contains a pointer to the status code PPI
    // structure.  This is a bit of a short cut as I just used the PPI GUID to
    // identify the HOB.  This HOB is caught by the DXE status code memory
    // listener and used to find the journal.
    //
    StatusCodeMemoryPpi = &mStatusCodeMemoryPpi;
    Status = PeiBuildHobGuidData (
              PeiServices,
              &gPeiStatusCodeMemoryPpiGuid,
              &StatusCodeMemoryPpi,
              sizeof (VOID *)
              );
    if (EFI_ERROR (Status)) {
      EFI_DEADLOOP ();
    }
  }
}

EFI_STATUS
EFIAPI
MemoryReportStatusCode (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId ,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
/*++

Routine Description:

  Provide a memory status code

Arguments:

  Same as ReportStatusCode PPI
    
Returns:

  EFI_SUCCESS   This function always returns success

--*/
{
  EFI_STATUS                  Status;
  PEI_STATUS_CODE_MEMORY_PPI  *StatusCodeMemoryPpi;
  EFI_STATUS_CODE_ENTRY       *CurrentEntry;
  UINTN                       LastEntry;
  MEMORY_STATUS_CODE_INSTANCE *PrivateData;
  EFI_PEI_PPI_DESCRIPTOR      *StatusCodeMemoryDescriptor;

  //
  // We don't care to log debug codes.
  //
  if ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_DEBUG_CODE) {
    return EFI_SUCCESS;
  }

  if (!mRunningFromMemory) {
    //
    // If we are called from DXE and have not been reinstalled into memory, we
    // can no longer locate the journal, so we can no longer log status codes.
    //
    if (!PeiServices) {
      return EFI_SUCCESS;
    }
    //
    // Locate Journal
    //
    Status = (*PeiServices)->LocatePpi (
                              PeiServices,
                              &gPeiStatusCodeMemoryPpiGuid,
                              0,
                              &StatusCodeMemoryDescriptor,
                              (VOID **) &StatusCodeMemoryPpi
                              );
    if (EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
    //
    // Determine the last entry in the journal.
    // This is needed to properly implement the rolling queue.
    //
    LastEntry = PEI_STATUS_CODE_MAX_HEAP_ENTRY;

    //
    // Get private data
    //
    PrivateData = MEMORY_STATUS_CODE_FROM_DESCRIPTOR_THIS (StatusCodeMemoryDescriptor);

    //
    // Once memory gets installed, heap gets moved to real memory.
    // We need to fix up the pointers to match the move.
    //
    PrivateData->PpiDescriptor.Ppi  = &PrivateData->StatusCodeMemoryPpi;
    PrivateData->PpiDescriptor.Guid = &gPeiStatusCodeMemoryPpiGuid;
    PrivateData->StatusCodeMemoryPpi.Address = PrivateData->StatusCodeMemoryPpi.Address +
      (UINTN) PrivateData - (UINTN) PrivateData->This;
    PrivateData->This                     = PrivateData;
#if (PI_SPECIFICATION_VERSION < 0x00010000)
    PrivateData->NotifyDescriptor.Guid    = &gPeiFvFileLoaderPpiGuid;
    PrivateData->NotifyDescriptor.Notify  = LoadImageCallback;
#endif
    StatusCodeMemoryPpi                   = PrivateData->PpiDescriptor.Ppi;
  } else {
    //
    // Use global/memory copy of the PPI
    //
    StatusCodeMemoryPpi = &mStatusCodeMemoryPpi;

    //
    // Determine the last entry in the journal.
    // This is needed to properly implement the rolling queue.
    //
    LastEntry = PEI_STATUS_CODE_MAX_RT_ENTRY;
  }
  //
  // Return if we are using a cleared PPI somehow
  //
  if (!StatusCodeMemoryPpi->Address || !StatusCodeMemoryPpi->Length) {
    return EFI_SUCCESS;
  }
  //
  // Update the latest entry in the journal (may actually be first due to rolling
  // queue).
  //
  CurrentEntry = (EFI_STATUS_CODE_ENTRY *) (UINTN) (StatusCodeMemoryPpi->Address + (StatusCodeMemoryPpi->LastEntry * sizeof (EFI_STATUS_CODE_ENTRY)));

  StatusCodeMemoryPpi->LastEntry = (StatusCodeMemoryPpi->LastEntry + 1) % LastEntry;
  if (StatusCodeMemoryPpi->LastEntry == StatusCodeMemoryPpi->FirstEntry) {
    StatusCodeMemoryPpi->FirstEntry = (StatusCodeMemoryPpi->FirstEntry + 1) % LastEntry;
  }

  CurrentEntry->Type      = CodeType;
  CurrentEntry->Value     = Value;
  CurrentEntry->Instance  = Instance;

  return EFI_SUCCESS;
}


#if (PI_SPECIFICATION_VERSION < 0x00010000)

EFI_STATUS
EFIAPI
LoadImageCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
/*++

Routine Description:

  Relocate the PEIM into memory.

  Once load protocol becomes available, relocate our PEIM into memory.
  The primary benefit is to eliminate the blackout window that we would have in
  the memory log between the end of PEI and the status code DXE driver taking
  control.  If we don't do this, we cannot determine where our memory journal
  is located and cannot function.

  A second benefit is speed optimization throughout DXE.

Arguments:

  PeiServices      - General purpose services available to every PEIM.
  NotifyDescriptor - Information about the notify event.
  Ppi              - Context
    
Returns:

  EFI_SUCCESS   This function always returns success.

--*/
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        ImageAddress;
  EFI_PHYSICAL_ADDRESS        EntryPoint;
  UINT64                      ImageSize;
  MEMORY_STATUS_CODE_INSTANCE *PrivateData;

  //
  // Relocate to memory
  //
  if (!mRunningFromMemory) {
    //
    // Use the callback descriptor to get the FfsHeader
    //
    PrivateData = MEMORY_STATUS_CODE_FROM_NOTIFY_THIS (NotifyDescriptor);

    Status = ((EFI_PEI_FV_FILE_LOADER_PPI *) Ppi)->FvLoadFile (
                                                    Ppi,
                                                    PrivateData->FfsHeader,
                                                    &ImageAddress,
                                                    &ImageSize,
                                                    &EntryPoint
                                                    );
    if (EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
    //
    // Set the flag in the loaded image that indicates the PEIM is executing
    // from memory.
    //
#ifdef EFI_NT_EMULATOR
    //
    // For NT32, we should also relocate image here, because if the DLL
    // is already load, we will NOT load it twice. This feature is added to
    // prevent loading driver twice in DXE phase cause system crash.
    //
    * (BOOLEAN *) ((UINTN) &mRunningFromMemory + (UINTN) EntryPoint - (UINTN) InstallMonoStatusCode) = TRUE;
#else
    * (BOOLEAN *) ((UINTN) &mRunningFromMemory + (UINTN) EntryPoint - (UINTN) InstallMonoStatusCode) = TRUE;
#endif
    Status = ((EFI_PEIM_ENTRY_POINT )(UINTN) EntryPoint) (PrivateData->FfsHeader, PeiServices);
    if (EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
  }

  return EFI_SUCCESS;
}
#endif

