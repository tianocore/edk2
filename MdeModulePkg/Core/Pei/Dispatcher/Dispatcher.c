/** @file
  EFI PEI Core dispatch services

Copyright (c) 2006 - 2024, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiMain.h"

//
// Utility global variables
//

/**
  DelayedDispatchDispatcher

  Delayed Dispach cycle (ie one pass) through each entry, calling functions when their
  time has expired.  When DelayedGroupId is specified, if there are any of the specified entries
  in the dispatch queue during dispatch, repeat the DelayedDispatch cycle.

  @param DelayedDispatchTable  Pointer to dispatch table
  @param OPTIONAL              DelayedGroupId used to insure particular time is met.

  @return BOOLEAN
**/
BOOLEAN
DelayedDispatchDispatcher (
  IN DELAYED_DISPATCH_TABLE  *DelayedDispatchTable,
  IN EFI_GUID                *DelayedGroupId  OPTIONAL
  );

/**
  DelayedDispatch End of PEI callback function. Insure that all of the delayed dispatch
  entries are complete before exiting PEI.

  @param[in] PeiServices   - Pointer to PEI Services Table.
  @param[in] NotifyDesc    - Pointer to the descriptor for the Notification event that
                             caused this function to execute.
  @param[in] Ppi           - Pointer to the PPI data associated with this function.

  @retval EFI_STATUS       - Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
PeiDelayedDispatchOnEndOfPei (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN VOID                       *Ppi
  );

EFI_DELAYED_DISPATCH_PPI  mDelayedDispatchPpi  = { PeiDelayedDispatchRegister, PeiDelayedDispatchWaitOnEvent };
EFI_PEI_PPI_DESCRIPTOR    mDelayedDispatchDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiDelayedDispatchPpiGuid,
  &mDelayedDispatchPpi
};

EFI_PEI_NOTIFY_DESCRIPTOR  mDelayedDispatchNotifyDesc = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEfiEndOfPeiSignalPpiGuid,
  PeiDelayedDispatchOnEndOfPei
};

/**
  Helper function to look up DELAYED_DISPATCH_TABLE published in HOB.

  @return Pointer to DELAYED_DISPATCH_TABLE from HOB
**/
DELAYED_DISPATCH_TABLE *
GetDelayedDispatchTable (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = GetFirstGuidHob (&gEfiDelayedDispatchTableGuid);
  if (GuidHob == NULL) {
    // There is something off about the build if this happens. We do want to
    // assert here to catch it during development.
    DEBUG ((DEBUG_ERROR, "%a - Delayed Dispatch Hob not available.\n", __func__));
    ASSERT (FALSE);
    return NULL;
  }

  return (DELAYED_DISPATCH_TABLE *)GET_GUID_HOB_DATA (GuidHob);
}

/**
  Register a callback to be called after a minimum delay has occurred.

  This service is the single member function of the EFI_DELAYED_DISPATCH_PPI

  @param[in] This           Pointer to the EFI_DELAYED_DISPATCH_PPI instance
  @param[in] Function       Function to call back
  @param[in] Context        Context data
  @param[in] DelayedGroupId GUID for this Delayed Dispatch request.
  @param[in] Delay          Delay interval

  @retval EFI_SUCCESS               Function successfully loaded
  @retval EFI_INVALID_PARAMETER     One of the Arguments is not supported
  @retval EFI_OUT_OF_RESOURCES      No more entries

**/
EFI_STATUS
EFIAPI
PeiDelayedDispatchRegister (
  IN  EFI_DELAYED_DISPATCH_PPI       *This,
  IN  EFI_DELAYED_DISPATCH_FUNCTION  Function,
  IN  UINT64                         Context,
  IN  EFI_GUID                       *DelayedGroupId   OPTIONAL,
  IN  UINT32                         Delay
  )
{
  DELAYED_DISPATCH_TABLE  *DelayedDispatchTable;
  DELAYED_DISPATCH_ENTRY  *Entry;
  EFI_STATUS              Status;

  // Check input parameters
  if ((Function == NULL) || (Delay > FixedPcdGet32 (PcdDelayedDispatchMaxDelayUs)) || (This == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a Invalid parameter. Function: %Lx, Delay: %u, This: %p\n", __func__, (UINT64)(UINTN)Function, Delay, This));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Get delayed dispatch table
  DelayedDispatchTable = GetDelayedDispatchTable ();
  if (DelayedDispatchTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a Unable to locate dispatch table\n", __func__));
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  // Check for available entry slots
  ASSERT (DelayedDispatchTable->Count <= DELAYED_DISPATCH_MAX_ENTRIES);
  if (DelayedDispatchTable->Count == DELAYED_DISPATCH_MAX_ENTRIES) {
    DEBUG ((DEBUG_ERROR, "%a Too many entries requested\n", __func__));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  Entry           = &DelayedDispatchTable->Entry[DelayedDispatchTable->Count];
  Entry->Function = Function;
  Entry->Context  = Context;
  Status          = SafeUint64Add (GET_TIME_IN_US (), Delay, &Entry->DispatchTime);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Delay overflow\n", __func__));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (DelayedGroupId == NULL) {
    ZeroMem (&Entry->DelayedGroupId, sizeof (EFI_GUID));
  } else {
    CopyGuid (&Entry->DelayedGroupId, DelayedGroupId);
  }

  Entry->MicrosecondDelay = Delay;
  DelayedDispatchTable->Count++;

  DEBUG ((DEBUG_INFO, "%a  Adding dispatch Entry\n", __func__));
  DEBUG ((DEBUG_INFO, "    Requested Delay = %d\n", Delay));
  DEBUG ((DEBUG_INFO, "    Trigger Time = %d\n", Entry->DispatchTime));
  DEBUG ((DEBUG_INFO, "    Context = 0x%016lx\n", Entry->Context));
  DEBUG ((DEBUG_INFO, "    Function = %Lx\n", (UINT64)(UINTN)Entry->Function));
  DEBUG ((DEBUG_INFO, "    DelayedGroupId = %g\n", &Entry->DelayedGroupId));

  if (Delay == 0) {
    // Force early dispatch point
    DelayedDispatchDispatcher (DelayedDispatchTable, NULL);
  }

  Status = EFI_SUCCESS;

Exit:
  return Status;
}

/**
  DelayedDispatchDispatcher

  Delayed Dispach cycle (ie one pass) through each entry, calling functions when their
  time has expired.  When DelayedGroupId is specified, if there are any of the specified entries
  in the dispatch queue during dispatch, repeat the DelayedDispatch cycle.

  @param DelayedDispatchTable  Pointer to dispatch table
  @param OPTIONAL              DelayedGroupId used to insure particular time is met.

  @return BOOLEAN
**/
BOOLEAN
DelayedDispatchDispatcher (
  IN DELAYED_DISPATCH_TABLE  *DelayedDispatchTable,
  IN EFI_GUID                *DelayedGroupId           OPTIONAL
  )
{
  BOOLEAN                 Dispatched;
  UINT64                  TimeCurrent;
  UINT64                  MaxDispatchTime;
  UINTN                   Index1;
  BOOLEAN                 DelayedGroupIdPresent;
  DELAYED_DISPATCH_ENTRY  *Entry;
  EFI_STATUS              Status;

  Dispatched            = FALSE;
  DelayedGroupIdPresent = TRUE;
  Status                = SafeUint64Add (GET_TIME_IN_US (), FixedPcdGet32 (PcdDelayedDispatchCompletionTimeoutUs), &MaxDispatchTime);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Delay overflow\n", __func__));
    return FALSE;
  }

  while ((DelayedDispatchTable->Count > 0) && (DelayedGroupIdPresent)) {
    DelayedGroupIdPresent = FALSE;
    DelayedDispatchTable->DispCount++;

    // If dispatching is messed up, clear DelayedDispatchTable and exit.
    TimeCurrent =  GET_TIME_IN_US ();
    if (TimeCurrent > MaxDispatchTime) {
      DEBUG ((DEBUG_ERROR, "%a - DelayedDispatch Completion timeout!\n", __func__));
      ReportStatusCode ((EFI_ERROR_MAJOR | EFI_ERROR_CODE), (EFI_SOFTWARE_PEI_CORE | EFI_SW_EC_ABORTED));
      ASSERT (FALSE);
      DelayedDispatchTable->Count = 0;
      break;
    }

    // Check each entry in the table for possible dispatch
    for (Index1 = 0; Index1 < DelayedDispatchTable->Count;) {
      Entry = &DelayedDispatchTable->Entry[Index1];
      // If DelayedGroupId is present, insure there is an additional check of the table.
      if (DelayedGroupId != NULL) {
        if (CompareGuid (DelayedGroupId, &Entry->DelayedGroupId)) {
          DelayedGroupIdPresent = TRUE;
        }
      }

      TimeCurrent =  GET_TIME_IN_US ();
      if (TimeCurrent >= Entry->DispatchTime) {
        // Time expired, invoked the function
        DEBUG ((
          DEBUG_ERROR,
          "Delayed dispatch entry %d @ %p, Target=%d, Act=%d Disp=%d\n",
          Index1,
          Entry->Function,
          Entry->DispatchTime,
          TimeCurrent,
          DelayedDispatchTable->DispCount
          ));
        Dispatched              = TRUE;
        Entry->MicrosecondDelay = 0;
        Entry->Function (
                 &Entry->Context,
                 &Entry->MicrosecondDelay
                 );
        DEBUG ((DEBUG_ERROR, "Delayed dispatch Function returned delay=%d\n", Entry->MicrosecondDelay));
        if (Entry->MicrosecondDelay == 0) {
          // NewTime = 0 = delete this entry from the table
          DelayedDispatchTable->Count--;
          CopyMem (Entry, Entry+1, sizeof (DELAYED_DISPATCH_ENTRY) * (DelayedDispatchTable->Count - Index1));
        } else {
          if (Entry->MicrosecondDelay > FixedPcdGet32 (PcdDelayedDispatchMaxDelayUs)) {
            DEBUG ((DEBUG_ERROR, "%a Illegal new delay %d requested\n", __func__, Entry->MicrosecondDelay));
            ASSERT (FALSE);
            Entry->MicrosecondDelay = FixedPcdGet32 (PcdDelayedDispatchMaxDelayUs);
          }

          // NewTime != 0 - update the time from us to Dispatch time
          Status = SafeUint64Add (GET_TIME_IN_US (), Entry->MicrosecondDelay, &Entry->DispatchTime);
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "%a Delay overflow, this event will likely never be fired...\n", __func__));
            Entry->DispatchTime = MAX_UINT64;
          }

          Index1++;
        }
      } else {
        Index1++;
      }
    }
  }

  return Dispatched;
}

/**
  Wait on a registered Delayed Dispatch unit that has a DelayedGroupId. Continue
  to dispatch all registered delayed dispatch entries until *ALL* entries with
  DelayedGroupId have completed.

  Example usage:
  1. Register a Delayed Dispatch entry with a DelayedGroupId.
  2. Call this function with the DelayedGroupId
  3. The registered function in #1 will be called after the specified delay.
  4. This function will wait until all entries with the DelayedGroupId have completed.

  @param[in]  This            The Delayed Dispatch PPI pointer.
  @param[in]  DelayedGroupId  Delayed dispatch request ID the caller will wait on

  @retval EFI_SUCCESS            The operation succeeds.
  @retval EFI_INVALID_PARAMETER  The parameters are invalid.

**/
EFI_STATUS
EFIAPI
PeiDelayedDispatchWaitOnEvent (
  IN  EFI_DELAYED_DISPATCH_PPI  *This,
  IN  EFI_GUID                  DelayedGroupId
  )
{
  PERF_FUNCTION_BEGIN ();
  EFI_STATUS              Status;
  DELAYED_DISPATCH_TABLE  *DelayedDispatchTable;

  // Get delayed dispatch table
  DelayedDispatchTable = GetDelayedDispatchTable ();
  if (DelayedDispatchTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a Unable to locate dispatch table\n", __func__));
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  if (IsZeroGuid (&DelayedGroupId)) {
    DEBUG ((DEBUG_ERROR, "%a Delayed Group ID is a null GUID\n", __func__));
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  DEBUG ((DEBUG_INFO, "Delayed dispatch on %g. Count=%d, DispatchCount=%d\n", &DelayedGroupId, DelayedDispatchTable->Count, DelayedDispatchTable->DispCount));
  PERF_EVENT_SIGNAL_BEGIN (&DelayedGroupId);
  DelayedDispatchDispatcher (DelayedDispatchTable, &DelayedGroupId);
  PERF_EVENT_SIGNAL_END (&DelayedGroupId);

  Status = EFI_SUCCESS;

Exit:
  PERF_FUNCTION_END ();
  return Status;
}

/**
  DelayedDispatch End of PEI callback function. Insure that all of the delayed dispatch
  entries are complete before exiting PEI.

  @param[in] PeiServices   - Pointer to PEI Services Table.
  @param[in] NotifyDesc    - Pointer to the descriptor for the Notification event that
                             caused this function to execute.
  @param[in] Ppi           - Pointer to the PPI data associated with this function.

  @retval EFI_STATUS       - Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
PeiDelayedDispatchOnEndOfPei (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDesc,
  IN VOID                       *Ppi
  )
{
  DELAYED_DISPATCH_TABLE  *DelayedDispatchTable;

  // Get delayed dispatch table
  DelayedDispatchTable = GetDelayedDispatchTable ();
  if (DelayedDispatchTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a Unable to locate dispatch table\n", __func__));
    return EFI_UNSUPPORTED;
  }

  PERF_INMODULE_BEGIN ("PerfDelayedDispatchEndOfPei");
  while (DelayedDispatchTable->Count > 0) {
    DelayedDispatchDispatcher (DelayedDispatchTable, NULL);
  }

  DEBUG ((DEBUG_ERROR, "%a Count of dispatch cycles is %d\n", __func__, DelayedDispatchTable->DispCount));
  PERF_INMODULE_END ("PerfDelayedDispatchEndOfPei");

  return EFI_SUCCESS;
}

/**
  Discover all PEIMs and optional Apriori file in one FV. There is at most one
  Apriori file in one FV.


  @param Private          Pointer to the private data passed in from caller
  @param CoreFileHandle   The instance of PEI_CORE_FV_HANDLE.

**/
VOID
DiscoverPeimsAndOrderWithApriori (
  IN  PEI_CORE_INSTANCE   *Private,
  IN  PEI_CORE_FV_HANDLE  *CoreFileHandle
  )
{
  EFI_STATUS                   Status;
  EFI_PEI_FILE_HANDLE          FileHandle;
  EFI_PEI_FILE_HANDLE          AprioriFileHandle;
  EFI_GUID                     *Apriori;
  UINTN                        Index;
  UINTN                        Index2;
  UINTN                        PeimIndex;
  UINTN                        PeimCount;
  EFI_GUID                     *Guid;
  EFI_PEI_FILE_HANDLE          *TempFileHandles;
  EFI_GUID                     *TempFileGuid;
  EFI_PEI_FIRMWARE_VOLUME_PPI  *FvPpi;
  EFI_FV_FILE_INFO             FileInfo;

  FvPpi = CoreFileHandle->FvPpi;

  //
  // Walk the FV and find all the PEIMs and the Apriori file.
  //
  AprioriFileHandle             = NULL;
  Private->CurrentFvFileHandles = NULL;
  Guid                          = NULL;

  //
  // If the current FV has been scanned, directly get its cached records.
  //
  if (CoreFileHandle->ScanFv) {
    Private->CurrentFvFileHandles = CoreFileHandle->FvFileHandles;
    return;
  }

  TempFileHandles = Private->TempFileHandles;
  TempFileGuid    = Private->TempFileGuid;

  //
  // Go ahead to scan this FV, get PeimCount and cache FileHandles within it to TempFileHandles.
  //
  PeimCount  = 0;
  FileHandle = NULL;
  do {
    Status = FvPpi->FindFileByType (FvPpi, PEI_CORE_INTERNAL_FFS_FILE_DISPATCH_TYPE, CoreFileHandle->FvHandle, &FileHandle);
    if (!EFI_ERROR (Status)) {
      if (PeimCount >= Private->TempPeimCount) {
        //
        // Run out of room, grow the buffer.
        //
        TempFileHandles = AllocatePool (
                            sizeof (EFI_PEI_FILE_HANDLE) * (Private->TempPeimCount + TEMP_FILE_GROWTH_STEP)
                            );
        if (TempFileHandles == NULL) {
          ASSERT (TempFileHandles != NULL);
          return;
        }

        CopyMem (
          TempFileHandles,
          Private->TempFileHandles,
          sizeof (EFI_PEI_FILE_HANDLE) * Private->TempPeimCount
          );
        Private->TempFileHandles = TempFileHandles;
        TempFileGuid             = AllocatePool (
                                     sizeof (EFI_GUID) * (Private->TempPeimCount + TEMP_FILE_GROWTH_STEP)
                                     );
        if (TempFileGuid == NULL) {
          ASSERT (TempFileGuid != NULL);
          return;
        }

        CopyMem (
          TempFileGuid,
          Private->TempFileGuid,
          sizeof (EFI_GUID) * Private->TempPeimCount
          );
        Private->TempFileGuid  = TempFileGuid;
        Private->TempPeimCount = Private->TempPeimCount + TEMP_FILE_GROWTH_STEP;
      }

      TempFileHandles[PeimCount++] = FileHandle;
    }
  } while (!EFI_ERROR (Status));

  DEBUG ((
    DEBUG_INFO,
    "%a(): Found 0x%x PEI FFS files in the %dth FV\n",
    __func__,
    PeimCount,
    Private->CurrentPeimFvCount
    ));

  if (PeimCount == 0) {
    //
    // No PEIM FFS file is found, set ScanFv flag and return.
    //
    CoreFileHandle->ScanFv = TRUE;
    return;
  }

  //
  // Record PeimCount, allocate buffer for PeimState and FvFileHandles.
  //
  CoreFileHandle->PeimCount = PeimCount;
  CoreFileHandle->PeimState = AllocateZeroPool (sizeof (UINT8) * PeimCount);
  ASSERT (CoreFileHandle->PeimState != NULL);
  CoreFileHandle->FvFileHandles = AllocateZeroPool (sizeof (EFI_PEI_FILE_HANDLE) * PeimCount);
  ASSERT (CoreFileHandle->FvFileHandles != NULL);

  //
  // Get Apriori File handle
  //
  Private->AprioriCount = 0;
  Status                = FvPpi->FindFileByName (FvPpi, &gPeiAprioriFileNameGuid, &CoreFileHandle->FvHandle, &AprioriFileHandle);
  if (!EFI_ERROR (Status) && (AprioriFileHandle != NULL)) {
    //
    // Read the Apriori file
    //
    Status = FvPpi->FindSectionByType (FvPpi, EFI_SECTION_RAW, AprioriFileHandle, (VOID **)&Apriori);
    if (!EFI_ERROR (Status)) {
      //
      // Calculate the number of PEIMs in the Apriori file
      //
      Status = FvPpi->GetFileInfo (FvPpi, AprioriFileHandle, &FileInfo);
      ASSERT_EFI_ERROR (Status);
      Private->AprioriCount = FileInfo.BufferSize;
      if (IS_SECTION2 (FileInfo.Buffer)) {
        Private->AprioriCount -= sizeof (EFI_COMMON_SECTION_HEADER2);
      } else {
        Private->AprioriCount -= sizeof (EFI_COMMON_SECTION_HEADER);
      }

      Private->AprioriCount /= sizeof (EFI_GUID);

      for (Index = 0; Index < PeimCount; Index++) {
        //
        // Make an array of file name GUIDs that matches the FileHandle array so we can convert
        // quickly from file name to file handle
        //
        Status = FvPpi->GetFileInfo (FvPpi, TempFileHandles[Index], &FileInfo);
        ASSERT_EFI_ERROR (Status);
        CopyMem (&TempFileGuid[Index], &FileInfo.FileName, sizeof (EFI_GUID));
      }

      //
      // Walk through TempFileGuid array to find out who is invalid PEIM GUID in Apriori file.
      // Add available PEIMs in Apriori file into FvFileHandles array.
      //
      Index = 0;
      for (Index2 = 0; Index2 < Private->AprioriCount; Index2++) {
        Guid = ScanGuid (TempFileGuid, PeimCount * sizeof (EFI_GUID), &Apriori[Index2]);
        if (Guid != NULL) {
          PeimIndex                              = ((UINTN)Guid - (UINTN)&TempFileGuid[0])/sizeof (EFI_GUID);
          CoreFileHandle->FvFileHandles[Index++] = TempFileHandles[PeimIndex];

          //
          // Since we have copied the file handle we can remove it from this list.
          //
          TempFileHandles[PeimIndex] = NULL;
        }
      }

      //
      // Update valid AprioriCount
      //
      Private->AprioriCount = Index;

      //
      // Add in any PEIMs not in the Apriori file
      //
      for (Index2 = 0; Index2 < PeimCount; Index2++) {
        if (TempFileHandles[Index2] != NULL) {
          CoreFileHandle->FvFileHandles[Index++] = TempFileHandles[Index2];
          TempFileHandles[Index2]                = NULL;
        }
      }

      ASSERT (Index == PeimCount);
    }
  } else {
    CopyMem (CoreFileHandle->FvFileHandles, TempFileHandles, sizeof (EFI_PEI_FILE_HANDLE) * PeimCount);
  }

  //
  // The current FV File Handles have been cached. So that we don't have to scan the FV again.
  // Instead, we can retrieve the file handles within this FV from cached records.
  //
  CoreFileHandle->ScanFv        = TRUE;
  Private->CurrentFvFileHandles = CoreFileHandle->FvFileHandles;
}

//
// This is the minimum memory required by DxeCore initialization. When LMFA feature enabled,
// This part of memory still need reserved on the very top of memory so that the DXE Core could
// use these memory for data initialization. This macro should be sync with the same marco
// defined in DXE Core.
//
#define MINIMUM_INITIAL_MEMORY_SIZE  0x10000

/**
  This function is to test if the memory range described in resource HOB is available or not.

  This function should only be invoked when Loading Module at Fixed Address(LMFA) feature is enabled. Some platform may allocate the
  memory before PeiLoadFixAddressHook in invoked. so this function is to test if the memory range described by the input resource HOB is
  available or not.

  @param PrivateData         Pointer to the private data passed in from caller
  @param ResourceHob         Pointer to a resource HOB which described the memory range described by the input resource HOB
**/
BOOLEAN
PeiLoadFixAddressIsMemoryRangeAvailable (
  IN PEI_CORE_INSTANCE            *PrivateData,
  IN EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob
  )
{
  EFI_HOB_MEMORY_ALLOCATION  *MemoryHob;
  BOOLEAN                    IsAvailable;
  EFI_PEI_HOB_POINTERS       Hob;

  IsAvailable = TRUE;
  if ((PrivateData == NULL) || (ResourceHob == NULL)) {
    return FALSE;
  }

  //
  // test if the memory range describe in the HOB is already allocated.
  //
  for (Hob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    //
    // See if this is a memory allocation HOB
    //
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      MemoryHob = Hob.MemoryAllocation;
      if ((MemoryHob->AllocDescriptor.MemoryBaseAddress == ResourceHob->PhysicalStart) &&
          (MemoryHob->AllocDescriptor.MemoryBaseAddress + MemoryHob->AllocDescriptor.MemoryLength == ResourceHob->PhysicalStart + ResourceHob->ResourceLength))
      {
        IsAvailable = FALSE;
        break;
      }
    }
  }

  return IsAvailable;
}

/**
  Hook function for Loading Module at Fixed Address feature

  This function should only be invoked when Loading Module at Fixed Address(LMFA) feature is enabled. When feature is
  configured as Load Modules at Fix Absolute Address, this function is to validate the top address assigned by user. When
  feature is configured as Load Modules at Fixed Offset, the function is to find the top address which is TOLM-TSEG in general.
  And also the function will re-install PEI memory.

  @param PrivateData         Pointer to the private data passed in from caller

**/
VOID
PeiLoadFixAddressHook (
  IN PEI_CORE_INSTANCE  *PrivateData
  )
{
  EFI_PHYSICAL_ADDRESS         TopLoadingAddress;
  UINT64                       PeiMemorySize;
  UINT64                       TotalReservedMemorySize;
  UINT64                       MemoryRangeEnd;
  EFI_PHYSICAL_ADDRESS         HighAddress;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *NextResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *CurrentResourceHob;
  EFI_PEI_HOB_POINTERS         CurrentHob;
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_PEI_HOB_POINTERS         NextHob;
  EFI_HOB_MEMORY_ALLOCATION    *MemoryHob;

  //
  // Initialize Local Variables
  //
  CurrentResourceHob = NULL;
  ResourceHob        = NULL;
  NextResourceHob    = NULL;
  HighAddress        = 0;
  TopLoadingAddress  = 0;
  MemoryRangeEnd     = 0;
  CurrentHob.Raw     = PrivateData->HobList.Raw;
  PeiMemorySize      = PrivateData->PhysicalMemoryLength;
  //
  // The top reserved memory include 3 parts: the topest range is for DXE core initialization with the size  MINIMUM_INITIAL_MEMORY_SIZE
  // then RuntimeCodePage range and Boot time code range.
  //
  TotalReservedMemorySize  = MINIMUM_INITIAL_MEMORY_SIZE + EFI_PAGES_TO_SIZE (PcdGet32 (PcdLoadFixAddressRuntimeCodePageNumber));
  TotalReservedMemorySize += EFI_PAGES_TO_SIZE (PcdGet32 (PcdLoadFixAddressBootTimeCodePageNumber));
  //
  // PEI memory range lies below the top reserved memory
  //
  TotalReservedMemorySize += PeiMemorySize;

  DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED INFO: PcdLoadFixAddressRuntimeCodePageNumber= 0x%x.\n", PcdGet32 (PcdLoadFixAddressRuntimeCodePageNumber)));
  DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED INFO: PcdLoadFixAddressBootTimeCodePageNumber= 0x%x.\n", PcdGet32 (PcdLoadFixAddressBootTimeCodePageNumber)));
  DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED INFO: PcdLoadFixAddressPeiCodePageNumber= 0x%x.\n", PcdGet32 (PcdLoadFixAddressPeiCodePageNumber)));
  DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED INFO: Total Reserved Memory Size = 0x%lx.\n", TotalReservedMemorySize));
  //
  // Loop through the system memory typed HOB to merge the adjacent memory range
  //
  for (Hob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    //
    // See if this is a resource descriptor HOB
    //
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      ResourceHob = Hob.ResourceDescriptor;
      //
      // If range described in this HOB is not system memory or higher than MAX_ADDRESS, ignored.
      //
      if ((ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) ||
          (ResourceHob->PhysicalStart + ResourceHob->ResourceLength > MAX_ADDRESS))
      {
        continue;
      }

      for (NextHob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST (NextHob); NextHob.Raw = GET_NEXT_HOB (NextHob)) {
        if (NextHob.Raw == Hob.Raw) {
          continue;
        }

        //
        // See if this is a resource descriptor HOB
        //
        if (GET_HOB_TYPE (NextHob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
          NextResourceHob = NextHob.ResourceDescriptor;
          //
          // test if range described in this NextResourceHob is system memory and have the same attribute.
          // Note: Here is a assumption that system memory should always be healthy even without test.
          //
          if ((NextResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) &&
              (((NextResourceHob->ResourceAttribute^ResourceHob->ResourceAttribute) & (~EFI_RESOURCE_ATTRIBUTE_TESTED)) == 0))
          {
            //
            // See if the memory range described in ResourceHob and NextResourceHob is adjacent
            //
            if (((ResourceHob->PhysicalStart <= NextResourceHob->PhysicalStart) &&
                 (ResourceHob->PhysicalStart + ResourceHob->ResourceLength >= NextResourceHob->PhysicalStart)) ||
                ((ResourceHob->PhysicalStart >= NextResourceHob->PhysicalStart) &&
                 (ResourceHob->PhysicalStart <= NextResourceHob->PhysicalStart + NextResourceHob->ResourceLength)))
            {
              MemoryRangeEnd = ((ResourceHob->PhysicalStart + ResourceHob->ResourceLength) > (NextResourceHob->PhysicalStart + NextResourceHob->ResourceLength)) ?
                               (ResourceHob->PhysicalStart + ResourceHob->ResourceLength) : (NextResourceHob->PhysicalStart + NextResourceHob->ResourceLength);

              ResourceHob->PhysicalStart = (ResourceHob->PhysicalStart < NextResourceHob->PhysicalStart) ?
                                           ResourceHob->PhysicalStart : NextResourceHob->PhysicalStart;

              ResourceHob->ResourceLength = (MemoryRangeEnd - ResourceHob->PhysicalStart);

              ResourceHob->ResourceAttribute = ResourceHob->ResourceAttribute & (~EFI_RESOURCE_ATTRIBUTE_TESTED);
              //
              // Delete the NextResourceHob by marking it as unused.
              //
              GET_HOB_TYPE (NextHob) = EFI_HOB_TYPE_UNUSED;
            }
          }
        }
      }
    }
  }

  //
  // Some platform is already allocated pages before the HOB re-org. Here to build dedicated resource HOB to describe
  //  the allocated memory range
  //
  for (Hob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    //
    // See if this is a memory allocation HOB
    //
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      MemoryHob = Hob.MemoryAllocation;
      for (NextHob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST (NextHob); NextHob.Raw = GET_NEXT_HOB (NextHob)) {
        //
        // See if this is a resource descriptor HOB
        //
        if (GET_HOB_TYPE (NextHob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
          NextResourceHob = NextHob.ResourceDescriptor;
          //
          // If range described in this HOB is not system memory or higher than MAX_ADDRESS, ignored.
          //
          if ((NextResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) || (NextResourceHob->PhysicalStart + NextResourceHob->ResourceLength > MAX_ADDRESS)) {
            continue;
          }

          //
          // If the range describe in memory allocation HOB belongs to the memory range described by the resource HOB
          //
          if ((MemoryHob->AllocDescriptor.MemoryBaseAddress >= NextResourceHob->PhysicalStart) &&
              (MemoryHob->AllocDescriptor.MemoryBaseAddress + MemoryHob->AllocDescriptor.MemoryLength <= NextResourceHob->PhysicalStart + NextResourceHob->ResourceLength))
          {
            //
            // Build separate resource HOB for this allocated range
            //
            if (MemoryHob->AllocDescriptor.MemoryBaseAddress > NextResourceHob->PhysicalStart) {
              BuildResourceDescriptorHob (
                EFI_RESOURCE_SYSTEM_MEMORY,
                NextResourceHob->ResourceAttribute,
                NextResourceHob->PhysicalStart,
                (MemoryHob->AllocDescriptor.MemoryBaseAddress - NextResourceHob->PhysicalStart)
                );
            }

            if (MemoryHob->AllocDescriptor.MemoryBaseAddress + MemoryHob->AllocDescriptor.MemoryLength < NextResourceHob->PhysicalStart + NextResourceHob->ResourceLength) {
              BuildResourceDescriptorHob (
                EFI_RESOURCE_SYSTEM_MEMORY,
                NextResourceHob->ResourceAttribute,
                MemoryHob->AllocDescriptor.MemoryBaseAddress + MemoryHob->AllocDescriptor.MemoryLength,
                (NextResourceHob->PhysicalStart + NextResourceHob->ResourceLength -(MemoryHob->AllocDescriptor.MemoryBaseAddress + MemoryHob->AllocDescriptor.MemoryLength))
                );
            }

            NextResourceHob->PhysicalStart  = MemoryHob->AllocDescriptor.MemoryBaseAddress;
            NextResourceHob->ResourceLength = MemoryHob->AllocDescriptor.MemoryLength;
            break;
          }
        }
      }
    }
  }

  //
  // Try to find and validate the TOP address.
  //
  if ((INT64)PcdGet64 (PcdLoadModuleAtFixAddressEnable) > 0 ) {
    //
    // The LMFA feature is enabled as load module at fixed absolute address.
    //
    TopLoadingAddress = (EFI_PHYSICAL_ADDRESS)PcdGet64 (PcdLoadModuleAtFixAddressEnable);
    DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED INFO: Loading module at fixed absolute address.\n"));
    //
    // validate the Address. Loop the resource descriptor HOB to make sure the address is in valid memory range
    //
    if ((TopLoadingAddress & EFI_PAGE_MASK) != 0) {
      DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED ERROR:Top Address 0x%lx is invalid since top address should be page align. \n", TopLoadingAddress));
      ASSERT (FALSE);
    }

    //
    // Search for a memory region that is below MAX_ADDRESS and in which TopLoadingAddress lies
    //
    for (Hob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
      //
      // See if this is a resource descriptor HOB
      //
      if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
        ResourceHob = Hob.ResourceDescriptor;
        //
        // See if this resource descriptor HOB describes tested system memory below MAX_ADDRESS
        //
        if ((ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) &&
            (ResourceHob->PhysicalStart + ResourceHob->ResourceLength <= MAX_ADDRESS))
        {
          //
          // See if Top address specified by user is valid.
          //
          if ((ResourceHob->PhysicalStart + TotalReservedMemorySize < TopLoadingAddress) &&
              ((ResourceHob->PhysicalStart + ResourceHob->ResourceLength - MINIMUM_INITIAL_MEMORY_SIZE) >= TopLoadingAddress) &&
              PeiLoadFixAddressIsMemoryRangeAvailable (PrivateData, ResourceHob))
          {
            CurrentResourceHob = ResourceHob;
            CurrentHob         = Hob;
            break;
          }
        }
      }
    }

    if (CurrentResourceHob != NULL) {
      DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED INFO:Top Address 0x%lx is valid \n", TopLoadingAddress));
      TopLoadingAddress += MINIMUM_INITIAL_MEMORY_SIZE;
    } else {
      DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED ERROR:Top Address 0x%lx is invalid \n", TopLoadingAddress));
      DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED ERROR:The recommended Top Address for the platform is: \n"));
      //
      // Print the recommended Top address range.
      //
      for (Hob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
        //
        // See if this is a resource descriptor HOB
        //
        if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
          ResourceHob = Hob.ResourceDescriptor;
          //
          // See if this resource descriptor HOB describes tested system memory below MAX_ADDRESS
          //
          if ((ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) &&
              (ResourceHob->PhysicalStart + ResourceHob->ResourceLength <= MAX_ADDRESS))
          {
            //
            // See if Top address specified by user is valid.
            //
            if ((ResourceHob->ResourceLength > TotalReservedMemorySize) && PeiLoadFixAddressIsMemoryRangeAvailable (PrivateData, ResourceHob)) {
              DEBUG ((
                DEBUG_INFO,
                "(0x%lx, 0x%lx)\n",
                (ResourceHob->PhysicalStart + TotalReservedMemorySize -MINIMUM_INITIAL_MEMORY_SIZE),
                (ResourceHob->PhysicalStart + ResourceHob->ResourceLength -MINIMUM_INITIAL_MEMORY_SIZE)
                ));
            }
          }
        }
      }

      //
      // Assert here
      //
      ASSERT (FALSE);
      return;
    }
  } else {
    //
    // The LMFA feature is enabled as load module at fixed offset relative to TOLM
    // Parse the Hob list to find the topest available memory. Generally it is (TOLM - TSEG)
    //
    //
    // Search for a tested memory region that is below MAX_ADDRESS
    //
    for (Hob.Raw = PrivateData->HobList.Raw; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
      //
      // See if this is a resource descriptor HOB
      //
      if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
        ResourceHob = Hob.ResourceDescriptor;
        //
        // See if this resource descriptor HOB describes tested system memory below MAX_ADDRESS
        //
        if ((ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) &&
            (ResourceHob->PhysicalStart + ResourceHob->ResourceLength <= MAX_ADDRESS) &&
            (ResourceHob->ResourceLength > TotalReservedMemorySize) && PeiLoadFixAddressIsMemoryRangeAvailable (PrivateData, ResourceHob))
        {
          //
          // See if this is the highest largest system memory region below MaxAddress
          //
          if (ResourceHob->PhysicalStart > HighAddress) {
            CurrentResourceHob = ResourceHob;
            CurrentHob         = Hob;
            HighAddress        = CurrentResourceHob->PhysicalStart;
          }
        }
      }
    }

    if (CurrentResourceHob == NULL) {
      DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED ERROR:The System Memory is too small\n"));
      //
      // Assert here
      //
      ASSERT (FALSE);
      return;
    } else {
      TopLoadingAddress = CurrentResourceHob->PhysicalStart + CurrentResourceHob->ResourceLength;
    }
  }

  if (CurrentResourceHob != NULL) {
    //
    // rebuild resource HOB for PEI memory and reserved memory
    //
    BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      (
       EFI_RESOURCE_ATTRIBUTE_PRESENT |
       EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
       EFI_RESOURCE_ATTRIBUTE_TESTED |
       EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
       EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
       EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
       EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
      ),
      (TopLoadingAddress - TotalReservedMemorySize),
      TotalReservedMemorySize
      );
    //
    // rebuild resource for the remain memory if necessary
    //
    if (CurrentResourceHob->PhysicalStart < TopLoadingAddress - TotalReservedMemorySize) {
      BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        (
         EFI_RESOURCE_ATTRIBUTE_PRESENT |
         EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
         EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
        ),
        CurrentResourceHob->PhysicalStart,
        (TopLoadingAddress - TotalReservedMemorySize - CurrentResourceHob->PhysicalStart)
        );
    }

    if (CurrentResourceHob->PhysicalStart + CurrentResourceHob->ResourceLength  > TopLoadingAddress ) {
      BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        (
         EFI_RESOURCE_ATTRIBUTE_PRESENT |
         EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
         EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
         EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
        ),
        TopLoadingAddress,
        (CurrentResourceHob->PhysicalStart + CurrentResourceHob->ResourceLength  - TopLoadingAddress)
        );
    }

    //
    // Delete CurrentHob by marking it as unused since the memory range described by is rebuilt.
    //
    GET_HOB_TYPE (CurrentHob) = EFI_HOB_TYPE_UNUSED;
  }

  //
  // Cache the top address for Loading Module at Fixed Address feature
  //
  PrivateData->LoadModuleAtFixAddressTopAddress = TopLoadingAddress - MINIMUM_INITIAL_MEMORY_SIZE;
  DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED INFO: Top address = 0x%lx\n", PrivateData->LoadModuleAtFixAddressTopAddress));
  //
  // reinstall the PEI memory relative to TopLoadingAddress
  //
  PrivateData->PhysicalMemoryBegin   = TopLoadingAddress - TotalReservedMemorySize;
  PrivateData->FreePhysicalMemoryTop = PrivateData->PhysicalMemoryBegin + PeiMemorySize;
}

/**
  This routine is invoked in switch stack as PeiCore Entry.

  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.
  @param Private         Pointer to old core data that is used to initialize the
                         core's data areas.
**/
VOID
EFIAPI
PeiCoreEntry (
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *Private
  )
{
  //
  // Entry PEI Phase 2
  //
  PeiCore (SecCoreData, NULL, Private);
}

/**
  Check SwitchStackSignal and switch stack if SwitchStackSignal is TRUE.

  @param[in] SecCoreData    Points to a data structure containing information about the PEI core's operating
                            environment, such as the size and location of temporary RAM, the stack location and
                            the BFV location.
  @param[in] Private        Pointer to the private data passed in from caller.

**/
VOID
PeiCheckAndSwitchStack (
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *Private
  )
{
  VOID                               *LoadFixPeiCodeBegin;
  EFI_STATUS                         Status;
  CONST EFI_PEI_SERVICES             **PeiServices;
  UINT64                             NewStackSize;
  EFI_PHYSICAL_ADDRESS               TopOfOldStack;
  EFI_PHYSICAL_ADDRESS               TopOfNewStack;
  UINTN                              StackOffset;
  BOOLEAN                            StackOffsetPositive;
  EFI_PHYSICAL_ADDRESS               TemporaryRamBase;
  UINTN                              TemporaryRamSize;
  UINTN                              TemporaryStackSize;
  VOID                               *TemporaryStackBase;
  UINTN                              PeiTemporaryRamSize;
  VOID                               *PeiTemporaryRamBase;
  EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI  *TemporaryRamSupportPpi;
  EFI_PHYSICAL_ADDRESS               BaseOfNewHeap;
  EFI_PHYSICAL_ADDRESS               HoleMemBase;
  UINTN                              HoleMemSize;
  UINTN                              HeapTemporaryRamSize;
  EFI_PHYSICAL_ADDRESS               TempBase1;
  UINTN                              TempSize1;
  EFI_PHYSICAL_ADDRESS               TempBase2;
  UINTN                              TempSize2;
  UINTN                              Index;

  PeiServices = (CONST EFI_PEI_SERVICES **)&Private->Ps;

  if (Private->SwitchStackSignal) {
    //
    // Before switch stack from temporary memory to permanent memory, calculate the heap and stack
    // usage in temporary memory for debugging.
    //
    DEBUG_CODE_BEGIN ();
    UINT32                *StackPointer;
    EFI_PEI_HOB_POINTERS  Hob;

    for (  StackPointer = (UINT32 *)SecCoreData->StackBase;
           (StackPointer < (UINT32 *)((UINTN)SecCoreData->StackBase + SecCoreData->StackSize)) \
        && (*StackPointer == PcdGet32 (PcdInitValueInTempStack));
           StackPointer++)
    {
    }

    DEBUG ((DEBUG_INFO, "Temp Stack : BaseAddress=0x%p Length=0x%X\n", SecCoreData->StackBase, (UINT32)SecCoreData->StackSize));
    DEBUG ((DEBUG_INFO, "Temp Heap  : BaseAddress=0x%p Length=0x%X\n", SecCoreData->PeiTemporaryRamBase, (UINT32)SecCoreData->PeiTemporaryRamSize));
    DEBUG ((DEBUG_INFO, "Total temporary memory:    %d bytes.\n", (UINT32)SecCoreData->TemporaryRamSize));
    DEBUG ((
      DEBUG_INFO,
      "  temporary memory stack ever used:       %d bytes.\n",
      (UINT32)(SecCoreData->StackSize - ((UINTN)StackPointer - (UINTN)SecCoreData->StackBase))
      ));
    DEBUG ((
      DEBUG_INFO,
      "  temporary memory heap used for HobList: %d bytes.\n",
      (UINT32)((UINTN)Private->HobList.HandoffInformationTable->EfiFreeMemoryBottom - (UINTN)Private->HobList.Raw)
      ));
    DEBUG ((
      DEBUG_INFO,
      "  temporary memory heap occupied by memory pages: %d bytes.\n",
      (UINT32)(UINTN)(Private->HobList.HandoffInformationTable->EfiMemoryTop - Private->HobList.HandoffInformationTable->EfiFreeMemoryTop)
      ));
    for (Hob.Raw = Private->HobList.Raw; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
      if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
        DEBUG ((
          DEBUG_INFO,
          "Memory Allocation 0x%08x 0x%0lx - 0x%0lx\n", \
          Hob.MemoryAllocation->AllocDescriptor.MemoryType,               \
          Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,        \
          Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress + Hob.MemoryAllocation->AllocDescriptor.MemoryLength - 1
          ));
      }
    }

    DEBUG_CODE_END ();

    if ((PcdGet64 (PcdLoadModuleAtFixAddressEnable) != 0) && (Private->HobList.HandoffInformationTable->BootMode != BOOT_ON_S3_RESUME)) {
      //
      // Loading Module at Fixed Address is enabled
      //
      PeiLoadFixAddressHook (Private);

      //
      // If Loading Module at Fixed Address is enabled, Allocating memory range for Pei code range.
      //
      LoadFixPeiCodeBegin = AllocatePages ((UINTN)PcdGet32 (PcdLoadFixAddressPeiCodePageNumber));
      DEBUG ((DEBUG_INFO, "LOADING MODULE FIXED INFO: PeiCodeBegin = 0x%lX, PeiCodeTop= 0x%lX\n", (UINT64)(UINTN)LoadFixPeiCodeBegin, (UINT64)((UINTN)LoadFixPeiCodeBegin + PcdGet32 (PcdLoadFixAddressPeiCodePageNumber) * EFI_PAGE_SIZE)));
    }

    //
    // Reserve the size of new stack at bottom of physical memory
    //
    // The size of new stack in permanent memory must be the same size
    // or larger than the size of old stack in temporary memory.
    // But if new stack is smaller than the size of old stack, we also reserve
    // the size of old stack at bottom of permanent memory.
    //
    NewStackSize = RShiftU64 (Private->PhysicalMemoryLength, 1);
    NewStackSize = ALIGN_VALUE (NewStackSize, EFI_PAGE_SIZE);
    NewStackSize = MIN (PcdGet32 (PcdPeiCoreMaxPeiStackSize), NewStackSize);
    DEBUG ((DEBUG_INFO, "Old Stack size %d, New stack size %d\n", (UINT32)SecCoreData->StackSize, (UINT32)NewStackSize));
    ASSERT (NewStackSize >= SecCoreData->StackSize);

    //
    // Calculate stack offset and heap offset between temporary memory and new permanent
    // memory separately.
    //
    TopOfOldStack = (UINTN)SecCoreData->StackBase + SecCoreData->StackSize;
    TopOfNewStack = Private->PhysicalMemoryBegin + NewStackSize;
    if (TopOfNewStack >= TopOfOldStack) {
      StackOffsetPositive = TRUE;
      StackOffset         = (UINTN)(TopOfNewStack - TopOfOldStack);
    } else {
      StackOffsetPositive = FALSE;
      StackOffset         = (UINTN)(TopOfOldStack - TopOfNewStack);
    }

    Private->StackOffsetPositive = StackOffsetPositive;
    Private->StackOffset         = StackOffset;

    //
    // Build Stack HOB that describes the permanent memory stack
    //
    DEBUG ((DEBUG_INFO, "Stack Hob: BaseAddress=0x%lX Length=0x%lX\n", TopOfNewStack - NewStackSize, NewStackSize));
    BuildStackHob (TopOfNewStack - NewStackSize, NewStackSize);

    //
    // Cache information from SecCoreData into locals before SecCoreData is converted to a permanent memory address
    //
    TemporaryRamBase    = (EFI_PHYSICAL_ADDRESS)(UINTN)SecCoreData->TemporaryRamBase;
    TemporaryRamSize    = SecCoreData->TemporaryRamSize;
    TemporaryStackSize  = SecCoreData->StackSize;
    TemporaryStackBase  = SecCoreData->StackBase;
    PeiTemporaryRamSize = SecCoreData->PeiTemporaryRamSize;
    PeiTemporaryRamBase = SecCoreData->PeiTemporaryRamBase;

    //
    // TemporaryRamSupportPpi is produced by platform's SEC
    //
    Status = PeiServicesLocatePpi (
               &gEfiTemporaryRamSupportPpiGuid,
               0,
               NULL,
               (VOID **)&TemporaryRamSupportPpi
               );
    if (!EFI_ERROR (Status)) {
      //
      // Heap Offset
      //
      BaseOfNewHeap = TopOfNewStack;
      if (BaseOfNewHeap >= (UINTN)SecCoreData->PeiTemporaryRamBase) {
        Private->HeapOffsetPositive = TRUE;
        Private->HeapOffset         = (UINTN)(BaseOfNewHeap - (UINTN)SecCoreData->PeiTemporaryRamBase);
      } else {
        Private->HeapOffsetPositive = FALSE;
        Private->HeapOffset         = (UINTN)((UINTN)SecCoreData->PeiTemporaryRamBase - BaseOfNewHeap);
      }

      DEBUG ((DEBUG_INFO, "Heap Offset = 0x%lX Stack Offset = 0x%lX\n", (UINT64)Private->HeapOffset, (UINT64)Private->StackOffset));

      //
      // Calculate new HandOffTable and PrivateData address in permanent memory's stack
      //
      if (StackOffsetPositive) {
        SecCoreData = (CONST EFI_SEC_PEI_HAND_OFF *)((UINTN)(VOID *)SecCoreData + StackOffset);
        Private     = (PEI_CORE_INSTANCE *)((UINTN)(VOID *)Private + StackOffset);
      } else {
        SecCoreData = (CONST EFI_SEC_PEI_HAND_OFF *)((UINTN)(VOID *)SecCoreData - StackOffset);
        Private     = (PEI_CORE_INSTANCE *)((UINTN)(VOID *)Private - StackOffset);
      }

      //
      // Temporary Ram Support PPI is provided by platform, it will copy
      // temporary memory to permanent memory and do stack switching.
      // After invoking Temporary Ram Support PPI, the following code's
      // stack is in permanent memory.
      //
      TemporaryRamSupportPpi->TemporaryRamMigration (
                                PeiServices,
                                TemporaryRamBase,
                                (EFI_PHYSICAL_ADDRESS)(UINTN)(TopOfNewStack - TemporaryStackSize),
                                TemporaryRamSize
                                );

      //
      // Migrate memory pages allocated in pre-memory phase.
      // It could not be called before calling TemporaryRamSupportPpi->TemporaryRamMigration()
      // as the migrated memory pages may be overridden by TemporaryRamSupportPpi->TemporaryRamMigration().
      //
      MigrateMemoryPages (Private, TRUE);

      //
      // Entry PEI Phase 2
      //
      PeiCore (SecCoreData, NULL, Private);
    } else {
      //
      // Migrate memory pages allocated in pre-memory phase.
      //
      MigrateMemoryPages (Private, FALSE);

      //
      // Migrate the PEI Services Table pointer from temporary RAM to permanent RAM.
      //
      MigratePeiServicesTablePointer ();

      //
      // Heap Offset
      //
      BaseOfNewHeap = TopOfNewStack;
      HoleMemBase   = TopOfNewStack;
      HoleMemSize   = TemporaryRamSize - PeiTemporaryRamSize - TemporaryStackSize;
      if (HoleMemSize != 0) {
        //
        // Make sure HOB List start address is 8 byte alignment.
        //
        BaseOfNewHeap = ALIGN_VALUE (BaseOfNewHeap + HoleMemSize, 8);
      }

      if (BaseOfNewHeap >= (UINTN)SecCoreData->PeiTemporaryRamBase) {
        Private->HeapOffsetPositive = TRUE;
        Private->HeapOffset         = (UINTN)(BaseOfNewHeap - (UINTN)SecCoreData->PeiTemporaryRamBase);
      } else {
        Private->HeapOffsetPositive = FALSE;
        Private->HeapOffset         = (UINTN)((UINTN)SecCoreData->PeiTemporaryRamBase - BaseOfNewHeap);
      }

      DEBUG ((DEBUG_INFO, "Heap Offset = 0x%lX Stack Offset = 0x%lX\n", (UINT64)Private->HeapOffset, (UINT64)Private->StackOffset));

      //
      // Migrate Heap
      //
      HeapTemporaryRamSize = (UINTN)(Private->HobList.HandoffInformationTable->EfiFreeMemoryBottom - Private->HobList.HandoffInformationTable->EfiMemoryBottom);
      ASSERT (BaseOfNewHeap + HeapTemporaryRamSize <= Private->FreePhysicalMemoryTop);
      CopyMem ((UINT8 *)(UINTN)BaseOfNewHeap, PeiTemporaryRamBase, HeapTemporaryRamSize);

      //
      // Migrate Stack
      //
      CopyMem ((UINT8 *)(UINTN)(TopOfNewStack - TemporaryStackSize), TemporaryStackBase, TemporaryStackSize);

      //
      // Copy Hole Range Data
      //
      if (HoleMemSize != 0) {
        //
        // Prepare Hole
        //
        if (PeiTemporaryRamBase < TemporaryStackBase) {
          TempBase1 = (EFI_PHYSICAL_ADDRESS)(UINTN)PeiTemporaryRamBase;
          TempSize1 = PeiTemporaryRamSize;
          TempBase2 = (EFI_PHYSICAL_ADDRESS)(UINTN)TemporaryStackBase;
          TempSize2 = TemporaryStackSize;
        } else {
          TempBase1 = (EFI_PHYSICAL_ADDRESS)(UINTN)TemporaryStackBase;
          TempSize1 = TemporaryStackSize;
          TempBase2 = (EFI_PHYSICAL_ADDRESS)(UINTN)PeiTemporaryRamBase;
          TempSize2 = PeiTemporaryRamSize;
        }

        if (TemporaryRamBase < TempBase1) {
          Private->HoleData[0].Base = TemporaryRamBase;
          Private->HoleData[0].Size = (UINTN)(TempBase1 - TemporaryRamBase);
        }

        if (TempBase1 + TempSize1 < TempBase2) {
          Private->HoleData[1].Base = TempBase1 + TempSize1;
          Private->HoleData[1].Size = (UINTN)(TempBase2 - TempBase1 - TempSize1);
        }

        if (TempBase2 + TempSize2 < TemporaryRamBase + TemporaryRamSize) {
          Private->HoleData[2].Base = TempBase2 + TempSize2;
          Private->HoleData[2].Size = (UINTN)(TemporaryRamBase + TemporaryRamSize - TempBase2 - TempSize2);
        }

        //
        // Copy Hole Range data.
        //
        for (Index = 0; Index < HOLE_MAX_NUMBER; Index++) {
          if (Private->HoleData[Index].Size > 0) {
            if (HoleMemBase > Private->HoleData[Index].Base) {
              Private->HoleData[Index].OffsetPositive = TRUE;
              Private->HoleData[Index].Offset         = (UINTN)(HoleMemBase - Private->HoleData[Index].Base);
            } else {
              Private->HoleData[Index].OffsetPositive = FALSE;
              Private->HoleData[Index].Offset         = (UINTN)(Private->HoleData[Index].Base - HoleMemBase);
            }

            CopyMem ((VOID *)(UINTN)HoleMemBase, (VOID *)(UINTN)Private->HoleData[Index].Base, Private->HoleData[Index].Size);
            HoleMemBase = HoleMemBase + Private->HoleData[Index].Size;
          }
        }
      }

      //
      // Switch new stack
      //
      SwitchStack (
        (SWITCH_STACK_ENTRY_POINT)(UINTN)PeiCoreEntry,
        (VOID *)SecCoreData,
        (VOID *)Private,
        (VOID *)(UINTN)TopOfNewStack
        );
    }

    //
    // Code should not come here
    //
    ASSERT (FALSE);
  }
}

/**
  Migrate a PEIM from temporary RAM to permanent memory.

  @param PeimFileHandle       Pointer to the FFS file header of the image.
  @param MigratedFileHandle   Pointer to the FFS file header of the migrated image.

  @retval EFI_SUCCESS         Successfully migrated the PEIM to permanent memory.

**/
EFI_STATUS
EFIAPI
MigratePeim (
  IN  EFI_PEI_FILE_HANDLE  FileHandle,
  IN  EFI_PEI_FILE_HANDLE  MigratedFileHandle
  )
{
  EFI_STATUS           Status;
  EFI_FFS_FILE_HEADER  *FileHeader;
  VOID                 *Pe32Data;
  VOID                 *ImageAddress;
  CHAR8                *AsciiString;
  UINTN                Index;

  Status = EFI_SUCCESS;

  FileHeader = (EFI_FFS_FILE_HEADER *)FileHandle;
  ASSERT (!IS_FFS_FILE2 (FileHeader));

  ImageAddress = NULL;
  PeiGetPe32Data (MigratedFileHandle, &ImageAddress);
  if (ImageAddress != NULL) {
    DEBUG_CODE_BEGIN ();
    AsciiString = PeCoffLoaderGetPdbPointer (ImageAddress);
    for (Index = 0; AsciiString[Index] != 0; Index++) {
      if ((AsciiString[Index] == '\\') || (AsciiString[Index] == '/')) {
        AsciiString = AsciiString + Index + 1;
        Index       = 0;
      } else if (AsciiString[Index] == '.') {
        AsciiString[Index] = 0;
      }
    }

    DEBUG ((DEBUG_VERBOSE, "%a", AsciiString));
    DEBUG_CODE_END ();

    Pe32Data = (VOID *)((UINTN)ImageAddress - (UINTN)MigratedFileHandle + (UINTN)FileHandle);
    Status   = LoadAndRelocatePeCoffImageInPlace (Pe32Data, ImageAddress);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

/**
  Migrate Status Code Callback function pointers inside an FV from temporary memory to permanent memory.

  @param OrgFvHandle      Address of FV handle in temporary memory.
  @param FvHandle         Address of FV handle in permanent memory.
  @param FvSize           Size of the FV.

**/
VOID
ConvertStatusCodeCallbacks (
  IN  UINTN  OrgFvHandle,
  IN  UINTN  FvHandle,
  IN  UINTN  FvSize
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 *NumberOfEntries;
  UINTN                 *CallbackEntry;
  UINTN                 Index;

  Hob.Raw = GetFirstGuidHob (&gStatusCodeCallbackGuid);
  while (Hob.Raw != NULL) {
    NumberOfEntries = GET_GUID_HOB_DATA (Hob);
    CallbackEntry   = NumberOfEntries + 1;
    for (Index = 0; Index < *NumberOfEntries; Index++) {
      if (((VOID *)CallbackEntry[Index]) != NULL) {
        if ((CallbackEntry[Index] >= OrgFvHandle) && (CallbackEntry[Index] < (OrgFvHandle + FvSize))) {
          DEBUG ((
            DEBUG_INFO,
            "Migrating CallbackEntry[%Lu] from 0x%0*Lx to ",
            (UINT64)Index,
            (sizeof CallbackEntry[Index]) * 2,
            (UINT64)CallbackEntry[Index]
            ));
          if (OrgFvHandle > FvHandle) {
            CallbackEntry[Index] = CallbackEntry[Index] - (OrgFvHandle - FvHandle);
          } else {
            CallbackEntry[Index] = CallbackEntry[Index] + (FvHandle - OrgFvHandle);
          }

          DEBUG ((
            DEBUG_INFO,
            "0x%0*Lx\n",
            (sizeof CallbackEntry[Index]) * 2,
            (UINT64)CallbackEntry[Index]
            ));
        }
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextGuidHob (&gStatusCodeCallbackGuid, Hob.Raw);
  }
}

/**
  Migrates PEIMs in the given firmware volume.

  @param Private          Pointer to the PeiCore's private data structure.
  @param FvIndex          The firmware volume index to migrate.
  @param OrgFvHandle      The handle to the firmware volume in temporary memory.
  @param FvHandle         The handle to the firmware volume in permanent memory.

  @retval   EFI_SUCCESS           The PEIMs in the FV were migrated successfully
  @retval   EFI_INVALID_PARAMETER The Private pointer is NULL or FvCount is invalid.

**/
EFI_STATUS
EFIAPI
MigratePeimsInFv (
  IN PEI_CORE_INSTANCE  *Private,
  IN  UINTN             FvIndex,
  IN  UINTN             OrgFvHandle,
  IN  UINTN             FvHandle
  )
{
  EFI_STATUS           Status;
  volatile UINTN       FileIndex;
  EFI_PEI_FILE_HANDLE  MigratedFileHandle;
  EFI_PEI_FILE_HANDLE  FileHandle;

  if ((Private == NULL) || (FvIndex >= Private->FvCount)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Private->Fv[FvIndex].ScanFv) {
    for (FileIndex = 0; FileIndex < Private->Fv[FvIndex].PeimCount; FileIndex++) {
      if (Private->Fv[FvIndex].FvFileHandles[FileIndex] != NULL) {
        FileHandle = Private->Fv[FvIndex].FvFileHandles[FileIndex];

        MigratedFileHandle = (EFI_PEI_FILE_HANDLE)((UINTN)FileHandle - OrgFvHandle + FvHandle);

        DEBUG ((DEBUG_VERBOSE, "    Migrating FileHandle %2d ", FileIndex));
        Status = MigratePeim (FileHandle, MigratedFileHandle);
        DEBUG ((DEBUG_VERBOSE, "\n"));
        ASSERT_EFI_ERROR (Status);

        if (!EFI_ERROR (Status)) {
          Private->Fv[FvIndex].FvFileHandles[FileIndex] = MigratedFileHandle;
          if (FvIndex == Private->CurrentPeimFvCount) {
            Private->CurrentFvFileHandles[FileIndex] = MigratedFileHandle;
          }
        }
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Migrate FVs out of temporary RAM before the cache is flushed.

  @param Private         PeiCore's private data structure
  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.

  @retval EFI_SUCCESS           Successfully migrated installed FVs from temporary RAM to permanent memory.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory exists to allocate needed pages.

**/
EFI_STATUS
EFIAPI
EvacuateTempRam (
  IN PEI_CORE_INSTANCE           *Private,
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData
  )
{
  EFI_STATUS                  Status;
  volatile UINTN              FvIndex;
  volatile UINTN              FvChildIndex;
  UINTN                       ChildFvOffset;
  EFI_PHYSICAL_ADDRESS        FvHeaderAddress;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  EFI_FIRMWARE_VOLUME_HEADER  *ChildFvHeader;
  EFI_FIRMWARE_VOLUME_HEADER  *MigratedFvHeader;
  EFI_FIRMWARE_VOLUME_HEADER  *RawDataFvHeader;
  EFI_FIRMWARE_VOLUME_HEADER  *MigratedChildFvHeader;

  PEI_CORE_FV_HANDLE            PeiCoreFvHandle;
  EFI_PEI_CORE_FV_LOCATION_PPI  *PeiCoreFvLocationPpi;
  EFI_PEI_HOB_POINTERS          Hob;
  EDKII_MIGRATION_INFO          *MigrationInfo;
  TO_MIGRATE_FV_INFO            *ToMigrateFvInfo;
  UINT32                        FvMigrationFlags;
  EDKII_MIGRATED_FV_INFO        MigratedFvInfo;
  UINTN                         Index;

  ASSERT (Private->PeiMemoryInstalled);

  DEBUG ((DEBUG_VERBOSE, "Beginning evacuation of content in temporary RAM.\n"));

  //
  // By default migrate all FVs and copy raw data
  //
  FvMigrationFlags = FLAGS_FV_RAW_DATA_COPY;

  //
  // Migrate PPI Pointers of PEI_CORE from temporary memory to newly loaded PEI_CORE in permanent memory.
  //
  Status = PeiLocatePpi ((CONST EFI_PEI_SERVICES **)&Private->Ps, &gEfiPeiCoreFvLocationPpiGuid, 0, NULL, (VOID **)&PeiCoreFvLocationPpi);
  if (!EFI_ERROR (Status) && (PeiCoreFvLocationPpi->PeiCoreFvLocation != NULL)) {
    PeiCoreFvHandle.FvHandle = (EFI_PEI_FV_HANDLE)PeiCoreFvLocationPpi->PeiCoreFvLocation;
  } else {
    PeiCoreFvHandle.FvHandle = (EFI_PEI_FV_HANDLE)SecCoreData->BootFirmwareVolumeBase;
  }

  if (Private->PeimDispatcherReenter) {
    //
    // PEI_CORE should be migrated after dispatcher re-enters from main memory.
    //
    for (FvIndex = 0; FvIndex < Private->FvCount; FvIndex++) {
      if (Private->Fv[FvIndex].FvHandle == PeiCoreFvHandle.FvHandle) {
        CopyMem (&PeiCoreFvHandle, &Private->Fv[FvIndex], sizeof (PEI_CORE_FV_HANDLE));
        break;
      }
    }

    Status = EFI_SUCCESS;

    ConvertPeiCorePpiPointers (Private, &PeiCoreFvHandle);
  }

  Hob.Raw = GetFirstGuidHob (&gEdkiiMigrationInfoGuid);
  if (Hob.Raw != NULL) {
    MigrationInfo = GET_GUID_HOB_DATA (Hob);
  } else {
    MigrationInfo = NULL;
  }

  for (FvIndex = 0; FvIndex < Private->FvCount; FvIndex++) {
    FvHeader = Private->Fv[FvIndex].FvHeader;
    ASSERT (FvHeader != NULL);
    ASSERT (FvIndex < Private->FvCount);

    DEBUG ((DEBUG_VERBOSE, "FV[%02d] at 0x%x.\n", FvIndex, (UINTN)FvHeader));
    if (
        !(
          ((EFI_PHYSICAL_ADDRESS)(UINTN)FvHeader >= Private->PhysicalMemoryBegin) &&
          (((EFI_PHYSICAL_ADDRESS)(UINTN)FvHeader + (FvHeader->FvLength - 1)) < Private->FreePhysicalMemoryTop)
          )
        )
    {
      if ((MigrationInfo == NULL) || (MigrationInfo->MigrateAll == TRUE)) {
        if (!Private->PeimDispatcherReenter) {
          //
          // Migration before dispatcher reentery is supported only when gEdkiiMigrationInfoGuid
          // HOB is built for selective FV migration.
          //
          return EFI_SUCCESS;
        }
      } else {
        for (Index = 0; Index < MigrationInfo->ToMigrateFvCount; Index++) {
          ToMigrateFvInfo = ((TO_MIGRATE_FV_INFO *)(MigrationInfo + 1)) + Index;
          if (ToMigrateFvInfo->FvOrgBaseOnTempRam == (UINT32)(UINTN)FvHeader) {
            //
            // This FV is to migrate
            //
            FvMigrationFlags = ToMigrateFvInfo->FvMigrationFlags;
            break;
          }
        }

        if ((Index == MigrationInfo->ToMigrateFvCount) ||
            ((!Private->PeimDispatcherReenter) &&
             (((FvMigrationFlags & FLAGS_FV_MIGRATE_BEFORE_PEI_CORE_REENTRY) == 0) ||
              (FvHeader == PeiCoreFvHandle.FvHandle))))
        {
          //
          // This FV is not expected to migrate
          //
          // FV should not be migrated before dispatcher reentry if any of the below condition is true:
          // a. MigrationInfo HOB is not built with flag FLAGS_FV_MIGRATE_BEFORE_PEI_CORE_REENTRY.
          // b. FV contains currently executing PEI Core.
          //
          continue;
        }
      }

      //
      // Allocate pages to save the rebased PEIMs, the PEIMs will get dispatched later.
      //
      Status =  PeiServicesAllocatePages (
                  EfiBootServicesCode,
                  EFI_SIZE_TO_PAGES ((UINTN)FvHeader->FvLength),
                  &FvHeaderAddress
                  );
      ASSERT_EFI_ERROR (Status);
      MigratedFvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvHeaderAddress;
      CopyMem (MigratedFvHeader, FvHeader, (UINTN)FvHeader->FvLength);

      DEBUG ((
        DEBUG_VERBOSE,
        "  Migrating FV[%d] from 0x%08X to 0x%08X\n",
        FvIndex,
        (UINTN)FvHeader,
        (UINTN)MigratedFvHeader
        ));

      //
      // Create hob to save MigratedFvInfo, this hob will only be produced when
      // Migration feature PCD PcdMigrateTemporaryRamFirmwareVolumes is set to TRUE.
      //
      MigratedFvInfo.FvOrgBase  = (UINT32)(UINTN)FvHeader;
      MigratedFvInfo.FvNewBase  = (UINT32)(UINTN)MigratedFvHeader;
      MigratedFvInfo.FvDataBase = 0;
      MigratedFvInfo.FvLength   = (UINT32)(UINTN)FvHeader->FvLength;

      //
      // When FLAGS_FV_RAW_DATA_COPY bit is set, copy the context to the raw pages and
      // reset raw data base address in MigratedFvInfo hob.
      //
      if ((FvMigrationFlags & FLAGS_FV_RAW_DATA_COPY) == FLAGS_FV_RAW_DATA_COPY) {
        //
        // Allocate pages to save the raw PEIMs
        //
        Status =  PeiServicesAllocatePages (
                    EfiBootServicesCode,
                    EFI_SIZE_TO_PAGES ((UINTN)FvHeader->FvLength),
                    &FvHeaderAddress
                    );
        ASSERT_EFI_ERROR (Status);
        RawDataFvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvHeaderAddress;
        CopyMem (RawDataFvHeader, FvHeader, (UINTN)FvHeader->FvLength);
        MigratedFvInfo.FvDataBase = (UINT32)(UINTN)RawDataFvHeader;
      }

      BuildGuidDataHob (&gEdkiiMigratedFvInfoGuid, &MigratedFvInfo, sizeof (MigratedFvInfo));

      //
      // Migrate any children for this FV now
      //
      for (FvChildIndex = FvIndex; FvChildIndex < Private->FvCount; FvChildIndex++) {
        ChildFvHeader = Private->Fv[FvChildIndex].FvHeader;
        if (
            ((UINTN)ChildFvHeader > (UINTN)FvHeader) &&
            (((UINTN)ChildFvHeader + ChildFvHeader->FvLength) < ((UINTN)FvHeader) + FvHeader->FvLength)
            )
        {
          DEBUG ((DEBUG_VERBOSE, "    Child FV[%02d] is being migrated.\n", FvChildIndex));
          ChildFvOffset = (UINTN)ChildFvHeader - (UINTN)FvHeader;
          DEBUG ((DEBUG_VERBOSE, "    Child FV offset = 0x%x.\n", ChildFvOffset));
          MigratedChildFvHeader              = (EFI_FIRMWARE_VOLUME_HEADER *)((UINTN)MigratedFvHeader + ChildFvOffset);
          Private->Fv[FvChildIndex].FvHeader = MigratedChildFvHeader;
          Private->Fv[FvChildIndex].FvHandle = (EFI_PEI_FV_HANDLE)MigratedChildFvHeader;
          DEBUG ((DEBUG_VERBOSE, "    Child migrated FV header at 0x%x.\n", (UINTN)MigratedChildFvHeader));

          Status =  MigratePeimsInFv (Private, FvChildIndex, (UINTN)ChildFvHeader, (UINTN)MigratedChildFvHeader);
          ASSERT_EFI_ERROR (Status);

          ConvertPpiPointersFv (
            Private,
            (UINTN)ChildFvHeader,
            (UINTN)MigratedChildFvHeader,
            (UINTN)ChildFvHeader->FvLength - 1
            );

          ConvertStatusCodeCallbacks (
            (UINTN)ChildFvHeader,
            (UINTN)MigratedChildFvHeader,
            (UINTN)ChildFvHeader->FvLength - 1
            );

          ConvertFvHob (Private, (UINTN)ChildFvHeader, (UINTN)MigratedChildFvHeader);
        }
      }

      Private->Fv[FvIndex].FvHeader = MigratedFvHeader;
      Private->Fv[FvIndex].FvHandle = (EFI_PEI_FV_HANDLE)MigratedFvHeader;

      Status = MigratePeimsInFv (Private, FvIndex, (UINTN)FvHeader, (UINTN)MigratedFvHeader);
      ASSERT_EFI_ERROR (Status);

      ConvertPpiPointersFv (
        Private,
        (UINTN)FvHeader,
        (UINTN)MigratedFvHeader,
        (UINTN)FvHeader->FvLength - 1
        );

      ConvertStatusCodeCallbacks (
        (UINTN)FvHeader,
        (UINTN)MigratedFvHeader,
        (UINTN)FvHeader->FvLength - 1
        );

      ConvertFvHob (Private, (UINTN)FvHeader, (UINTN)MigratedFvHeader);
    }
  }

  return Status;
}

/**
  Conduct PEIM dispatch.

  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.
  @param Private         Pointer to the private data passed in from caller

**/
VOID
PeiDispatcher (
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *Private
  )
{
  EFI_STATUS              Status;
  UINT32                  Index1;
  UINT32                  Index2;
  CONST EFI_PEI_SERVICES  **PeiServices;
  EFI_PEI_FILE_HANDLE     PeimFileHandle;
  UINTN                   FvCount;
  UINTN                   PeimCount;
  UINT32                  AuthenticationState;
  EFI_PHYSICAL_ADDRESS    EntryPoint;
  EFI_PEIM_ENTRY_POINT2   PeimEntryPoint;
  UINTN                   SaveCurrentPeimCount;
  UINTN                   SaveCurrentFvCount;
  EFI_PEI_FILE_HANDLE     SaveCurrentFileHandle;
  EFI_FV_FILE_INFO        FvFileInfo;
  PEI_CORE_FV_HANDLE      *CoreFvHandle;
  EFI_HOB_GUID_TYPE       *GuidHob;
  UINT32                  TableSize;

  PeiServices    = (CONST EFI_PEI_SERVICES **)&Private->Ps;
  PeimEntryPoint = NULL;
  PeimFileHandle = NULL;
  EntryPoint     = 0;

  if (Private->DelayedDispatchTable == NULL) {
    GuidHob = GetFirstGuidHob (&gEfiDelayedDispatchTableGuid);
    if (GuidHob != NULL) {
      Private->DelayedDispatchTable = (DELAYED_DISPATCH_TABLE *)(GET_GUID_HOB_DATA (GuidHob));
    } else {
      TableSize                     = sizeof (DELAYED_DISPATCH_TABLE) + ((DELAYED_DISPATCH_MAX_ENTRIES - 1) * sizeof (DELAYED_DISPATCH_ENTRY));
      Private->DelayedDispatchTable = BuildGuidHob (&gEfiDelayedDispatchTableGuid, TableSize);
      if (Private->DelayedDispatchTable != NULL) {
        ZeroMem (Private->DelayedDispatchTable, TableSize);
        Status = PeiServicesInstallPpi (&mDelayedDispatchDesc);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a Failed to install Delayed Dispatch PPI: %r!\n", __func__, Status));
          ASSERT_EFI_ERROR (Status);
        } else {
          Status = PeiServicesNotifyPpi (&mDelayedDispatchNotifyDesc);
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "%a Failed to notify Delayed Dispatch on End of Pei: %r!\n", __func__, Status));
            ASSERT_EFI_ERROR (Status);
          }
        }
      }
    }
  }

  if ((Private->PeiMemoryInstalled) &&
      (PcdGetBool (PcdMigrateTemporaryRamFirmwareVolumes) ||
       (Private->HobList.HandoffInformationTable->BootMode != BOOT_ON_S3_RESUME) ||
       PcdGetBool (PcdShadowPeimOnS3Boot))
      )
  {
    //
    // Once real memory is available, shadow the RegisterForShadow modules. And meanwhile
    // update the modules' status from PEIM_STATE_REGISTER_FOR_SHADOW to PEIM_STATE_DONE.
    //
    SaveCurrentPeimCount  = Private->CurrentPeimCount;
    SaveCurrentFvCount    = Private->CurrentPeimFvCount;
    SaveCurrentFileHandle =  Private->CurrentFileHandle;

    for (Index1 = 0; Index1 < Private->FvCount; Index1++) {
      for (Index2 = 0; Index2 < Private->Fv[Index1].PeimCount; Index2++) {
        if (Private->Fv[Index1].PeimState[Index2] == PEIM_STATE_REGISTER_FOR_SHADOW) {
          PeimFileHandle              = Private->Fv[Index1].FvFileHandles[Index2];
          Private->CurrentFileHandle  = PeimFileHandle;
          Private->CurrentPeimFvCount = Index1;
          Private->CurrentPeimCount   = Index2;
          Status                      = PeiLoadImage (
                                          (CONST EFI_PEI_SERVICES **)&Private->Ps,
                                          PeimFileHandle,
                                          PEIM_STATE_REGISTER_FOR_SHADOW,
                                          &EntryPoint,
                                          &AuthenticationState
                                          );
          if (Status == EFI_SUCCESS) {
            //
            // PEIM_STATE_REGISTER_FOR_SHADOW move to PEIM_STATE_DONE
            //
            Private->Fv[Index1].PeimState[Index2]++;
            //
            // Call the PEIM entry point
            //
            PeimEntryPoint = (EFI_PEIM_ENTRY_POINT2)(UINTN)EntryPoint;

            PERF_START_IMAGE_BEGIN (PeimFileHandle);
            PeimEntryPoint (PeimFileHandle, (const EFI_PEI_SERVICES **)&Private->Ps);
            PERF_START_IMAGE_END (PeimFileHandle);
          }

          //
          // Process the Notify list and dispatch any notifies for
          // newly installed PPIs.
          //
          ProcessDispatchNotifyList (Private);
        }
      }
    }

    Private->CurrentFileHandle  = SaveCurrentFileHandle;
    Private->CurrentPeimFvCount = SaveCurrentFvCount;
    Private->CurrentPeimCount   = SaveCurrentPeimCount;
  }

  //
  // This is the main dispatch loop.  It will search known FVs for PEIMs and
  // attempt to dispatch them.  If any PEIM gets dispatched through a single
  // pass of the dispatcher, it will start over from the BFV again to see
  // if any new PEIMs dependencies got satisfied.  With a well ordered
  // FV where PEIMs are found in the order their dependencies are also
  // satisfied, this dispatcher should run only once.
  //
  do {
    //
    // In case that reenter PeiCore happens, the last pass record is still available.
    //
    if (!Private->PeimDispatcherReenter) {
      Private->PeimNeedingDispatch    = FALSE;
      Private->PeimDispatchOnThisPass = FALSE;
    } else {
      Private->PeimDispatcherReenter = FALSE;
    }

    for (FvCount = Private->CurrentPeimFvCount; FvCount < Private->FvCount; FvCount++) {
      CoreFvHandle = FindNextCoreFvHandle (Private, FvCount);
      ASSERT (CoreFvHandle != NULL);
      if (CoreFvHandle != NULL) {
        //
        // If the FV has corresponding EFI_PEI_FIRMWARE_VOLUME_PPI instance, then dispatch it.
        //
        if (CoreFvHandle->FvPpi == NULL) {
          continue;
        }

        Private->CurrentPeimFvCount = FvCount;

        if (Private->CurrentPeimCount == 0) {
          //
          // When going through each FV, at first, search Apriori file to
          // reorder all PEIMs to ensure the PEIMs in Apriori file to get
          // dispatch at first.
          //
          DiscoverPeimsAndOrderWithApriori (Private, CoreFvHandle);
        }

        //
        // Start to dispatch all modules within the current FV.
        //
        for (PeimCount = Private->CurrentPeimCount;
             PeimCount < Private->Fv[FvCount].PeimCount;
             PeimCount++)
        {
          Private->CurrentPeimCount = PeimCount;
          PeimFileHandle            = Private->CurrentFileHandle = Private->CurrentFvFileHandles[PeimCount];

          if (Private->Fv[FvCount].PeimState[PeimCount] == PEIM_STATE_NOT_DISPATCHED) {
            if (!DepexSatisfied (Private, PeimFileHandle, PeimCount)) {
              Private->PeimNeedingDispatch = TRUE;
            } else {
              Status = CoreFvHandle->FvPpi->GetFileInfo (CoreFvHandle->FvPpi, PeimFileHandle, &FvFileInfo);
              ASSERT_EFI_ERROR (Status);
              if (FvFileInfo.FileType == EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) {
                //
                // For FV type file, Produce new FvInfo PPI and FV HOB
                //
                Status = ProcessFvFile (Private, &Private->Fv[FvCount], PeimFileHandle);
                if (Status == EFI_SUCCESS) {
                  //
                  // PEIM_STATE_NOT_DISPATCHED move to PEIM_STATE_DISPATCHED
                  //
                  Private->Fv[FvCount].PeimState[PeimCount]++;
                  Private->PeimDispatchOnThisPass = TRUE;
                } else {
                  //
                  // The related GuidedSectionExtraction/Decompress PPI for the
                  // encapsulated FV image section may be installed in the rest
                  // of this do-while loop, so need to make another pass.
                  //
                  Private->PeimNeedingDispatch = TRUE;
                }
              } else {
                //
                // For PEIM driver, Load its entry point
                //
                Status = PeiLoadImage (
                           PeiServices,
                           PeimFileHandle,
                           PEIM_STATE_NOT_DISPATCHED,
                           &EntryPoint,
                           &AuthenticationState
                           );
                if (Status == EFI_SUCCESS) {
                  //
                  // The PEIM has its dependencies satisfied, and its entry point
                  // has been found, so invoke it.
                  //
                  PERF_START_IMAGE_BEGIN (PeimFileHandle);

                  REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
                    EFI_PROGRESS_CODE,
                    (EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT_BEGIN),
                    (VOID *)(&PeimFileHandle),
                    sizeof (PeimFileHandle)
                    );

                  Status = VerifyPeim (Private, CoreFvHandle->FvHandle, PeimFileHandle, AuthenticationState);
                  if (Status != EFI_SECURITY_VIOLATION) {
                    //
                    // PEIM_STATE_NOT_DISPATCHED move to PEIM_STATE_DISPATCHED
                    //
                    Private->Fv[FvCount].PeimState[PeimCount]++;
                    //
                    // Call the PEIM entry point for PEIM driver
                    //
                    PeimEntryPoint = (EFI_PEIM_ENTRY_POINT2)(UINTN)EntryPoint;
                    PeimEntryPoint (PeimFileHandle, (const EFI_PEI_SERVICES **)PeiServices);
                    Private->PeimDispatchOnThisPass = TRUE;
                  } else {
                    //
                    // The related GuidedSectionExtraction PPI for the
                    // signed PEIM image section may be installed in the rest
                    // of this do-while loop, so need to make another pass.
                    //
                    Private->PeimNeedingDispatch = TRUE;
                  }

                  REPORT_STATUS_CODE_WITH_EXTENDED_DATA (
                    EFI_PROGRESS_CODE,
                    (EFI_SOFTWARE_PEI_CORE | EFI_SW_PC_INIT_END),
                    (VOID *)(&PeimFileHandle),
                    sizeof (PeimFileHandle)
                    );
                  PERF_START_IMAGE_END (PeimFileHandle);
                }
              }

              PeiCheckAndSwitchStack (SecCoreData, Private);

              //
              // Process the Notify list and dispatch any notifies for
              // newly installed PPIs.
              //
              ProcessDispatchNotifyList (Private);

              //
              // Recheck SwitchStackSignal after ProcessDispatchNotifyList()
              // in case PeiInstallPeiMemory() is done in a callback with
              // EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH.
              //
              PeiCheckAndSwitchStack (SecCoreData, Private);

              if ((Private->PeiMemoryInstalled) && (Private->Fv[FvCount].PeimState[PeimCount] == PEIM_STATE_REGISTER_FOR_SHADOW) &&   \
                  (PcdGetBool (PcdMigrateTemporaryRamFirmwareVolumes) ||
                   (Private->HobList.HandoffInformationTable->BootMode != BOOT_ON_S3_RESUME) ||
                   PcdGetBool (PcdShadowPeimOnS3Boot))
                  )
              {
                //
                // If memory is available we shadow images by default for performance reasons.
                // We call the entry point a 2nd time so the module knows it's shadowed.
                //
                // PERF_START (PeiServices, L"PEIM", PeimFileHandle, 0);
                if ((Private->HobList.HandoffInformationTable->BootMode != BOOT_ON_S3_RESUME) && !PcdGetBool (PcdShadowPeimOnBoot) &&
                    !PcdGetBool (PcdMigrateTemporaryRamFirmwareVolumes))
                {
                  //
                  // Load PEIM into Memory for Register for shadow PEIM.
                  //
                  Status = PeiLoadImage (
                             PeiServices,
                             PeimFileHandle,
                             PEIM_STATE_REGISTER_FOR_SHADOW,
                             &EntryPoint,
                             &AuthenticationState
                             );
                  if (Status == EFI_SUCCESS) {
                    PeimEntryPoint = (EFI_PEIM_ENTRY_POINT2)(UINTN)EntryPoint;
                  }
                }

                ASSERT (PeimEntryPoint != NULL);
                PeimEntryPoint (PeimFileHandle, (const EFI_PEI_SERVICES **)PeiServices);
                // PERF_END (PeiServices, L"PEIM", PeimFileHandle, 0);

                //
                // PEIM_STATE_REGISTER_FOR_SHADOW move to PEIM_STATE_DONE
                //
                Private->Fv[FvCount].PeimState[PeimCount]++;

                //
                // Process the Notify list and dispatch any notifies for
                // newly installed PPIs.
                //
                ProcessDispatchNotifyList (Private);
              }
            }
          }

          // Dispatch pending delalyed dispatch requests
          if (Private->DelayedDispatchTable != NULL) {
            if (DelayedDispatchDispatcher (Private->DelayedDispatchTable, NULL)) {
              ProcessDispatchNotifyList (Private);
            }
          }
        }
      }

      //
      // Before walking through the next FV, we should set them to NULL/0 to
      // start at the beginning of the next FV.
      //
      Private->CurrentFileHandle    = NULL;
      Private->CurrentPeimCount     = 0;
      Private->CurrentFvFileHandles = NULL;
      Private->AprioriCount         = 0;
    }

    //
    // Before making another pass, we should set it to 0 to
    // go through all the FVs.
    //
    Private->CurrentPeimFvCount = 0;

    //
    // PeimNeedingDispatch being TRUE means we found a PEIM/FV that did not get
    //  dispatched. So we need to make another pass
    //
    // PeimDispatchOnThisPass being TRUE means we dispatched a PEIM/FV on this
    //  pass. If we did not dispatch a PEIM/FV there is no point in trying again
    //  as it will fail the next time too (nothing has changed).
    //
    // Also continue dispatch loop if there are outstanding delay-
    // dispatch registrations still running.
  } while ((Private->PeimNeedingDispatch && Private->PeimDispatchOnThisPass) ||
           (Private->DelayedDispatchTable->Count > 0));
}

/**
  Initialize the Dispatcher's data members

  @param PrivateData     PeiCore's private data structure
  @param OldCoreData     Old data from SecCore
                         NULL if being run in non-permanent memory mode.
  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.

  @return None.

**/
VOID
InitializeDispatcherData (
  IN PEI_CORE_INSTANCE           *PrivateData,
  IN PEI_CORE_INSTANCE           *OldCoreData,
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData
  )
{
  if (OldCoreData == NULL) {
    PrivateData->PeimDispatcherReenter = FALSE;
    PeiInitializeFv (PrivateData, SecCoreData);
  } else {
    PeiReinitializeFv (PrivateData);
  }

  return;
}

/**
  This routine parses the Dependency Expression, if available, and
  decides if the module can be executed.


  @param Private         PeiCore's private data structure
  @param FileHandle      PEIM's file handle
  @param PeimCount       Peim count in all dispatched PEIMs.

  @retval TRUE   Can be dispatched
  @retval FALSE  Cannot be dispatched

**/
BOOLEAN
DepexSatisfied (
  IN PEI_CORE_INSTANCE    *Private,
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  IN UINTN                PeimCount
  )
{
  EFI_STATUS        Status;
  VOID              *DepexData;
  EFI_FV_FILE_INFO  FileInfo;

  Status = PeiServicesFfsGetFileInfo (FileHandle, &FileInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_DISPATCH, "Evaluate PEI DEPEX for FFS(Unknown)\n"));
  } else {
    DEBUG ((DEBUG_DISPATCH, "Evaluate PEI DEPEX for FFS(%g)\n", &FileInfo.FileName));
  }

  if (PeimCount < Private->AprioriCount) {
    //
    // If it's in the Apriori file then we set DEPEX to TRUE
    //
    DEBUG ((DEBUG_DISPATCH, "  RESULT = TRUE (Apriori)\n"));
    return TRUE;
  }

  //
  // Depex section not in the encapsulated section.
  //
  Status = PeiServicesFfsFindSectionData (
             EFI_SECTION_PEI_DEPEX,
             FileHandle,
             (VOID **)&DepexData
             );

  if (EFI_ERROR (Status)) {
    //
    // If there is no DEPEX, assume the module can be executed
    //
    DEBUG ((DEBUG_DISPATCH, "  RESULT = TRUE (No DEPEX)\n"));
    return TRUE;
  }

  //
  // Evaluate a given DEPEX
  //
  return PeimDispatchReadiness (&Private->Ps, DepexData);
}

/**
  This routine enables a PEIM to register itself for shadow when the PEI Foundation
  discovers permanent memory.

  @param FileHandle             File handle of a PEIM.

  @retval EFI_NOT_FOUND         The file handle doesn't point to PEIM itself.
  @retval EFI_ALREADY_STARTED   Indicate that the PEIM has been registered itself.
  @retval EFI_SUCCESS           Successfully to register itself.

**/
EFI_STATUS
EFIAPI
PeiRegisterForShadow (
  IN EFI_PEI_FILE_HANDLE  FileHandle
  )
{
  PEI_CORE_INSTANCE  *Private;

  Private = PEI_CORE_INSTANCE_FROM_PS_THIS (GetPeiServicesTablePointer ());

  if (Private->CurrentFileHandle != FileHandle) {
    //
    // The FileHandle must be for the current PEIM
    //
    return EFI_NOT_FOUND;
  }

  if (Private->Fv[Private->CurrentPeimFvCount].PeimState[Private->CurrentPeimCount] >= PEIM_STATE_REGISTER_FOR_SHADOW) {
    //
    // If the PEIM has already entered the PEIM_STATE_REGISTER_FOR_SHADOW or PEIM_STATE_DONE then it's already been started
    //
    return EFI_ALREADY_STARTED;
  }

  Private->Fv[Private->CurrentPeimFvCount].PeimState[Private->CurrentPeimCount] = PEIM_STATE_REGISTER_FOR_SHADOW;

  return EFI_SUCCESS;
}
