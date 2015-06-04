/** @file
  Performance library instance used in DXE phase to dump both PEI/DXE and SMM performance data.

  This library instance allows a DXE driver or UEFI application to dump both PEI/DXE and SMM performance data.
  StartPerformanceMeasurement(), EndPerformanceMeasurement(), StartPerformanceMeasurementEx()
  and EndPerformanceMeasurementEx() are not implemented.

  Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiDxe.h>

#include <Guid/Performance.h>

#include <Library/PerformanceLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/SmmCommunication.h>

#define SMM_PERFORMANCE_COMMUNICATION_BUFFER_SIZE (OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)  + sizeof (SMM_PERF_COMMUNICATE))

EFI_SMM_COMMUNICATION_PROTOCOL  *mSmmCommunication = NULL;
UINT8                           mSmmPerformanceBuffer[SMM_PERFORMANCE_COMMUNICATION_BUFFER_SIZE];
GAUGE_DATA_ENTRY                *mGaugeData = NULL;
UINTN                           mGaugeNumberOfEntries = 0;
GAUGE_DATA_ENTRY_EX             *mGaugeDataEx = NULL;
UINTN                           mGaugeNumberOfEntriesEx = 0;

BOOLEAN                         mNoSmmPerfHandler = FALSE;
BOOLEAN                         mNoSmmPerfExHandler = FALSE;

//
// The cached Performance Protocol and PerformanceEx Protocol interface.
//
PERFORMANCE_PROTOCOL            *mPerformance = NULL;
PERFORMANCE_EX_PROTOCOL         *mPerformanceEx = NULL;

/**
  The function caches the pointer to SMM Communication protocol.

  The function locates SMM Communication protocol from protocol database.

  @retval EFI_SUCCESS     SMM Communication protocol is successfully located.
  @retval Other           SMM Communication protocol is not located to log performance.

**/
EFI_STATUS
GetCommunicationProtocol (
  VOID
  )
{
  EFI_STATUS                      Status;
  EFI_SMM_COMMUNICATION_PROTOCOL  *Communication;

  if (mSmmCommunication != NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **) &Communication);
  if (!EFI_ERROR (Status)) {
    ASSERT (Communication != NULL);
    //
    // Cache SMM Communication protocol.
    //
    mSmmCommunication = Communication;
  }

  return Status;
}

/**
  The function caches the pointers to PerformanceEx protocol and Performance Protocol.

  The function locates PerformanceEx protocol and Performance Protocol from protocol database.

  @retval EFI_SUCCESS     PerformanceEx protocol or Performance Protocol is successfully located.
  @retval EFI_NOT_FOUND   Both PerformanceEx protocol and Performance Protocol are not located to log performance.

**/
EFI_STATUS
GetPerformanceProtocol (
  VOID
  )
{
  EFI_STATUS                Status;
  PERFORMANCE_PROTOCOL      *Performance;
  PERFORMANCE_EX_PROTOCOL   *PerformanceEx;

  if (mPerformanceEx != NULL || mPerformance != NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (&gPerformanceExProtocolGuid, NULL, (VOID **) &PerformanceEx);
  if (!EFI_ERROR (Status)) {
    ASSERT (PerformanceEx != NULL);
    //
    // Cache PerformanceEx Protocol.
    //
    mPerformanceEx = PerformanceEx;
    return EFI_SUCCESS;
  }

  Status = gBS->LocateProtocol (&gPerformanceProtocolGuid, NULL, (VOID **) &Performance);
  if (!EFI_ERROR (Status)) {
    ASSERT (Performance != NULL);
    //
    // Cache performance protocol.
    //
    mPerformance = Performance;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Creates a record for the beginning of a performance measurement.

  Creates a record that contains the Handle, Token, Module and Identifier.
  If TimeStamp is not zero, then TimeStamp is added to the record as the start time.
  If TimeStamp is zero, then this function reads the current time stamp
  and adds that time stamp value to the record as the start time.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.
  @param  Identifier              32-bit identifier. If the value is 0, the created record
                                  is same as the one created by StartPerformanceMeasurement.

  @retval RETURN_SUCCESS          The start of the measurement was recorded.
  @retval RETURN_OUT_OF_RESOURCES There are not enough resources to record the measurement.

**/
RETURN_STATUS
EFIAPI
StartPerformanceMeasurementEx (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp,
  IN UINT32       Identifier
  )
{
  return RETURN_SUCCESS;
}

/**
  Fills in the end time of a performance measurement.

  Looks up the record that matches Handle, Token, Module and Identifier.
  If the record can not be found then return RETURN_NOT_FOUND.
  If the record is found and TimeStamp is not zero,
  then TimeStamp is added to the record as the end time.
  If the record is found and TimeStamp is zero, then this function reads
  the current time stamp and adds that time stamp value to the record as the end time.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.
  @param  Identifier              32-bit identifier. If the value is 0, the found record
                                  is same as the one found by EndPerformanceMeasurement.

  @retval RETURN_SUCCESS          The end of  the measurement was recorded.
  @retval RETURN_NOT_FOUND        The specified measurement record could not be found.

**/
RETURN_STATUS
EFIAPI
EndPerformanceMeasurementEx (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp,
  IN UINT32       Identifier
  )
{
  return RETURN_SUCCESS;
}

/**
  Creates a record for the beginning of a performance measurement.

  Creates a record that contains the Handle, Token, and Module.
  If TimeStamp is not zero, then TimeStamp is added to the record as the start time.
  If TimeStamp is zero, then this function reads the current time stamp
  and adds that time stamp value to the record as the start time.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.

  @retval RETURN_SUCCESS          The start of the measurement was recorded.
  @retval RETURN_OUT_OF_RESOURCES There are not enough resources to record the measurement.

**/
RETURN_STATUS
EFIAPI
StartPerformanceMeasurement (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp
  )
{
  return RETURN_SUCCESS;
}

/**
  Fills in the end time of a performance measurement.

  Looks up the record that matches Handle, Token, and Module.
  If the record can not be found then return RETURN_NOT_FOUND.
  If the record is found and TimeStamp is not zero,
  then TimeStamp is added to the record as the end time.
  If the record is found and TimeStamp is zero, then this function reads
  the current time stamp and adds that time stamp value to the record as the end time.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.

  @retval RETURN_SUCCESS          The end of  the measurement was recorded.
  @retval RETURN_NOT_FOUND        The specified measurement record could not be found.

**/
RETURN_STATUS
EFIAPI
EndPerformanceMeasurement (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp
  )
{
  return RETURN_SUCCESS;
}

/**
  Attempts to retrieve a performance measurement log entry from the performance measurement log.
  It can also retrieve the log created by StartPerformanceMeasurement and EndPerformanceMeasurement,
  and then assign the Identifier with 0.

  Attempts to retrieve the performance log entry specified by LogEntryKey.  If LogEntryKey is
  zero on entry, then an attempt is made to retrieve the first entry from the performance log,
  and the key for the second entry in the log is returned.  If the performance log is empty,
  then no entry is retrieved and zero is returned.  If LogEntryKey is not zero, then the performance
  log entry associated with LogEntryKey is retrieved, and the key for the next entry in the log is
  returned.  If LogEntryKey is the key for the last entry in the log, then the last log entry is
  retrieved and an implementation specific non-zero key value that specifies the end of the performance
  log is returned.  If LogEntryKey is equal this implementation specific non-zero key value, then no entry
  is retrieved and zero is returned.  In the cases where a performance log entry can be returned,
  the log entry is returned in Handle, Token, Module, StartTimeStamp, EndTimeStamp and Identifier.
  If LogEntryKey is not a valid log entry key for the performance measurement log, then ASSERT().
  If Handle is NULL, then ASSERT().
  If Token is NULL, then ASSERT().
  If Module is NULL, then ASSERT().
  If StartTimeStamp is NULL, then ASSERT().
  If EndTimeStamp is NULL, then ASSERT().
  If Identifier is NULL, then ASSERT().

  @param  LogEntryKey             On entry, the key of the performance measurement log entry to retrieve.
                                  0, then the first performance measurement log entry is retrieved.
                                  On exit, the key of the next performance log entry.
  @param  Handle                  Pointer to environment specific context used to identify the component
                                  being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string that identifies the component
                                  being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string that identifies the module
                                  being measured.
  @param  StartTimeStamp          Pointer to the 64-bit time stamp that was recorded when the measurement
                                  was started.
  @param  EndTimeStamp            Pointer to the 64-bit time stamp that was recorded when the measurement
                                  was ended.
  @param  Identifier              Pointer to the 32-bit identifier that was recorded.

  @return The key for the next performance log entry (in general case).

**/
UINTN
EFIAPI
GetByPerformanceProtocol (
  IN  UINTN       LogEntryKey, 
  OUT CONST VOID  **Handle,
  OUT CONST CHAR8 **Token,
  OUT CONST CHAR8 **Module,
  OUT UINT64      *StartTimeStamp,
  OUT UINT64      *EndTimeStamp,
  OUT UINT32      *Identifier
  )
{
  EFI_STATUS            Status;
  GAUGE_DATA_ENTRY_EX   *GaugeData;

  Status = GetPerformanceProtocol ();
  if (EFI_ERROR (Status)) {
    return 0;
  }

  if (mPerformanceEx != NULL) {
    Status = mPerformanceEx->GetGaugeEx (LogEntryKey++, &GaugeData);
  } else if (mPerformance != NULL) {
    Status = mPerformance->GetGauge (LogEntryKey++, (GAUGE_DATA_ENTRY **) &GaugeData);
  } else {
    ASSERT (FALSE);
    return 0;
  }

  //
  // Make sure that LogEntryKey is a valid log entry key,
  //
  ASSERT (Status != EFI_INVALID_PARAMETER);

  if (EFI_ERROR (Status)) {
    //
    // The LogEntryKey is the last entry (equals to the total entry number).
    //
    return 0;
  }

  ASSERT (GaugeData != NULL);

  *Handle         = (VOID *) (UINTN) GaugeData->Handle;
  *Token          = GaugeData->Token;
  *Module         = GaugeData->Module;
  *StartTimeStamp = GaugeData->StartTimeStamp;
  *EndTimeStamp   = GaugeData->EndTimeStamp;
  if (mPerformanceEx != NULL) {
    *Identifier   = GaugeData->Identifier;
  } else {
    *Identifier   = 0;
  }

  return LogEntryKey;
}


/**
  Retrieves all previous logged performance measurement.
  Function will use SMM communicate protocol to get all previous SMM performance measurement data.
  If success, data buffer will be returned. If fail function will return NULL.

  @param  LogEntryKey             On entry, the key of the performance measurement log entry to retrieve.
                                  0, then the first performance measurement log entry is retrieved.
                                  On exit, the key of the next performance log entry.

  @retval !NULL           Get all gauge data success.
  @retval NULL            Get all gauge data failed.
**/
GAUGE_DATA_ENTRY *
EFIAPI
GetAllSmmGaugeData (
  IN UINTN      LogEntryKey
  )
{
  EFI_STATUS                  Status;
  EFI_SMM_COMMUNICATE_HEADER  *SmmCommBufferHeader;
  SMM_PERF_COMMUNICATE        *SmmPerfCommData;
  UINTN                       CommSize;
  UINTN                       DataSize;

  if (mNoSmmPerfHandler) {
    //
    // Not try to get the SMM gauge data again
    // if no SMM Performance handler found.
    //
    return NULL;
  }

  if (LogEntryKey != 0) {
    if (mGaugeData != NULL) {
      return mGaugeData;
    }
  } else {
    //
    // Reget the SMM gauge data at the first entry get.
    //
    if (mGaugeData != NULL) {
      FreePool (mGaugeData);
      mGaugeData = NULL;
      mGaugeNumberOfEntries = 0;
    }
  }

  Status = GetCommunicationProtocol ();
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Initialize communicate buffer 
  //
  SmmCommBufferHeader = (EFI_SMM_COMMUNICATE_HEADER *)mSmmPerformanceBuffer;
  SmmPerfCommData = (SMM_PERF_COMMUNICATE *)SmmCommBufferHeader->Data;
  ZeroMem((UINT8*)SmmPerfCommData, sizeof(SMM_PERF_COMMUNICATE));
    
  CopyGuid (&SmmCommBufferHeader->HeaderGuid, &gSmmPerformanceProtocolGuid);
  SmmCommBufferHeader->MessageLength = sizeof(SMM_PERF_COMMUNICATE);
  CommSize = SMM_PERFORMANCE_COMMUNICATION_BUFFER_SIZE;

  //
  // Get total number of SMM gauge entries
  //
  SmmPerfCommData->Function = SMM_PERF_FUNCTION_GET_GAUGE_ENTRY_NUMBER;
  Status = mSmmCommunication->Communicate (mSmmCommunication, mSmmPerformanceBuffer, &CommSize);
  if (Status == EFI_NOT_FOUND) {
    mNoSmmPerfHandler = TRUE;
  }
  if (EFI_ERROR (Status) || EFI_ERROR (SmmPerfCommData->ReturnStatus) || SmmPerfCommData->NumberOfEntries == 0) {
    return NULL;
  }

  mGaugeNumberOfEntries = SmmPerfCommData->NumberOfEntries;
  
  DataSize = mGaugeNumberOfEntries * sizeof(GAUGE_DATA_ENTRY);
  mGaugeData = AllocateZeroPool(DataSize);
  ASSERT (mGaugeData != NULL);
  
  //
  // Get all SMM gauge data
  //  
  SmmPerfCommData->Function = SMM_PERF_FUNCTION_GET_GAUGE_DATA;
  SmmPerfCommData->LogEntryKey = 0;
  SmmPerfCommData->NumberOfEntries = mGaugeNumberOfEntries;
  SmmPerfCommData->GaugeData = mGaugeData;
  Status = mSmmCommunication->Communicate (mSmmCommunication, mSmmPerformanceBuffer, &CommSize);
  if (EFI_ERROR (Status) || EFI_ERROR (SmmPerfCommData->ReturnStatus)) {
    FreePool (mGaugeData);
    mGaugeData = NULL;
    mGaugeNumberOfEntries = 0;
  }

  return mGaugeData;
}

/**
  Retrieves all previous logged performance measurement.
  Function will use SMM communicate protocol to get all previous SMM performance measurement data.
  If success, data buffer will be returned. If fail function will return NULL.

  @param  LogEntryKey             On entry, the key of the performance measurement log entry to retrieve.
                                  0, then the first performance measurement log entry is retrieved.
                                  On exit, the key of the next performance log entry.

  @retval !NULL           Get all gauge data success.
  @retval NULL            Get all gauge data failed.
**/
GAUGE_DATA_ENTRY_EX *
EFIAPI
GetAllSmmGaugeDataEx (
  IN UINTN      LogEntryKey
  )
{
  EFI_STATUS                  Status;
  EFI_SMM_COMMUNICATE_HEADER  *SmmCommBufferHeader;
  SMM_PERF_COMMUNICATE_EX     *SmmPerfCommData;
  UINTN                       CommSize;
  UINTN                       DataSize;

  if (mNoSmmPerfExHandler) {
    //
    // Not try to get the SMM gauge data again
    // if no SMM PerformanceEx handler found.
    //
    return NULL;
  }

  if (LogEntryKey != 0) {
    if (mGaugeDataEx != NULL) {
      return mGaugeDataEx;
    }
  } else {
    //
    // Reget the SMM gauge data at the first entry get.
    //
    if (mGaugeDataEx != NULL) {
      FreePool (mGaugeDataEx);
      mGaugeDataEx = NULL;
      mGaugeNumberOfEntriesEx = 0;
    }
  }

  Status = GetCommunicationProtocol ();
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Initialize communicate buffer 
  //
  SmmCommBufferHeader = (EFI_SMM_COMMUNICATE_HEADER *)mSmmPerformanceBuffer;
  SmmPerfCommData = (SMM_PERF_COMMUNICATE_EX *)SmmCommBufferHeader->Data;
  ZeroMem((UINT8*)SmmPerfCommData, sizeof(SMM_PERF_COMMUNICATE_EX));
    
  CopyGuid (&SmmCommBufferHeader->HeaderGuid, &gSmmPerformanceExProtocolGuid);
  SmmCommBufferHeader->MessageLength = sizeof(SMM_PERF_COMMUNICATE_EX);
  CommSize = SMM_PERFORMANCE_COMMUNICATION_BUFFER_SIZE;

  //
  // Get total number of SMM gauge entries
  //
  SmmPerfCommData->Function = SMM_PERF_FUNCTION_GET_GAUGE_ENTRY_NUMBER;
  Status = mSmmCommunication->Communicate (mSmmCommunication, mSmmPerformanceBuffer, &CommSize);
  if (Status == EFI_NOT_FOUND) {
    mNoSmmPerfExHandler = TRUE;
  }
  if (EFI_ERROR (Status) || EFI_ERROR (SmmPerfCommData->ReturnStatus) || SmmPerfCommData->NumberOfEntries == 0) {
    return NULL;
  }

  mGaugeNumberOfEntriesEx = SmmPerfCommData->NumberOfEntries;
  
  DataSize = mGaugeNumberOfEntriesEx * sizeof(GAUGE_DATA_ENTRY_EX);
  mGaugeDataEx = AllocateZeroPool(DataSize);
  ASSERT (mGaugeDataEx != NULL);
  
  //
  // Get all SMM gauge data
  //  
  SmmPerfCommData->Function = SMM_PERF_FUNCTION_GET_GAUGE_DATA;
  SmmPerfCommData->LogEntryKey = 0;
  SmmPerfCommData->NumberOfEntries = mGaugeNumberOfEntriesEx;
  SmmPerfCommData->GaugeDataEx = mGaugeDataEx;
  Status = mSmmCommunication->Communicate (mSmmCommunication, mSmmPerformanceBuffer, &CommSize);
  if (EFI_ERROR (Status) || EFI_ERROR (SmmPerfCommData->ReturnStatus)) {
    FreePool (mGaugeDataEx);
    mGaugeDataEx = NULL;
    mGaugeNumberOfEntriesEx = 0;
  }
 
  return mGaugeDataEx;
}

/**
  Attempts to retrieve a performance measurement log entry from the performance measurement log.
  It can also retrieve the log created by StartPerformanceMeasurement and EndPerformanceMeasurement,
  and then assign the Identifier with 0.

  Attempts to retrieve the performance log entry specified by LogEntryKey.  If LogEntryKey is
  zero on entry, then an attempt is made to retrieve the first entry from the performance log,
  and the key for the second entry in the log is returned.  If the performance log is empty,
  then no entry is retrieved and zero is returned.  If LogEntryKey is not zero, then the performance
  log entry associated with LogEntryKey is retrieved, and the key for the next entry in the log is
  returned.  If LogEntryKey is the key for the last entry in the log, then the last log entry is
  retrieved and an implementation specific non-zero key value that specifies the end of the performance
  log is returned.  If LogEntryKey is equal this implementation specific non-zero key value, then no entry
  is retrieved and zero is returned.  In the cases where a performance log entry can be returned,
  the log entry is returned in Handle, Token, Module, StartTimeStamp, EndTimeStamp and Identifier.
  If LogEntryKey is not a valid log entry key for the performance measurement log, then ASSERT().
  If Handle is NULL, then ASSERT().
  If Token is NULL, then ASSERT().
  If Module is NULL, then ASSERT().
  If StartTimeStamp is NULL, then ASSERT().
  If EndTimeStamp is NULL, then ASSERT().
  If Identifier is NULL, then ASSERT().

  @param  LogEntryKey             On entry, the key of the performance measurement log entry to retrieve.
                                  0, then the first performance measurement log entry is retrieved.
                                  On exit, the key of the next performance log entry.
  @param  Handle                  Pointer to environment specific context used to identify the component
                                  being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string that identifies the component
                                  being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string that identifies the module
                                  being measured.
  @param  StartTimeStamp          Pointer to the 64-bit time stamp that was recorded when the measurement
                                  was started.
  @param  EndTimeStamp            Pointer to the 64-bit time stamp that was recorded when the measurement
                                  was ended.
  @param  Identifier              Pointer to the 32-bit identifier that was recorded.

  @return The key for the next performance log entry (in general case).

**/
UINTN
EFIAPI
GetPerformanceMeasurementEx (
  IN  UINTN       LogEntryKey, 
  OUT CONST VOID  **Handle,
  OUT CONST CHAR8 **Token,
  OUT CONST CHAR8 **Module,
  OUT UINT64      *StartTimeStamp,
  OUT UINT64      *EndTimeStamp,
  OUT UINT32      *Identifier
  )
{
  GAUGE_DATA_ENTRY_EX   *GaugeData;

  GaugeData = NULL;

  ASSERT (Handle != NULL);
  ASSERT (Token != NULL);
  ASSERT (Module != NULL);
  ASSERT (StartTimeStamp != NULL);
  ASSERT (EndTimeStamp != NULL);
  ASSERT (Identifier != NULL);

  mGaugeDataEx = GetAllSmmGaugeDataEx (LogEntryKey);
  if (mGaugeDataEx != NULL) {
    if (LogEntryKey >= mGaugeNumberOfEntriesEx) {
      //
      // Try to get the data by Performance Protocol.
      //
      LogEntryKey = LogEntryKey - mGaugeNumberOfEntriesEx;
      LogEntryKey = GetByPerformanceProtocol (
                      LogEntryKey,
                      Handle,
                      Token,
                      Module,
                      StartTimeStamp,
                      EndTimeStamp,
                      Identifier
                      );
      if (LogEntryKey == 0) {
        //
        // Last entry.
        //
        return LogEntryKey;
      } else {
        return (LogEntryKey + mGaugeNumberOfEntriesEx);
      }
    }

    GaugeData = &mGaugeDataEx[LogEntryKey++];
    *Identifier = GaugeData->Identifier;
  } else {
    mGaugeData = GetAllSmmGaugeData (LogEntryKey);
    if (mGaugeData != NULL) {
      if (LogEntryKey >= mGaugeNumberOfEntries) {
        //
        // Try to get the data by Performance Protocol.
        //
        LogEntryKey = LogEntryKey - mGaugeNumberOfEntries;
        LogEntryKey = GetByPerformanceProtocol (
                        LogEntryKey,
                        Handle,
                        Token,
                        Module,
                        StartTimeStamp,
                        EndTimeStamp,
                        Identifier
                        );
        if (LogEntryKey == 0) {
          //
          // Last entry.
          //
          return LogEntryKey;
        } else {
          return (LogEntryKey + mGaugeNumberOfEntries);
        }
      }

      GaugeData = (GAUGE_DATA_ENTRY_EX *) &mGaugeData[LogEntryKey++];
      *Identifier = 0;
    } else {
      return GetByPerformanceProtocol (
               LogEntryKey,
               Handle,
               Token,
               Module,
               StartTimeStamp,
               EndTimeStamp,
               Identifier
               );
    }
  }

  *Handle         = (VOID *) (UINTN) GaugeData->Handle;
  *Token          = GaugeData->Token;
  *Module         = GaugeData->Module;
  *StartTimeStamp = GaugeData->StartTimeStamp;
  *EndTimeStamp   = GaugeData->EndTimeStamp;

  return LogEntryKey;
}

/**
  Attempts to retrieve a performance measurement log entry from the performance measurement log.
  It can also retrieve the log created by StartPerformanceMeasurementEx and EndPerformanceMeasurementEx,
  and then eliminate the Identifier.

  Attempts to retrieve the performance log entry specified by LogEntryKey.  If LogEntryKey is
  zero on entry, then an attempt is made to retrieve the first entry from the performance log,
  and the key for the second entry in the log is returned.  If the performance log is empty,
  then no entry is retrieved and zero is returned.  If LogEntryKey is not zero, then the performance
  log entry associated with LogEntryKey is retrieved, and the key for the next entry in the log is
  returned.  If LogEntryKey is the key for the last entry in the log, then the last log entry is
  retrieved and an implementation specific non-zero key value that specifies the end of the performance
  log is returned.  If LogEntryKey is equal this implementation specific non-zero key value, then no entry
  is retrieved and zero is returned.  In the cases where a performance log entry can be returned,
  the log entry is returned in Handle, Token, Module, StartTimeStamp, and EndTimeStamp.
  If LogEntryKey is not a valid log entry key for the performance measurement log, then ASSERT().
  If Handle is NULL, then ASSERT().
  If Token is NULL, then ASSERT().
  If Module is NULL, then ASSERT().
  If StartTimeStamp is NULL, then ASSERT().
  If EndTimeStamp is NULL, then ASSERT().

  @param  LogEntryKey             On entry, the key of the performance measurement log entry to retrieve.
                                  0, then the first performance measurement log entry is retrieved.
                                  On exit, the key of the next performance log entry.
  @param  Handle                  Pointer to environment specific context used to identify the component
                                  being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string that identifies the component
                                  being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string that identifies the module
                                  being measured.
  @param  StartTimeStamp          Pointer to the 64-bit time stamp that was recorded when the measurement
                                  was started.
  @param  EndTimeStamp            Pointer to the 64-bit time stamp that was recorded when the measurement
                                  was ended.

  @return The key for the next performance log entry (in general case).

**/
UINTN
EFIAPI
GetPerformanceMeasurement (
  IN  UINTN       LogEntryKey,
  OUT CONST VOID  **Handle,
  OUT CONST CHAR8 **Token,
  OUT CONST CHAR8 **Module,
  OUT UINT64      *StartTimeStamp,
  OUT UINT64      *EndTimeStamp
  )
{
  UINT32 Identifier;
  return GetPerformanceMeasurementEx (LogEntryKey, Handle, Token, Module, StartTimeStamp, EndTimeStamp, &Identifier);
}

/**
  Returns TRUE if the performance measurement macros are enabled.

  This function returns TRUE if the PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of
  PcdPerformanceLibraryPropertyMask is set.  Otherwise FALSE is returned.

  @retval TRUE                    The PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of
                                  PcdPerformanceLibraryPropertyMask is set.
  @retval FALSE                   The PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of
                                  PcdPerformanceLibraryPropertyMask is clear.

**/
BOOLEAN
EFIAPI
PerformanceMeasurementEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdPerformanceLibraryPropertyMask) & PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED) != 0);
}
