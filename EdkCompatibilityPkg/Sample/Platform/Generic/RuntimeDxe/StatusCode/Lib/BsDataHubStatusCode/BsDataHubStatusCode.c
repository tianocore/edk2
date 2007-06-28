/*++

Copyright (c)  2004 - 2006, Intel Corporation                                                         
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
  The status codes are recorded in a extensiable buffer, and a event is signalled 
  to log them to the data hub. The recorder is the producer of the status code in
  buffer and the event notify function the consummer.

--*/

#include "BsDataHubStatusCode.h"

//
// Globals only work at BootService Time. NOT at Runtime!
//
static EFI_DATA_HUB_PROTOCOL  *mDataHub;
static EFI_LIST_ENTRY         *mRecordHead;
static EFI_LIST_ENTRY         *mRecordTail;
static INTN                   mRecordNum = 0;
static EFI_EVENT              mLogDataHubEvent;
static EFI_LOCK               mStatusCodeReportLock = EFI_INITIALIZE_LOCK_VARIABLE(EFI_TPL_HIGH_LEVEL);
static BOOLEAN                mEventHandlerActive   = FALSE;


STATUS_CODE_RECORD_LIST *
AllocateRecordBuffer (
  VOID
  )
/*++

Routine Description:

  Allocate a new record list node and initialize it.
  Inserting the node into the list isn't the task of this function.

Arguments:

  None

Returns:

  A pointer to the new allocated node or NULL if non available

--*/
{
  STATUS_CODE_RECORD_LIST *DataBuffer;

  DataBuffer = NULL;

  gBS->AllocatePool (EfiBootServicesData, sizeof (STATUS_CODE_RECORD_LIST), &DataBuffer);
  if (DataBuffer == NULL) {
    return NULL;
  }

  EfiCommonLibZeroMem (DataBuffer, sizeof (STATUS_CODE_RECORD_LIST));
  DataBuffer->Signature = BS_DATA_HUB_STATUS_CODE_SIGNATURE;

  return DataBuffer;
}

DATA_HUB_STATUS_CODE_DATA_RECORD *
AquireEmptyRecordBuffer (
  VOID
  )
/*++

Routine Description:

  Acquire an empty record buffer from the record list if there's free node,
  or allocate one new node and insert it to the list if the list is full and
  the function isn't run in EFI_TPL_HIGH_LEVEL.

Arguments:

  None

Returns:

  Pointer to new record buffer. NULL if none available.

--*/
{
  EFI_TPL                 OldTpl;
  STATUS_CODE_RECORD_LIST *DataBuffer;

  DataBuffer = NULL;

  //
  // This function must be reentrant because an event with higher priority may interrupt it
  // and also report status code.
  //
  EfiAcquireLock (&mStatusCodeReportLock);
  if (mRecordTail != mRecordHead->ForwardLink) {
    if (mRecordNum != 0) {
      mRecordHead = mRecordHead->ForwardLink;
    }
    DataBuffer = CR (mRecordHead, STATUS_CODE_RECORD_LIST, Link, BS_DATA_HUB_STATUS_CODE_SIGNATURE);
    mRecordNum++;
    EfiReleaseLock (&mStatusCodeReportLock);

    //
    // Initalize the record buffer is the responsibility of the producer,
    // because the consummer is in a lock so must keep it short.
    //
    EfiCommonLibZeroMem (&DataBuffer->RecordBuffer[0], BYTES_PER_BUFFER);
  } else if (mRecordNum < MAX_RECORD_NUM) {
    //
    // The condition of "mRecordNum < MAX_RECORD_NUM" is not promised,
    // because mRecodeNum may be increased out of this lock.
    //
    EfiReleaseLock (&mStatusCodeReportLock);

    //
    // Can't allocate additional buffer in EFI_TPL_HIGH_LEVEL.
    // Reporting too many status code in EFI_TPL_HIGH_LEVEL may cause status code lost.
    //
    OldTpl = gBS->RaiseTPL (EFI_TPL_HIGH_LEVEL);
    if (OldTpl == EFI_TPL_HIGH_LEVEL) {
      return NULL;
    }
    gBS->RestoreTPL (OldTpl);
    DataBuffer = AllocateRecordBuffer ();
    if (DataBuffer == NULL) {
      return NULL;
    }
    EfiAcquireLock (&mStatusCodeReportLock);
    InsertHeadList (mRecordHead, &DataBuffer->Link);
    mRecordHead = mRecordHead->ForwardLink;
    mRecordNum++;
    EfiReleaseLock (&mStatusCodeReportLock);
  } else {
    EfiReleaseLock (&mStatusCodeReportLock);
    return NULL;
  }

  return (DATA_HUB_STATUS_CODE_DATA_RECORD *) DataBuffer->RecordBuffer;
}

EFI_STATUS
ReleaseRecordBuffer (
  IN  STATUS_CODE_RECORD_LIST  *RecordBuffer
  )
/*++

Routine Description:

  Release a buffer in the list, remove some nodes to keep the list inital length.

Arguments:

  RecordBuffer          - Buffer to release

Returns:

  EFI_SUCCESS           - If DataRecord is valid
  EFI_UNSUPPORTED       - The record list has empty

--*/
{
  ASSERT (RecordBuffer != NULL);

  //
  // The consummer needn't to be reentrient and the producer won't do any meaningful thing
  // when consummer is logging records.
  //
  if (mRecordNum <= 0) {
    return EFI_UNSUPPORTED;
  } else if (mRecordNum > INITIAL_RECORD_NUM) {
    mRecordTail = mRecordTail->ForwardLink;
    RemoveEntryList (&RecordBuffer->Link);
    mRecordNum--;
    gBS->FreePool (RecordBuffer);
  } else {
    if (mRecordNum != 1) {
      mRecordTail = mRecordTail->ForwardLink;
    }
    mRecordNum--;
  }

  return EFI_SUCCESS;
}

EFI_BOOTSERVICE
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
  DATA_HUB_STATUS_CODE_DATA_RECORD  *DataHub;
  UINT32                            ErrorLevel;
  VA_LIST                           Marker;
  CHAR8                             *Format;
  UINTN                             Index;
  CHAR16                            FormatBuffer[BYTES_PER_RECORD];

  DataHub = NULL;

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
    EfiCopyMem (&DataHub->CallerId, CallerId, sizeof (EFI_GUID));
  } else {
    EfiZeroMem (&DataHub->CallerId, sizeof (EFI_GUID));
  }

  if (Data == NULL) {
    EfiZeroMem (&DataHub->Data, sizeof (EFI_STATUS_CODE_DATA));
  } else {
    //
    // Copy generic Header
    //
    EfiCopyMem (&DataHub->Data, Data, sizeof (EFI_STATUS_CODE_DATA));

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
                (UINT16 *) (DataHub + 1),
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

      EfiCopyMem (DataHub + 1, Data + 1, DataHub->Data.Size);
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
  EFI_LIST_ENTRY                    *Link;
  STATUS_CODE_RECORD_LIST           *BufferEntry;

  //
  // Set our global flag so we don't recurse if we get an error here.
  //
  mEventHandlerActive = TRUE;
  
  //
  // Log DataRecord in Data Hub.
  // If there are multiple DataRecords, Log all of them.
  //
  Link = mRecordTail;

  while (mRecordNum != 0) {
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
      ErrorPrint (L"ERROR", "Memory Error\n");
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      //
      // We don't log EFI_D_POOL and EFI_D_PAGE debug info to datahub
      // to avoid recursive logging due to the memory allocation in datahub
      //
      if (DataRecordClass != EFI_DATA_RECORD_CLASS_DEBUG ||
          ((DataRecord->Instance & EFI_D_POOL) == 0 && (DataRecord->Instance & EFI_D_PAGE) == 0)) {
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
    }

    ReleaseRecordBuffer (BufferEntry);
  }

  mEventHandlerActive = FALSE;

  return ;
}

EFI_BOOTSERVICE
EFI_STATUS
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
  STATUS_CODE_RECORD_LIST *DataBuffer;
  UINTN                   Index1;

  DataBuffer = NULL;

  Status = gBS->LocateProtocol (&gEfiDataHubProtocolGuid, NULL, &mDataHub);
  //
  // Should never fail due to dependency grammer
  //
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize a record list with length not greater than INITIAL_RECORD_NUM.
  // If no buffer can be allocated, return EFI_OUT_OF_RESOURCES.
  //
  DataBuffer = AllocateRecordBuffer ();
  if (DataBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  mRecordHead = &DataBuffer->Link;
  mRecordTail = mRecordHead;
  InitializeListHead (mRecordHead);
  
  for (Index1 = 1; Index1 < INITIAL_RECORD_NUM; Index1++) {
    DataBuffer = AllocateRecordBuffer ();
    if (DataBuffer == NULL) {
      break;
    }
    InsertHeadList (mRecordHead, &DataBuffer->Link);
  }


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

  return EFI_SUCCESS;
}
