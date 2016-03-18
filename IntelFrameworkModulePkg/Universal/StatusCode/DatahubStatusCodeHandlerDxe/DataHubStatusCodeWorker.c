/** @file
  Data Hub status code worker.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DatahubStatusCodeHandlerDxe.h"

//
// Initialize FIFO to cache records.
//
LIST_ENTRY                mRecordsFifo          = INITIALIZE_LIST_HEAD_VARIABLE (mRecordsFifo);
LIST_ENTRY                mRecordsBuffer        = INITIALIZE_LIST_HEAD_VARIABLE (mRecordsBuffer);
UINT32                    mLogDataHubStatus     = 0;
EFI_EVENT                 mLogDataHubEvent;
//
// Cache data hub protocol.
//
EFI_DATA_HUB_PROTOCOL     *mDataHubProtocol = NULL;


/**
  Retrieve one record of from free record buffer. This record is removed from
  free record buffer.

  This function retrieves one record from free record buffer.
  If the pool has been exhausted, then new memory would be allocated for it.

  @return  Pointer to the free record.
           NULL means failure to allocate new memeory for free record buffer.

**/
DATA_HUB_STATUS_CODE_DATA_RECORD *
AcquireRecordBuffer (
  VOID
  )
{
  DATAHUB_STATUSCODE_RECORD *Record;
  EFI_TPL                   CurrentTpl;
  LIST_ENTRY                *Node;
  UINT32                    Index;

  CurrentTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  if (!IsListEmpty (&mRecordsBuffer)) {
    //
    // Strip one entry from free record buffer.
    //
    Node = GetFirstNode (&mRecordsBuffer);
    RemoveEntryList (Node);

    Record = BASE_CR (Node, DATAHUB_STATUSCODE_RECORD, Node);
  } else {
    if (CurrentTpl > TPL_NOTIFY) {
      //
      // Memory management should work at <=TPL_NOTIFY
      // 
      gBS->RestoreTPL (CurrentTpl);
      return NULL;
    }

    //
    // If free record buffer is exhausted, then allocate 16 new records for it.
    //
    gBS->RestoreTPL (CurrentTpl);
    Record   = (DATAHUB_STATUSCODE_RECORD *) AllocateZeroPool (sizeof (DATAHUB_STATUSCODE_RECORD) * 16);
    if (Record == NULL) {
      return NULL;
    }

    CurrentTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    //
    // Here we only insert 15 new records to the free record buffer, for the first record
    // will be returned immediately.
    //
    for (Index = 1; Index < 16; Index++) {
      InsertTailList (&mRecordsBuffer, &Record[Index].Node);
    }
  }

  Record->Signature = DATAHUB_STATUS_CODE_SIGNATURE;
  InsertTailList (&mRecordsFifo, &Record->Node);

  gBS->RestoreTPL (CurrentTpl);

  return (DATA_HUB_STATUS_CODE_DATA_RECORD *) (Record->Data);
}


/**
  Retrieve one record from Records FIFO. The record would be removed from FIFO.

  @return   Point to record, which is ready to be logged.
            NULL means the FIFO of record is empty.

**/
DATA_HUB_STATUS_CODE_DATA_RECORD *
RetrieveRecord (
  VOID
  )
{
  DATA_HUB_STATUS_CODE_DATA_RECORD  *RecordData;
  DATAHUB_STATUSCODE_RECORD         *Record;
  LIST_ENTRY                        *Node;
  EFI_TPL                           CurrentTpl;

  RecordData = NULL;

  CurrentTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  if (!IsListEmpty (&mRecordsFifo)) {
    Node = GetFirstNode (&mRecordsFifo);
    Record = CR (Node, DATAHUB_STATUSCODE_RECORD, Node, DATAHUB_STATUS_CODE_SIGNATURE);
    ASSERT (Record != NULL);

    RemoveEntryList (&Record->Node);
    RecordData = (DATA_HUB_STATUS_CODE_DATA_RECORD *) Record->Data;
  }

  gBS->RestoreTPL (CurrentTpl);

  return RecordData;
}

/**
  Release given record and return it to free record buffer.
  
  @param RecordData  Pointer to the record to release.

**/
VOID
ReleaseRecord (
  DATA_HUB_STATUS_CODE_DATA_RECORD  *RecordData
  )
{
  DATAHUB_STATUSCODE_RECORD         *Record;
  EFI_TPL                           CurrentTpl;

  Record = CR (RecordData, DATAHUB_STATUSCODE_RECORD, Data[0], DATAHUB_STATUS_CODE_SIGNATURE);
  ASSERT (Record != NULL);

  CurrentTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  InsertTailList (&mRecordsBuffer, &Record->Node);
  Record->Signature = 0;

  gBS->RestoreTPL (CurrentTpl);
}

/**
  Report status code into DataHub.

  @param  CodeType             Indicates the type of status code being reported.
  @param  Value                Describes the current status of a hardware or software entity.
                               This included information about the class and subclass that is used to
                               classify the entity as well as an operation.
  @param  Instance             The enumeration of a hardware or software entity within
                               the system. Valid instance numbers start with 1.
  @param  CallerId             This optional parameter may be used to identify the caller.
                               This parameter allows the status code driver to apply different rules to
                               different callers.
  @param  Data                 This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS          The function completed successfully.
  @retval EFI_DEVICE_ERROR     Function is reentered.
  @retval EFI_DEVICE_ERROR     Function is called at runtime.
  @retval EFI_OUT_OF_RESOURCES Fail to allocate memory for free record buffer.

**/
EFI_STATUS
EFIAPI
DataHubStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )
{
  DATA_HUB_STATUS_CODE_DATA_RECORD  *Record;
  UINT32                            ErrorLevel;
  BASE_LIST                         Marker;
  CHAR8                             *Format;
  UINTN                             CharCount;

  //
  // Use atom operation to avoid the reentant of report.
  // If current status is not zero, then the function is reentrancy.
  //
  if (InterlockedCompareExchange32 (&mLogDataHubStatus, 0, 0) == 1) {
    return EFI_DEVICE_ERROR;
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
    CopyMem (&Record->CallerId, CallerId, sizeof (EFI_GUID));
  }

  if (Data != NULL) {
    if (ReportStatusCodeExtractDebugInfo (Data, &ErrorLevel, &Marker, &Format)) {
      CharCount = UnicodeBSPrintAsciiFormat (
                    (CHAR16 *) (Record + 1),
                    EFI_STATUS_CODE_DATA_MAX_SIZE,
                    Format,
                    Marker
                    );
      //
      // Change record data type to DebugType.
      //
      CopyGuid (&Record->Data.Type, &gEfiStatusCodeDataTypeDebugGuid);
      Record->Data.HeaderSize = Data->HeaderSize;
      Record->Data.Size = (UINT16) ((CharCount + 1) * sizeof (CHAR16));
    } else {
      //
      // Copy status code data header
      //
      CopyMem (&Record->Data, Data, sizeof (EFI_STATUS_CODE_DATA));

      if (Data->Size > EFI_STATUS_CODE_DATA_MAX_SIZE) {
        Record->Data.Size = EFI_STATUS_CODE_DATA_MAX_SIZE;
      }
      CopyMem ((VOID *) (Record + 1), Data + 1, Record->Data.Size);
    }
  }

  gBS->SignalEvent (mLogDataHubEvent);

  return EFI_SUCCESS;
}


/**
  The Event handler which will be notified to log data in Data Hub.

  @param  Event       Instance of the EFI_EVENT to signal whenever data is
                      available to be logged in the system.
  @param  Context     Context of the event.

**/
VOID
EFIAPI
LogDataHubEventCallBack (
  IN  EFI_EVENT     Event,
  IN  VOID          *Context
  )
{
  DATA_HUB_STATUS_CODE_DATA_RECORD  *Record;
  UINT32                            Size;
  UINT64                            DataRecordClass;

  //
  // Use atom operation to avoid the reentant of report.
  // If current status is not zero, then the function is reentrancy.
  //
  if (InterlockedCompareExchange32 (&mLogDataHubStatus, 0, 1) == 1) {
    return;
  }

  //
  // Log DataRecord in Data Hub.
  // Journal records fifo to find all record entry.
  //
  while (TRUE) {
    //
    // Retrieve record from record FIFO until no more record can be retrieved.
    //
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
                        &gEfiDataHubStatusCodeRecordGuid,
                        &gEfiStatusCodeRuntimeProtocolGuid,
                        DataRecordClass,
                        Record,
                        Size
                        );

    ReleaseRecord (Record);
  }

  //
  // Restore the nest status of report
  //
  InterlockedCompareExchange32 (&mLogDataHubStatus, 1, 0);
}


/**
  Locate Data Hub Protocol and create event for logging data
  as initialization for data hub status code worker.

  @retval EFI_SUCCESS  Initialization is successful.

**/
EFI_STATUS
DataHubStatusCodeInitializeWorker (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  &gEfiDataHubProtocolGuid, 
                  NULL, 
                  (VOID **) &mDataHubProtocol
                  );
  if (EFI_ERROR (Status)) {
    mDataHubProtocol = NULL;
    return Status;
  }

  //
  // Create a Notify Event to log data in Data Hub
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  LogDataHubEventCallBack,
                  NULL,
                  &mLogDataHubEvent
                  );

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}


