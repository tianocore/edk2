/** @file
  Thunk driver's entry that install filter for DataRecord.
  
Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Thunk.h"

//
// Global variables
//
LIST_ENTRY  mStructureList;

/**
  Entry Point of thunk driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to EFI system table.

  @retval EFI_SUCCESS          The event handlers were registered.
  @retval EFI_DEVICE_ERROR     Failed to register the event handlers
**/  
EFI_STATUS
EFIAPI
ThunkEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_DATA_HUB_PROTOCOL *DataHub;
  EFI_EVENT             FilterEvent;
    
  Status = gBS->LocateProtocol (&gEfiDataHubProtocolGuid, NULL, (VOID**) &DataHub);
  ASSERT_EFI_ERROR (Status);
  ASSERT (DataHub != NULL);

  InitializeListHead (&mStructureList);

  //
  // Register SmBios Data Filter Function.
  // This function is notified at TPL_CALLBACK.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SmbiosDataFilter,
                  NULL,
                  &FilterEvent
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DataHub->RegisterFilterDriver (
                      DataHub,
                      FilterEvent,
                      TPL_APPLICATION,
                      EFI_DATA_RECORD_CLASS_DATA,
                      NULL
                      );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (FilterEvent);
    return Status;
  }

  return Status;

} 

/**
  Smbios data filter function. This function is invoked when there is data records
  available in the Data Hub. 

  @param Event         The event that is signaled.
  @param Context       not used here.
**/
VOID
EFIAPI
SmbiosDataFilter (
  IN EFI_EVENT    Event,
  IN VOID         *Context
  )
{
  EFI_STATUS              Status;
  EFI_DATA_HUB_PROTOCOL   *DataHub;
  EFI_HANDLE              DataHubHandle;
  UINTN                   HandleSize;
  UINT64                  MonotonicCount;
  EFI_DATA_RECORD_HEADER  *Record;

  Status  = EFI_SUCCESS;
  DataHub = NULL;

  //
  // Get the Data Hub Protocol. Assume only one instance
  // of Data Hub Protocol is availabe in the system.
  //
  HandleSize = sizeof (EFI_HANDLE);

  Status = gBS->LocateHandle (
                  ByProtocol,
                  &gEfiDataHubProtocolGuid,
                  NULL,
                  &HandleSize,
                  &DataHubHandle
                  );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = gBS->HandleProtocol (
                  DataHubHandle,
                  &gEfiDataHubProtocolGuid,
                  (VOID **) &DataHub
                  );

  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Get all available data records from data hub
  //
  MonotonicCount  = 0;
  Record          = NULL;

  do {

    Status = DataHub->GetNextRecord (
                        DataHub,
                        &MonotonicCount,
                        &Event,
                        &Record
                        );

    if (!EFI_ERROR (Status)) {
      if (Record->DataRecordClass == EFI_DATA_RECORD_CLASS_DATA) {
      
        //
        // It's of expected Data Type. Process it.
        //
        SmbiosProcessDataRecord (Record);
      }
    }
  } while (!EFI_ERROR (Status) && (MonotonicCount != 0));

Done:

  return ;

}
