/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DataHub.c

Abstract:

  This code produces the Data Hub protocol. It preloads the data hub
  with status information copied in from PEI HOBs.
  
  Only code that implements the Data Hub protocol should go in this file!

  The Term MTC stands for MonoTonicCounter. 

  For more information please look at DataHub.doc

  NOTE: For extra security of the log GetNextDataRecord () could return a copy
        of the data record.
--*/

#include "DataHub.h"

CONST EFI_GUID gZeroGuid  = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

//
// Worker functions private to this file
//
STATIC
DATA_HUB_FILTER_DRIVER  *
FindFilterDriverByEvent (
  IN  LIST_ENTRY      *Head,
  IN  EFI_EVENT       Event
  );

STATIC
EFI_DATA_RECORD_HEADER  *
GetNextDataRecord (
  IN  LIST_ENTRY          *Head,
  IN  UINT64              ClassFilter,
  IN OUT  UINT64          *PtrCurrentMTC
  );

STATIC
EFI_STATUS
EFIAPI
DataHubLogData (
  IN  EFI_DATA_HUB_PROTOCOL   *This,
  IN  EFI_GUID                *DataRecordGuid,
  IN  EFI_GUID                *ProducerName,
  IN  UINT64                  DataRecordClass,
  IN  VOID                    *RawData,
  IN  UINT32                  RawDataSize
  )
/*++

Routine Description:

  Log data record into the data logging hub

Arguments:

  This                - Protocol instance structure

  DataRecordGuid      - GUID that defines record contents

  ProducerName        - GUID that defines the name of the producer of the data

  DataRecordClass     - Class that defines generic record type

  RawData             - Data Log record as defined by DataRecordGuid
  
  RawDataSize         - Size of Data Log data in bytes

Returns: 

  EFI_SUCCESS           - If data was logged

  EFI_OUT_OF_RESOURCES  - If data was not logged due to lack of system 
                           resources.
--*/
{
  EFI_STATUS              Status;
  DATA_HUB_INSTANCE       *Private;
  EFI_DATA_ENTRY          *LogEntry;
  UINT32                  TotalSize;
  UINT32                  RecordSize;
  EFI_DATA_RECORD_HEADER  *Record;
  VOID                    *Raw;
  DATA_HUB_FILTER_DRIVER  *FilterEntry;
  LIST_ENTRY              *Link;
  LIST_ENTRY              *Head;

  Private = DATA_HUB_INSTANCE_FROM_THIS (This);

  //
  // Combine the storage for the internal structs and a copy of the log record.
  //  Record follows PrivateLogEntry. The consumer will be returned a pointer
  //  to Record so we don't what it to be the thing that was allocated from
  //  pool, so the consumer can't free an data record by mistake.
  //
  RecordSize  = sizeof (EFI_DATA_RECORD_HEADER) + RawDataSize;
  TotalSize   = sizeof (EFI_DATA_ENTRY) + RecordSize;

  //
  // The Logging action is the critical section, so it is locked.
  //  The MTC asignment & update, time, and logging must be an
  //  atomic operation, so use the lock.
  //
  Status = EfiAcquireLockOrFail (&Private->DataLock);
  if (EFI_ERROR (Status)) {
    //
    // Reentrancy detected so exit!
    //
    return Status;
  }

  LogEntry = AllocatePool (TotalSize);

  if (LogEntry == NULL) {
    EfiReleaseLock (&Private->DataLock);
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (LogEntry, TotalSize);

  Record  = (EFI_DATA_RECORD_HEADER *) (LogEntry + 1);
  Raw     = (VOID *) (Record + 1);

  //
  // Build Standard Log Header
  //
  Record->Version     = EFI_DATA_RECORD_HEADER_VERSION;
  Record->HeaderSize  = sizeof (EFI_DATA_RECORD_HEADER);
  Record->RecordSize  = RecordSize;
  CopyMem (&Record->DataRecordGuid, DataRecordGuid, sizeof (EFI_GUID));
  CopyMem (&Record->ProducerName, ProducerName, sizeof (EFI_GUID));
  Record->DataRecordClass   = DataRecordClass;

  Record->LogMonotonicCount = Private->GlobalMonotonicCount++;

  gRT->GetTime (&Record->LogTime, NULL);

  //
  // Insert log into the internal linked list.
  //
  LogEntry->Signature   = EFI_DATA_ENTRY_SIGNATURE;
  LogEntry->Record      = Record;
  LogEntry->RecordSize  = sizeof (EFI_DATA_ENTRY) + RawDataSize;
  InsertTailList (&Private->DataListHead, &LogEntry->Link);

  CopyMem (Raw, RawData, RawDataSize);

  EfiReleaseLock (&Private->DataLock);

  //
  // Send Signal to all the filter drivers which are interested
  //  in the record's class and guid.
  //
  Head = &Private->FilterDriverListHead;
  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    FilterEntry = FILTER_ENTRY_FROM_LINK (Link);
    if (((FilterEntry->ClassFilter & DataRecordClass) != 0) &&
        (CompareGuid (&FilterEntry->FilterDataRecordGuid, &gZeroGuid) || 
         CompareGuid (&FilterEntry->FilterDataRecordGuid, DataRecordGuid))) {
      gBS->SignalEvent (FilterEntry->Event);
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DataHubGetNextRecord (
  IN EFI_DATA_HUB_PROTOCOL            *This,
  IN OUT UINT64                       *MonotonicCount,
  IN EFI_EVENT                        *FilterDriverEvent, OPTIONAL
  OUT EFI_DATA_RECORD_HEADER          **Record
  )
/*++

Routine Description:

  Get a previously logged data record and the MonotonicCount for the next
  availible Record. This allows all records or all records later 
  than a give MonotonicCount to be returned. If an optional FilterDriverEvent
  is passed in with a MonotonicCout of zero return the first record 
  not yet read by the filter driver. If FilterDriverEvent is NULL and 
  MonotonicCount is zero return the first data record.

Arguments:

  This              - The EFI_DATA_HUB_PROTOCOL instance.
  MonotonicCount    - Specifies the Record to return. On input, zero means
                      return the first record. On output, contains the next
                      record to availible. Zero indicates no more records.
  FilterDriverEvent - If FilterDriverEvent is not passed in a MonotonicCount 
                      of zero, it means to return the first data record. 
                      If FilterDriverEvent is passed in, then a MonotonicCount 
                      of zero means to return the first data not yet read by 
                      FilterDriverEvent.
  Record            - Returns a dynamically allocated memory buffer with a data 
                      record that matches MonotonicCount.

Returns: 

  EFI_SUCCESS             - Data was returned in Record.
  EFI_INVALID_PARAMETER   - FilterDriverEvent was passed in but does not exist.
  EFI_NOT_FOUND           - MonotonicCount does not match any data record in the
                            system. If a MonotonicCount of zero was passed in, then
                            no data records exist in the system.
  EFI_OUT_OF_RESOURCES    - Record was not returned due to lack of system resources.

--*/
{
  DATA_HUB_INSTANCE       *Private;
  DATA_HUB_FILTER_DRIVER  *FilterDriver;
  UINT64                  ClassFilter;
  UINT64                  FilterMonotonicCount;

  Private               = DATA_HUB_INSTANCE_FROM_THIS (This);

  FilterDriver          = NULL;
  FilterMonotonicCount  = 0;
  ClassFilter = EFI_DATA_RECORD_CLASS_DEBUG |
    EFI_DATA_RECORD_CLASS_ERROR |
    EFI_DATA_RECORD_CLASS_DATA |
    EFI_DATA_RECORD_CLASS_PROGRESS_CODE;

  if (FilterDriverEvent != NULL) {
    //
    // For events the beginning is the last unread record. This info is
    // stored in the instance structure, so we must look up the event
    // to get the data.
    //
    FilterDriver = FindFilterDriverByEvent (
                    &Private->FilterDriverListHead,
                    *FilterDriverEvent
                    );
    if (FilterDriver == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // Use the Class filter the event was created with.
    //
    ClassFilter = FilterDriver->ClassFilter;

    if (*MonotonicCount == 0) {
      //
      // Use the MTC from the Filter Driver.
      //
      FilterMonotonicCount = FilterDriver->GetNextMonotonicCount;
      if (FilterMonotonicCount != 0) {
        //
        // The GetNextMonotonicCount field remembers the last value from the previous time.
        // But we already processed this vaule, so we need to find the next one. So if
        // It is not the first time get the new record entry.
        //
        *Record         = GetNextDataRecord (&Private->DataListHead, ClassFilter, &FilterMonotonicCount);
        *MonotonicCount = FilterMonotonicCount;
        if (FilterMonotonicCount == 0) {
          //
          // If there is no new record to get exit now.
          //
          return EFI_NOT_FOUND;
        }
      }
    }
  }
  //
  // Return the record
  //
  *Record = GetNextDataRecord (&Private->DataListHead, ClassFilter, MonotonicCount);
  if (*Record == NULL) {
    return EFI_NOT_FOUND;
  }

  if (FilterDriver != NULL) {
    //
    // If we have a filter driver update the records that have been read.
    // If MonotonicCount is zero No more reacords left.
    //
    if (*MonotonicCount == 0) {
      if (FilterMonotonicCount != 0) {
        //
        // Return the result of our extra GetNextDataRecord.
        //
        FilterDriver->GetNextMonotonicCount = FilterMonotonicCount;
      }
    } else {
      //
      // Point to next undread record
      //
      FilterDriver->GetNextMonotonicCount = *MonotonicCount;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DataHubRegisterFilterDriver (
  IN EFI_DATA_HUB_PROTOCOL    * This,
  IN EFI_EVENT                FilterEvent,
  IN EFI_TPL                  FilterTpl,
  IN UINT64                   FilterClass,
  IN EFI_GUID                 * FilterDataRecordGuid OPTIONAL
  )
/*++

Routine Description:

  This function registers the data hub filter driver that is represented 
  by FilterEvent. Only one instance of each FilterEvent can be registered.
  After the FilterEvent is registered, it will be signaled so it can sync 
  with data records that have been recorded prior to the FilterEvent being 
  registered.
    
Arguments:

  This                  - The EFI_DATA_HUB_PROTOCOL instance.
  FilterEvent           - The EFI_EVENT to signal whenever data that matches 
                          FilterClass is logged in the system.
  FilterTpl             - The maximum EFI_TPL at which FilterEvent can be 
                          signaled. It is strongly recommended that you use the 
                          lowest EFI_TPL possible.
  FilterClass           - FilterEvent will be signaled whenever a bit in 
                          EFI_DATA_RECORD_HEADER.DataRecordClass is also set in 
                          FilterClass. If FilterClass is zero, no class-based 
                          filtering will be performed.
  FilterDataRecordGuid  - FilterEvent will be signaled whenever FilterDataRecordGuid 
                          matches EFI_DATA_RECORD_HEADER.DataRecordGuid. If 
                          FilterDataRecordGuid is NULL, then no GUID-based filtering 
                          will be performed.              
Returns: 

  EFI_SUCCESS             - The filter driver event was registered.
  EFI_ALREADY_STARTED     - FilterEvent was previously registered and cannot be 
                            registered again.
  EFI_OUT_OF_RESOURCES    - The filter driver event was not registered due to lack of 
                            system resources.

--*/
{
  DATA_HUB_INSTANCE       *Private;
  DATA_HUB_FILTER_DRIVER  *FilterDriver;

  Private       = DATA_HUB_INSTANCE_FROM_THIS (This);

  FilterDriver  = (DATA_HUB_FILTER_DRIVER *) AllocateZeroPool (sizeof (DATA_HUB_FILTER_DRIVER));
  if (FilterDriver == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Initialize filter driver info
  //
  FilterDriver->Signature             = EFI_DATA_HUB_FILTER_DRIVER_SIGNATURE;
  FilterDriver->Event                 = FilterEvent;
  FilterDriver->Tpl                   = FilterTpl;
  FilterDriver->GetNextMonotonicCount = 0;
  if (FilterClass == 0) {
    FilterDriver->ClassFilter = EFI_DATA_RECORD_CLASS_DEBUG |
      EFI_DATA_RECORD_CLASS_ERROR |
      EFI_DATA_RECORD_CLASS_DATA |
      EFI_DATA_RECORD_CLASS_PROGRESS_CODE;
  } else {
    FilterDriver->ClassFilter = FilterClass;
  }

  if (FilterDataRecordGuid != NULL) {
    CopyMem (&FilterDriver->FilterDataRecordGuid, FilterDataRecordGuid, sizeof (EFI_GUID));
  }
  //
  // Search for duplicate entries
  //
  if (FindFilterDriverByEvent (&Private->FilterDriverListHead, FilterEvent) != NULL) {
    FreePool (FilterDriver);
    return EFI_ALREADY_STARTED;
  }
  //
  // Make insertion an atomic operation with the lock.
  //
  EfiAcquireLock (&Private->DataLock);
  InsertTailList (&Private->FilterDriverListHead, &FilterDriver->Link);
  EfiReleaseLock (&Private->DataLock);

  //
  // Signal the Filter driver we just loaded so they will recieve all the
  // previous history. If we did not signal here we would have to wait until
  // the next data was logged to get the history. In a case where no next
  // data was logged we would never get synced up.
  //
  gBS->SignalEvent (FilterEvent);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
DataHubUnregisterFilterDriver (
  IN EFI_DATA_HUB_PROTOCOL    *This,
  IN EFI_EVENT                FilterEvent
  )
/*++

Routine Description:

  Remove a Filter Driver, so it no longer gets called when data 
   information is logged.

Arguments:

  This        - Protocol instance structure

  FilterEvent - Event that represents a filter driver that is to be 
                 Unregistered.

Returns: 

  EFI_SUCCESS   - If FilterEvent was unregistered

  EFI_NOT_FOUND - If FilterEvent does not exist

--*/
{
  DATA_HUB_INSTANCE       *Private;
  DATA_HUB_FILTER_DRIVER  *FilterDriver;

  Private = DATA_HUB_INSTANCE_FROM_THIS (This);

  //
  // Search for duplicate entries
  //
  FilterDriver = FindFilterDriverByEvent (
                  &Private->FilterDriverListHead,
                  FilterEvent
                  );
  if (FilterDriver == NULL) {
    return EFI_NOT_FOUND;
  }
  //
  // Make removal an atomic operation with the lock
  //
  EfiAcquireLock (&Private->DataLock);
  RemoveEntryList (&FilterDriver->Link);
  EfiReleaseLock (&Private->DataLock);

  return EFI_SUCCESS;
}
//
// STATIC Worker fucntions follow
//
STATIC
DATA_HUB_FILTER_DRIVER *
FindFilterDriverByEvent (
  IN  LIST_ENTRY      *Head,
  IN  EFI_EVENT       Event
  )
/*++

Routine Description:
  Search the Head list for a EFI_DATA_HUB_FILTER_DRIVER member that
   represents Event and return it.

Arguments:

  Head  - Head of dual linked list of EFI_DATA_HUB_FILTER_DRIVER
           structures.

  Event - Event to be search for in the Head list.

Returns: 

  EFI_DATA_HUB_FILTER_DRIVER - Returned if Event stored in the
                               Head doubly linked list.

  NULL - If Event is not in the list

--*/
{
  DATA_HUB_FILTER_DRIVER  *FilterEntry;
  LIST_ENTRY              *Link;

  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    FilterEntry = FILTER_ENTRY_FROM_LINK (Link);
    if (FilterEntry->Event == Event) {
      return FilterEntry;
    }
  }

  return NULL;
}

STATIC
EFI_DATA_RECORD_HEADER *
GetNextDataRecord (
  IN  LIST_ENTRY          *Head,
  IN  UINT64              ClassFilter,
  IN OUT  UINT64          *PtrCurrentMTC
  )
/*++

Routine Description:
  Search the Head doubly linked list for the passed in MTC. Return the 
   matching element in Head and the MTC on the next entry.

Arguments:

  Head          - Head of Data Log linked list.

  ClassFilter   - Only match the MTC if it is in the same Class as the
                  ClassFilter.

  PtrCurrentMTC - On IN contians MTC to search for. On OUT contians next
                   MTC in the data log list or zero if at end of the list.
  
Returns:

  EFI_DATA_LOG_ENTRY - Return pointer to data log data from Head list.

  NULL - If no data record exists.

--*/
{
  EFI_DATA_ENTRY          *LogEntry;
  LIST_ENTRY              *Link;
  BOOLEAN                 ReturnFirstEntry;
  EFI_DATA_RECORD_HEADER  *Record;
  EFI_DATA_ENTRY          *NextLogEntry;

  //
  // If MonotonicCount == 0 just return the first one
  //
  ReturnFirstEntry  = (BOOLEAN) (*PtrCurrentMTC == 0);

  Record            = NULL;
  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    LogEntry = DATA_ENTRY_FROM_LINK (Link);
    if ((LogEntry->Record->DataRecordClass & ClassFilter) == 0) {
      //
      // Skip any entry that does not have the correct ClassFilter
      //
      continue;
    }

    if ((LogEntry->Record->LogMonotonicCount == *PtrCurrentMTC) || ReturnFirstEntry) {
      //
      // Return record to the user
      //
      Record = LogEntry->Record;

      //
      // Calculate the next MTC value. If there is no next entry set
      // MTC to zero.
      //
      *PtrCurrentMTC = 0;
      for (Link = Link->ForwardLink; Link != Head; Link = Link->ForwardLink) {
        NextLogEntry = DATA_ENTRY_FROM_LINK (Link);
        if ((NextLogEntry->Record->DataRecordClass & ClassFilter) != 0) {
          //
          // Return the MTC of the next thing to search for if found
          //
          *PtrCurrentMTC = NextLogEntry->Record->LogMonotonicCount;
          break;
        }
      }
      //
      // Record found exit loop and return
      //
      break;
    }
  }

  return Record;
}
//
// Module Global:
//  Since this driver will only ever produce one instance of the Logging Hub
//  protocol you are not required to dynamically allocate the PrivateData.
//
DATA_HUB_INSTANCE mPrivateData;

EFI_STATUS
EFIAPI
DataHubInstall (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:
  Install Driver to produce Data Hub protocol. 

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns: 

  EFI_SUCCESS - Logging Hub protocol installed

  Other       - No protocol installed, unload driver.

--*/
{
  EFI_STATUS  Status;
  UINT32      HighMontonicCount;

  mPrivateData.Signature                      = DATA_HUB_INSTANCE_SIGNATURE;
  mPrivateData.DataHub.LogData                = DataHubLogData;
  mPrivateData.DataHub.GetNextRecord          = DataHubGetNextRecord;
  mPrivateData.DataHub.RegisterFilterDriver   = DataHubRegisterFilterDriver;
  mPrivateData.DataHub.UnregisterFilterDriver = DataHubUnregisterFilterDriver;

  //
  // Initialize Private Data in CORE_LOGGING_HUB_INSTANCE that is
  //  required by this protocol
  //
  InitializeListHead (&mPrivateData.DataListHead);
  InitializeListHead (&mPrivateData.FilterDriverListHead);

  EfiInitializeLock (&mPrivateData.DataLock, TPL_NOTIFY);

  //
  // Make sure we get a bigger MTC number on every boot!
  //
  Status = gRT->GetNextHighMonotonicCount (&HighMontonicCount);
  if (EFI_ERROR (Status)) {
    //
    // if system service fails pick a sane value.
    //
    mPrivateData.GlobalMonotonicCount = 0;
  } else {
    mPrivateData.GlobalMonotonicCount = LShiftU64 ((UINT64) HighMontonicCount, 32);
  }
  //
  // Make a new handle and install the protocol
  //
  mPrivateData.Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &mPrivateData.Handle,
                  &gEfiDataHubProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPrivateData.DataHub
                  );
  return Status;
}
