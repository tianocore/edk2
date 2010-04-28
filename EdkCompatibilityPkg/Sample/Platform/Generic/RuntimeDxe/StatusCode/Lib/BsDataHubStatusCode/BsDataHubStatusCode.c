/*++

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BsDataHubStatusCode.c

Abstract:

  This implements a status code listener that logs status codes into the data
  hub.  This is only active during non-runtime DXE.
  The status codes are recorded in a extensiable buffer, and a event is signalled 
  to log them to the data hub. The recorder is the producer of the status code in
  buffer and the event notify function the consummer.

--*/

#include "BsDataHubStatusCode.h"

//
// Initialize FIFO to cache records.
//
STATIC EFI_LIST_ENTRY   mRecordsFifo    = INITIALIZE_LIST_HEAD_VARIABLE (mRecordsFifo);
STATIC EFI_LIST_ENTRY   mRecordsBuffer  = INITIALIZE_LIST_HEAD_VARIABLE (mRecordsBuffer);
STATIC EFI_EVENT        mLogDataHubEvent;
STATIC BOOLEAN          mEventHandlerActive = FALSE;

//
// Cache data hub protocol.
//
STATIC EFI_DATA_HUB_PROTOCOL     *mDataHubProtocol;

STATIC
DATA_HUB_STATUS_CODE_DATA_RECORD *
AcquireRecordBuffer (
  VOID
  )
/*++

Routine Description:

  Return one DATAHUB_STATUSCODE_RECORD space.
  The size of free record pool would be extend, if the pool is empty.

Arguments:

  None

Returns:

  A pointer to the new allocated node or NULL if non available

--*/
{
  DATAHUB_STATUSCODE_RECORD *Record;
  EFI_TPL                   CurrentTpl;
  EFI_LIST_ENTRY            *Node;
  UINT32                    Index;

  Record     = NULL;
  CurrentTpl = gBS->RaiseTPL (EFI_TPL_HIGH_LEVEL);

  if (!IsListEmpty (&mRecordsBuffer)) {
    Node = GetFirstNode (&mRecordsBuffer);
    RemoveEntryList (Node);

    Record = _CR (Node, DATAHUB_STATUSCODE_RECORD, Node);
  } else {
    if (CurrentTpl > EFI_TPL_NOTIFY) {
      gBS->RestoreTPL (CurrentTpl);
      return NULL;
    }

    gBS->RestoreTPL (CurrentTpl);
    
    gBS->AllocatePool (EfiBootServicesData, sizeof (DATAHUB_STATUSCODE_RECORD) * 16, (VOID **) &Record);
    if (Record == NULL) {
      return NULL;
    }
    EfiCommonLibZeroMem (Record, sizeof (DATAHUB_STATUSCODE_RECORD) * 16);    


    CurrentTpl = gBS->RaiseTPL (EFI_TPL_HIGH_LEVEL);
    for (Index = 1; Index < 16; Index++) {
      InsertTailList (&mRecordsBuffer, &Record[Index].Node);
    }
  }

  Record->Signature = BS_DATA_HUB_STATUS_CODE_SIGNATURE;
  InsertTailList (&mRecordsFifo, &Record->Node);

  gBS->RestoreTPL (CurrentTpl);

  return (DATA_HUB_STATUS_CODE_DATA_RECORD *) (Record->Data);
}

STATIC
DATA_HUB_STATUS_CODE_DATA_RECORD *
RetrieveRecord (
  VOID
  )
/*++

Routine Description:

  Retrieve one record from Records FIFO. The record would be removed from FIFO and 
  release to free record buffer.

Arguments:

  None

Returns:

  Point to record which is ready to be logged, or NULL if the FIFO of record is empty.

--*/  
{
  DATA_HUB_STATUS_CODE_DATA_RECORD  *RecordData;
  DATAHUB_STATUSCODE_RECORD   *Record;
  EFI_LIST_ENTRY              *Node;
  EFI_TPL                     CurrentTpl;
  
  RecordData = NULL;
  
  CurrentTpl = gBS->RaiseTPL (EFI_TPL_HIGH_LEVEL);

  if (!IsListEmpty (&mRecordsFifo)) {
    Node = GetFirstNode (&mRecordsFifo);
    Record = CR (Node, DATAHUB_STATUSCODE_RECORD, Node, BS_DATA_HUB_STATUS_CODE_SIGNATURE);

    RemoveEntryList (&Record->Node);
    InsertTailList (&mRecordsBuffer, &Record->Node);
    Record->Signature = 0;
    RecordData = (DATA_HUB_STATUS_CODE_DATA_RECORD *) Record->Data;
  }

  gBS->RestoreTPL (CurrentTpl);

  return RecordData;
}

EFI_STATUS
EFIAPI
BsDataHubReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
/*++

Routine Description:

  Boot service report status code listener.  This function logs the status code
  into the data hub.

Arguments:

  Same as gRT->ReportStatusCode (See Tiano Runtime Specification)

Returns:

  None

--*/
{
  DATA_HUB_STATUS_CODE_DATA_RECORD  *Record;
  UINT32                     ErrorLevel;
  VA_LIST                    Marker;
  CHAR8                      *Format;
  CHAR16                     FormatBuffer[BYTES_PER_RECORD];
  UINTN                      Index;

  //
  // See whether in runtime phase or not.
  //
  if (EfiAtRuntime ()) {
    //
    // For now all we do is post code at runtime
    //
    return EFI_SUCCESS;
  }

  //
  // Discard new DataHubRecord caused by DataHub->LogData()
  //
  if (mEventHandlerActive) {
    return EFI_SUCCESS;
  }

  Record = AcquireRecordBuffer ();
  if (Record == NULL) {
    //
    // There are no empty record buffer in private buffers
    //
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Construct Data Hub Extended Data
  //
  Record->CodeType = CodeType;
  Record->Value    = Value;
  Record->Instance = Instance;

  if (CallerId != NULL) {
    EfiCopyMem (&Record->CallerId, CallerId, sizeof (EFI_GUID));
  }

  if (Data != NULL) {
    if (ReportStatusCodeExtractDebugInfo (Data, &ErrorLevel, &Marker, &Format)) {
      //
      // Convert Ascii Format string to Unicode.
      //
      for (Index = 0; Format[Index] != '\0' && Index < (BYTES_PER_RECORD - 1); Index += 1) {
        FormatBuffer[Index] = (CHAR16) Format[Index];
      }

      FormatBuffer[Index] = L'\0';

      //
      // Put processed string into the buffer
      //
      Index = VSPrint (
                (CHAR16 *) (Record + 1),
                BYTES_PER_RECORD - (sizeof (DATA_HUB_STATUS_CODE_DATA_RECORD)),
                FormatBuffer,
                Marker
                );
                
      EfiCopyMem (&Record->Data.Type, &gEfiStatusCodeDataTypeDebugGuid, sizeof (EFI_GUID));
      Record->Data.HeaderSize = Data->HeaderSize;
      Record->Data.Size = (UINT16) (Index * sizeof (CHAR16));
    } else {
      //
      // Copy status code data header
      //
      EfiCopyMem (&Record->Data, Data, sizeof (EFI_STATUS_CODE_DATA));

      if (Data->Size > BYTES_PER_RECORD - sizeof (DATA_HUB_STATUS_CODE_DATA_RECORD)) {
        Record->Data.Size = (UINT16) (BYTES_PER_RECORD - sizeof (DATA_HUB_STATUS_CODE_DATA_RECORD));
      } 
      EfiCopyMem ((VOID *) (Record + 1), Data + 1, Record->Data.Size);
    }
  }

  gBS->SignalEvent (mLogDataHubEvent);

  return EFI_SUCCESS;
}

VOID
EFIAPI
LogDataHubEventHandler (
  IN  EFI_EVENT     Event,
  IN  VOID          *Context
  )
/*++

Routine Description:

  The Event handler which will be notified to log data in Data Hub.

Arguments:

  Event     -   Instance of the EFI_EVENT to signal whenever data is
                available to be logged in the system.
  Context   -   Context of the event.

Returns:

  None.

--*/
{
  DATA_HUB_STATUS_CODE_DATA_RECORD  *Record;
  UINT32                            Size;
  UINT64                            DataRecordClass;

  //
  // Set global flag so we don't recurse if DataHub->LogData eventually causes new DataHubRecord
  //
  mEventHandlerActive = TRUE;
  
  //
  // Log DataRecord in Data Hub.
  // Journal records fifo to find all record entry.
  //
  while (1) {
    Record = RetrieveRecord ();
    if (Record == NULL) {
      break;
    }
    //
    // Add in the size of the header we added.
    //
    Size = sizeof (DATA_HUB_STATUS_CODE_DATA_RECORD) + (UINT32) Record->Data.Size;

    if ((Record->CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) {
      DataRecordClass = EFI_DATA_RECORD_CLASS_PROGRESS_CODE;
    } else if ((Record->CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) {
      DataRecordClass = EFI_DATA_RECORD_CLASS_ERROR;
    } else if ((Record->CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_DEBUG_CODE) {
      DataRecordClass = EFI_DATA_RECORD_CLASS_DEBUG;
    } else {
      //
      // Should never get here.
      //
      DataRecordClass = EFI_DATA_RECORD_CLASS_DEBUG |
        EFI_DATA_RECORD_CLASS_ERROR |
        EFI_DATA_RECORD_CLASS_DATA |
        EFI_DATA_RECORD_CLASS_PROGRESS_CODE;
    }

    //
    // Log DataRecord in Data Hub
    //
    
    mDataHubProtocol->LogData (
                        mDataHubProtocol,
                        &gEfiStatusCodeGuid,
                        &gEfiStatusCodeRuntimeProtocolGuid,
                        DataRecordClass,
                        Record,
                        Size
                        );

  }

  mEventHandlerActive = FALSE;

}

EFI_STATUS
EFIAPI
BsDataHubInitializeStatusCode (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  Install a data hub listener.

Arguments:

  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:

  EFI_SUCCESS - Logging Hub protocol installed
  Other       - No protocol installed, unload driver.

--*/
{
  EFI_STATUS              Status;

  Status = gBS->LocateProtocol (
                  &gEfiDataHubProtocolGuid, 
                  NULL, 
                  (VOID **) &mDataHubProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Create a Notify Event to log data in Data Hub
  //
  Status = gBS->CreateEvent (
                  EFI_EVENT_NOTIFY_SIGNAL,
                  EFI_TPL_CALLBACK,
                  LogDataHubEventHandler,
                  NULL,
                  &mLogDataHubEvent
                  );

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}


