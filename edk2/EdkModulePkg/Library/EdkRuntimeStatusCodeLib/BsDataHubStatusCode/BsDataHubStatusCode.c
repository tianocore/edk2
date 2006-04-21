/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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

--*/
#include "BsDataHubStatusCode.h"

//
// Globals only work at BootService Time. NOT at Runtime!
//
static EFI_DATA_HUB_PROTOCOL  *mDataHub;
static LIST_ENTRY             mRecordBuffer;
static INTN                   mRecordNum;
static EFI_EVENT              mLogDataHubEvent;
static EFI_LOCK               mStatusCodeReportLock;
static BOOLEAN                mEventHandlerActive   = FALSE;

STATUS_CODE_RECORD_LIST *
GetRecordBuffer (
  VOID
  )
/*++

Routine Description:

  Returned buffer of length BYTES_PER_RECORD

Arguments:

  None

Returns:

  Entry in mRecordBuffer or NULL if non available

--*/
{
  STATUS_CODE_RECORD_LIST *Buffer;

  gBS->AllocatePool (EfiBootServicesData, sizeof (STATUS_CODE_RECORD_LIST), (VOID **) &Buffer);
  if (Buffer == NULL) {
    return NULL;
  }

  ZeroMem (Buffer, sizeof (STATUS_CODE_RECORD_LIST));
  Buffer->Signature = BS_DATA_HUB_STATUS_CODE_SIGNATURE;

  return Buffer;
}

DATA_HUB_STATUS_CODE_DATA_RECORD *
AquireEmptyRecordBuffer (
  VOID
  )
/*++

Routine Description:

  Allocate a mRecordBuffer entry in the form of a pointer.

Arguments:

  None

Returns:

  Pointer to new buffer. NULL if none exist.

--*/
{
  STATUS_CODE_RECORD_LIST *DataBuffer;

  if (mRecordNum < MAX_RECORD_NUM) {
    DataBuffer = GetRecordBuffer ();
    if (DataBuffer != NULL) {
      EfiAcquireLock (&mStatusCodeReportLock);
      InsertTailList (&mRecordBuffer, &DataBuffer->Link);
      mRecordNum++;
      EfiReleaseLock (&mStatusCodeReportLock);
      return (DATA_HUB_STATUS_CODE_DATA_RECORD *) DataBuffer->RecordBuffer;
    }
  }

  return NULL;
}

EFI_STATUS
ReleaseRecordBuffer (
  IN  STATUS_CODE_RECORD_LIST  *RecordBuffer
  )
/*++

Routine Description:

  Release a mRecordBuffer entry allocated by AquireEmptyRecordBuffer ().

Arguments:

  RecordBuffer          - Data to free

Returns:

  EFI_SUCCESS           - If DataRecord is valid
  EFI_UNSUPPORTED       - The record list has empty

--*/
{
  ASSERT (RecordBuffer != NULL);
  if (mRecordNum <= 0) {
    return EFI_UNSUPPORTED;
  }

  EfiAcquireLock (&mStatusCodeReportLock);
  RemoveEntryList (&RecordBuffer->Link);
  mRecordNum--;
  EfiReleaseLock (&mStatusCodeReportLock);
  gBS->FreePool (RecordBuffer);
  return EFI_SUCCESS;
}

EFI_STATUS
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

  Same as ReportStatusCode (See Tiano Runtime Specification)

Returns:

  None

--*/
{
  DATA_HUB_STATUS_CODE_DATA_RECORD  *DataHub;
  UINT32                            ErrorLevel;
  VA_LIST                           Marker;
  CHAR8                             *Format;
  UINTN                             Index;
  CHAR16                            FormatBuffer[BYTES_PER_RECORD];

  if (EfiAtRuntime ()) {
    //
    // For now all we do is post code at runtime
    //
    return EFI_SUCCESS;
  }
  //
  // If we had an error while in our event handler, then do nothing so
  // that we don't get in an endless loop.
  //
  if (mEventHandlerActive) {
    return EFI_SUCCESS;
  }

  DataHub = (DATA_HUB_STATUS_CODE_DATA_RECORD *) AquireEmptyRecordBuffer ();
  if (DataHub == NULL) {
    //
    // There are no empty record buffer in private buffers
    //
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Construct Data Hub Extended Data
  //
  DataHub->CodeType = CodeType;
  DataHub->Value    = Value;
  DataHub->Instance = Instance;

  if (CallerId != NULL) {
    CopyMem (&DataHub->CallerId, CallerId, sizeof (EFI_GUID));
  } else {
    ZeroMem (&DataHub->CallerId, sizeof (EFI_GUID));
  }

  if (Data == NULL) {
    ZeroMem (&DataHub->Data, sizeof (EFI_STATUS_CODE_DATA));
  } else {
    //
    // Copy generic Header
    //
    CopyMem (&DataHub->Data, Data, sizeof (EFI_STATUS_CODE_DATA));

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
      Index = UnicodeVSPrint (
                (CHAR16 *) (DataHub + 1),
                BYTES_PER_RECORD - (sizeof (DATA_HUB_STATUS_CODE_DATA_RECORD)),
                FormatBuffer,
                Marker
                );

      //
      // DATA_HUB_STATUS_CODE_DATA_RECORD followed by VSPrint String Buffer
      //
      DataHub->Data.Size = (UINT16) (Index * sizeof (CHAR16));

    } else {
      //
      // Default behavior is to copy optional data
      //
      if (Data->Size > (BYTES_PER_RECORD - sizeof (DATA_HUB_STATUS_CODE_DATA_RECORD))) {
        DataHub->Data.Size = (UINT16) (BYTES_PER_RECORD - sizeof (DATA_HUB_STATUS_CODE_DATA_RECORD));
      }

      CopyMem (DataHub + 1, Data + 1, DataHub->Data.Size);
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
  EFI_STATUS                        Status;
  DATA_HUB_STATUS_CODE_DATA_RECORD  *DataRecord;
  UINTN                             Size;
  UINT64                            DataRecordClass;
  LIST_ENTRY                        *Link;
  STATUS_CODE_RECORD_LIST           *BufferEntry;

  //
  // Set our global flag so we don't recurse if we get an error here.
  //
  mEventHandlerActive = TRUE;

  //
  // Log DataRecord in Data Hub.
  // If there are multiple DataRecords, Log all of them.
  //
  for (Link = mRecordBuffer.ForwardLink; Link != &mRecordBuffer;) {
    BufferEntry = CR (Link, STATUS_CODE_RECORD_LIST, Link, BS_DATA_HUB_STATUS_CODE_SIGNATURE);
    DataRecord  = (DATA_HUB_STATUS_CODE_DATA_RECORD *) (BufferEntry->RecordBuffer);
    Link        = Link->ForwardLink;

    //
    // Add in the size of the header we added.
    //
    Size = sizeof (DATA_HUB_STATUS_CODE_DATA_RECORD) + DataRecord->Data.Size;

    if ((DataRecord->CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) {
      DataRecordClass = EFI_DATA_RECORD_CLASS_PROGRESS_CODE;
    } else if ((DataRecord->CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) {
      DataRecordClass = EFI_DATA_RECORD_CLASS_ERROR;
    } else if ((DataRecord->CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_DEBUG_CODE) {
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

    if (((DataRecord->Instance & EFI_D_ERROR) != 0) &&
        (((DataRecord->Instance & EFI_D_POOL) != 0) || ((DataRecord->Instance & EFI_D_PAGE) != 0))
        ) {
      //
      // If memory error, do not call LogData ().
      //
      DebugPrint ((UINTN)-1, "Memory Error\n");
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      //
      // Log DataRecord in Data Hub
      //
      Status = mDataHub->LogData (
                          mDataHub,
                          &gEfiStatusCodeGuid,
                          &gEfiStatusCodeRuntimeProtocolGuid,
                          DataRecordClass,
                          DataRecord,
                          (UINT32) Size
                          );
    }

    ReleaseRecordBuffer (BufferEntry);
  }

  mEventHandlerActive = FALSE;

  return ;
}

VOID
BsDataHubStatusCodeInitialize (
  VOID
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
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiDataHubProtocolGuid, NULL, (VOID **) &mDataHub);
  //
  // Should never fail due to dependency grammer
  //
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize FIFO
  //
  InitializeListHead (&mRecordBuffer);
  mRecordNum = 0;

  EfiInitializeLock (&mStatusCodeReportLock, EFI_TPL_HIGH_LEVEL);

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

}
