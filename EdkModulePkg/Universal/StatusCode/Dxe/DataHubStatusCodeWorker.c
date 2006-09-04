/** @file
  Data Hub status code worker in DXE.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                            
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  DataHubStatusCodeWorker.c

**/
#include "DxeStatusCode.h"

//
// Initialize FIFO to cache records.
//
STATIC 
EFI_LOCK                  mFifoLock        = EFI_INITIALIZE_LOCK_VARIABLE  (EFI_TPL_HIGH_LEVEL);
STATIC
LIST_ENTRY                mRecordsFifo     = INITIALIZE_LIST_HEAD_VARIABLE (mRecordsFifo);
STATIC
UINTN                     mNumberOfRecords = 0;
STATIC
EFI_EVENT                 mLogDataHubEvent;
//
// Cache data hub protocol.
//
STATIC
EFI_DATA_HUB_PROTOCOL     *mDataHubProtocol;


/**
  Return buffer of length DATAHUB_STATUSCODE_RECORD
 
  @retval  NULL   Can not allocate free memeory for record.
  @retval  !NULL  Point to buffer of record.

**/
DATAHUB_STATUSCODE_RECORD *
AcquireRecordBuffer (
  VOID
  )
{
  DATAHUB_STATUSCODE_RECORD *Record;

  Record   = (DATAHUB_STATUSCODE_RECORD *) AllocateZeroPool (sizeof (DATAHUB_STATUSCODE_RECORD));
  if (NULL == Record) {
    return NULL;
  }
  Record->Signature = DATAHUB_STATUS_CODE_SIGNATURE;

  EfiAcquireLock (&mFifoLock);
  InsertTailList (&mRecordsFifo, &Record->Node);
  mNumberOfRecords++;
  EfiReleaseLock (&mFifoLock);

  return Record;
}


/**
  Release a mRecordBuffer entry allocated by AcquirRecordBuffer ().

  @param   Record        Point to record buffer which is acquired by AcquirRecordBuffer()
 
**/
VOID
FreeRecordBuffer (
  IN  DATAHUB_STATUSCODE_RECORD  *Record
  )
{
  ASSERT (Record != NULL);
  ASSERT (mNumberOfRecords != 0);

  EfiAcquireLock (&mFifoLock);
  RemoveEntryList (&Record->Node);
  mNumberOfRecords--;
  EfiReleaseLock (&mFifoLock);

  FreePool (Record);
}


/**
  Report status code into DataHub.
 
  @param  CodeType      Indicates the type of status code being reported.  Type EFI_STATUS_CODE_TYPE is defined in "Related Definitions¡± below.
 
  @param  Value         Describes the current status of a hardware or software entity.  
                        This included information about the class and subclass that is used to classify the entity 
                        as well as an operation.  For progress codes, the operation is the current activity. 
                        For error codes, it is the exception.  For debug codes, it is not defined at this time. 
                        Type EFI_STATUS_CODE_VALUE is defined in ¡°Related Definitions¡± below.  
                        Specific values are discussed in the Intel? Platform Innovation Framework for EFI Status Code Specification.
 
  @param  Instance      The enumeration of a hardware or software entity within the system.  
                        A system may contain multiple entities that match a class/subclass pairing. 
                        The instance differentiates between them.  An instance of 0 indicates that instance information is unavailable, 
                        not meaningful, or not relevant.  Valid instance numbers start with 1.


  @param  CallerId      This optional parameter may be used to identify the caller. 
                        This parameter allows the status code driver to apply different rules to different callers. 
                        Type EFI_GUID is defined in InstallProtocolInterface() in the EFI 1.10 Specification.


  @param  Data          This optional parameter may be used to pass additional data
 
  @retval EFI_OUT_OF_RESOURCES   Can not acquire record buffer.
  @retval EFI_DEVICE_ERROR       EFI serial device can not work after ExitBootService() is called .
  @retval EFI_SUCCESS            Success to cache status code and signal log data event.

**/
EFI_STATUS
DataHubStatusCodeReportWorker (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )
{
  DATAHUB_STATUSCODE_RECORD  *Record;
  UINT32                     ErrorLevel;
  VA_LIST                    Marker;
  CHAR8                      *Format;
  UINTN                      CharCount;

  //
  // See whether in runtime phase or not.
  //
  if (EfiAtRuntime ()) {
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
      CharCount = UnicodeVSPrintAsciiFormat (
                    (CHAR16 *) Record->ExtendData,
                    EFI_STATUS_CODE_DATA_MAX_SIZE,
                    Format,
                    Marker
                    );
      //
      // Change record data type from DebugType to String Type.
      //
      CopyGuid (&Record->Data.Type, &gEfiStatusCodeDataTypeStringGuid);
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
      CopyMem (Record->ExtendData, Data + 1, Record->Data.Size);
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
  DATAHUB_STATUSCODE_RECORD         *Record;
  UINT32                            Size;
  UINT64                            DataRecordClass;
  LIST_ENTRY                        *Node;

  //
  // Log DataRecord in Data Hub.
  // Journal records fifo to find all record entry.
  //
  //
  for (Node = mRecordsFifo.ForwardLink; Node != &mRecordsFifo;) {
    Record = CR (Node, DATAHUB_STATUSCODE_RECORD, Node, DATAHUB_STATUS_CODE_SIGNATURE);
    Node   = Node->ForwardLink;

    //
    // Add in the size of the header we added.
    //
    Size = sizeof (DATAHUB_STATUSCODE_RECORD) + (UINT32) Record->Data.Size;

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

    FreeRecordBuffer (Record);
  }
}


/**
  Initialize data hubstatus code.
  Create a data hub listener.
 
  @return  The function always return EFI_SUCCESS

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
  ASSERT_EFI_ERROR (Status);

  //
  // Create a Notify Event to log data in Data Hub
  //
  Status = gBS->CreateEvent (
                  EFI_EVENT_NOTIFY_SIGNAL,
                  EFI_TPL_CALLBACK,
                  LogDataHubEventCallBack,
                  NULL,
                  &mLogDataHubEvent
                  );

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}


