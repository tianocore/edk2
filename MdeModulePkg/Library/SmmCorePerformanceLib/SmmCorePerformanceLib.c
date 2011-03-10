/** @file
  Performance library instance used by SMM Core.

  This library provides the performance measurement interfaces and initializes performance
  logging for the SMM phase.
  It initializes SMM phase performance logging by publishing the Performance Protocol,
  which is consumed by SmmPerformanceLib to logging performance data in SMM phase.

  This library is mainly used by SMM Core to start performance logging to ensure that
  SMM Performance Protocol is installed at the very beginning of SMM phase.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "SmmCorePerformanceLibInternal.h"

//
// The data structure to hold global performance data.
//
GAUGE_DATA_HEADER       *mGaugeData;

//
// The current maximum number of logging entries. If current number of 
// entries exceeds this value, it will re-allocate a larger array and
// migration the old data to the larger array.
//
UINT32                  mMaxGaugeRecords;

//
// The handle to install Performance Protocol instance.
//
EFI_HANDLE              mHandle = NULL;

BOOLEAN                 mPerformanceMeasurementEnabled;

SPIN_LOCK               mSmmPerfLock;

EFI_SMRAM_DESCRIPTOR    *mSmramRanges;
UINTN                   mSmramRangeCount;

//
// Interfaces for performance protocol.
//
PERFORMANCE_PROTOCOL mPerformanceInterface = {
  StartGauge,
  EndGauge,
  GetGauge
};

/**
  Searches in the gauge array with keyword Handle, Token and Module.

  This internal function searches for the gauge entry in the gauge array.
  If there is an entry that exactly matches the given key word triple
  and its end time stamp is zero, then the index of that gauge entry is returned;
  otherwise, the the number of gauge entries in the array is returned.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.

  @retval The index of gauge entry in the array.

**/
UINT32
SmmSearchForGaugeEntry (
  IN CONST VOID                 *Handle,  OPTIONAL
  IN CONST CHAR8                *Token,   OPTIONAL
  IN CONST CHAR8                *Module   OPTIONAL
  )
{
  UINT32                    Index;
  UINT32                    NumberOfEntries;
  GAUGE_DATA_ENTRY          *GaugeEntryArray;

  if (Token == NULL) {
    Token = "";
  }
  if (Module == NULL) {
    Module = "";
  }

  NumberOfEntries = mGaugeData->NumberOfEntries;
  GaugeEntryArray = (GAUGE_DATA_ENTRY *) (mGaugeData + 1);

  for (Index = 0; Index < NumberOfEntries; Index++) {
    if ((GaugeEntryArray[Index].Handle == (EFI_PHYSICAL_ADDRESS) (UINTN) Handle) &&
         AsciiStrnCmp (GaugeEntryArray[Index].Token, Token, SMM_PERFORMANCE_STRING_LENGTH) == 0 &&
         AsciiStrnCmp (GaugeEntryArray[Index].Module, Module, SMM_PERFORMANCE_STRING_LENGTH) == 0 &&
         GaugeEntryArray[Index].EndTimeStamp == 0) {
      break;
    }
  }

  return Index;
}

/**
  Adds a record at the end of the performance measurement log
  that records the start time of a performance measurement.

  Adds a record to the end of the performance measurement log
  that contains the Handle, Token, and Module.
  The end time of the new record must be set to zero.
  If TimeStamp is not zero, then TimeStamp is used to fill in the start time in the record.
  If TimeStamp is zero, the start time in the record is filled in with the value
  read from the current time stamp.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_OUT_OF_RESOURCES    There are not enough resources to record the measurement.

**/
EFI_STATUS
EFIAPI
StartGauge (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp
  )
{
  GAUGE_DATA_ENTRY          *GaugeEntryArray;
  UINTN                     GaugeDataSize;
  UINTN                     OldGaugeDataSize;
  GAUGE_DATA_HEADER         *OldGaugeData;
  UINT32                    Index;

  AcquireSpinLock (&mSmmPerfLock);

  Index = mGaugeData->NumberOfEntries;
  if (Index >= mMaxGaugeRecords) {
    //
    // Try to enlarge the scale of gauge array.
    //
    OldGaugeData      = mGaugeData;
    OldGaugeDataSize  = sizeof (GAUGE_DATA_HEADER) + sizeof (GAUGE_DATA_ENTRY) * mMaxGaugeRecords;

    mMaxGaugeRecords *= 2;
    GaugeDataSize     = sizeof (GAUGE_DATA_HEADER) + sizeof (GAUGE_DATA_ENTRY) * mMaxGaugeRecords;

    mGaugeData = AllocateZeroPool (GaugeDataSize);
    if (mGaugeData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Initialize new data array and migrate old data one.
    //
    mGaugeData = CopyMem (mGaugeData, OldGaugeData, OldGaugeDataSize);

    FreePool (OldGaugeData);
  }

  GaugeEntryArray               = (GAUGE_DATA_ENTRY *) (mGaugeData + 1);
  GaugeEntryArray[Index].Handle = (EFI_PHYSICAL_ADDRESS) (UINTN) Handle;

  if (Token != NULL) {
    AsciiStrnCpy (GaugeEntryArray[Index].Token, Token, SMM_PERFORMANCE_STRING_LENGTH);
  }
  if (Module != NULL) {
    AsciiStrnCpy (GaugeEntryArray[Index].Module, Module, SMM_PERFORMANCE_STRING_LENGTH);
  }

  if (TimeStamp == 0) {
    TimeStamp = GetPerformanceCounter ();
  }
  GaugeEntryArray[Index].StartTimeStamp = TimeStamp;

  mGaugeData->NumberOfEntries++;

  ReleaseSpinLock (&mSmmPerfLock);

  return EFI_SUCCESS;
}

/**
  Searches the performance measurement log from the beginning of the log
  for the first matching record that contains a zero end time and fills in a valid end time.

  Searches the performance measurement log from the beginning of the log
  for the first record that matches Handle, Token, and Module and has an end time value of zero.
  If the record can not be found then return EFI_NOT_FOUND.
  If the record is found and TimeStamp is not zero,
  then the end time in the record is filled in with the value specified by TimeStamp.
  If the record is found and TimeStamp is zero, then the end time in the matching record
  is filled in with the current time stamp value.

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               64-bit time stamp.

  @retval EFI_SUCCESS             The end of  the measurement was recorded.
  @retval EFI_NOT_FOUND           The specified measurement record could not be found.

**/
EFI_STATUS
EFIAPI
EndGauge (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp
  )
{
  GAUGE_DATA_ENTRY  *GaugeEntryArray;
  UINT32            Index;

  if (TimeStamp == 0) {
    TimeStamp = GetPerformanceCounter ();
  }

  Index = SmmSearchForGaugeEntry (Handle, Token, Module);
  if (Index >= mGaugeData->NumberOfEntries) {
    return EFI_NOT_FOUND;
  }
  GaugeEntryArray = (GAUGE_DATA_ENTRY  *) (mGaugeData + 1);
  GaugeEntryArray[Index].EndTimeStamp = TimeStamp;

  return EFI_SUCCESS;
}

/**
  Retrieves a previously logged performance measurement.

  Retrieves the performance log entry from the performance log specified by LogEntryKey.
  If it stands for a valid entry, then EFI_SUCCESS is returned and
  GaugeDataEntry stores the pointer to that entry.

  @param  LogEntryKey             The key for the previous performance measurement log entry.
                                  If 0, then the first performance measurement log entry is retrieved.
  @param  GaugeDataEntry          The indirect pointer to the gauge data entry specified by LogEntryKey
                                  if the retrieval is successful.

  @retval EFI_SUCCESS             The GuageDataEntry is successfully found based on LogEntryKey.
  @retval EFI_NOT_FOUND           The LogEntryKey is the last entry (equals to the total entry number).
  @retval EFI_INVALIDE_PARAMETER  The LogEntryKey is not a valid entry (greater than the total entry number).
  @retval EFI_INVALIDE_PARAMETER  GaugeDataEntry is NULL.

**/
EFI_STATUS
EFIAPI
GetGauge (
  IN  UINTN               LogEntryKey,
  OUT GAUGE_DATA_ENTRY    **GaugeDataEntry
  )
{
  UINTN               NumberOfEntries;
  GAUGE_DATA_ENTRY    *LogEntryArray;

  NumberOfEntries = (UINTN) (mGaugeData->NumberOfEntries);
  if (LogEntryKey > NumberOfEntries) {
    return EFI_INVALID_PARAMETER;
  }
  if (LogEntryKey == NumberOfEntries) {
    return EFI_NOT_FOUND;
  }

  LogEntryArray   = (GAUGE_DATA_ENTRY *) (mGaugeData + 1);

  if (GaugeDataEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *GaugeDataEntry = &LogEntryArray[LogEntryKey];

  return EFI_SUCCESS;
}


/**
  This function check if the address is in SMRAM.

  @param Buffer  the buffer address to be checked.
  @param Length  the buffer length to be checked.

  @retval TRUE  this address is in SMRAM.
  @retval FALSE this address is NOT in SMRAM.
**/
BOOLEAN
IsAddressInSmram (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  UINTN  Index;

  for (Index = 0; Index < mSmramRangeCount; Index ++) {
    if (((Buffer >= mSmramRanges[Index].CpuStart) && (Buffer < mSmramRanges[Index].CpuStart + mSmramRanges[Index].PhysicalSize)) ||
        ((mSmramRanges[Index].CpuStart >= Buffer) && (mSmramRanges[Index].CpuStart < Buffer + Length))) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Communication service SMI Handler entry.

  This SMI handler provides services for the variable wrapper driver.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer     A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers 
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should 
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still 
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.
**/
EFI_STATUS
EFIAPI
SmmPerformanceHandler (
  IN     EFI_HANDLE                    DispatchHandle,
  IN     CONST VOID                   *RegisterContext,
  IN OUT VOID                           *CommBuffer,
  IN OUT UINTN                          *CommBufferSize
  )
{
  EFI_STATUS            Status;
  SMM_PERF_COMMUNICATE  *SmmPerfCommData;
  GAUGE_DATA_ENTRY      *GaugeData; 
  UINTN                 DataSize;

  GaugeData = NULL;
  
  ASSERT (CommBuffer != NULL);

  SmmPerfCommData = (SMM_PERF_COMMUNICATE*)CommBuffer;

  switch (SmmPerfCommData->Function) {
    case SMM_PERF_FUNCTION_GET_GAUGE_ENTRY_NUMBER :
       SmmPerfCommData->NumberOfEntries = mGaugeData->NumberOfEntries;
       return Status = EFI_SUCCESS;
       break;

    case SMM_PERF_FUNCTION_GET_GAUGE_DATA :
       if ( SmmPerfCommData->GaugeData == NULL || SmmPerfCommData->NumberOfEntries <= 0 ||
	     (SmmPerfCommData->LogEntryKey + SmmPerfCommData->NumberOfEntries) > mGaugeData->NumberOfEntries) {
         Status = EFI_INVALID_PARAMETER;
         break;
       } 

       Status = GetGauge(SmmPerfCommData->LogEntryKey, &GaugeData);
       if (EFI_ERROR(Status)) {
         break;
       }
	   
       //
       // Sanity check
       //
       DataSize = SmmPerfCommData->NumberOfEntries * sizeof(GAUGE_DATA_ENTRY);
       if (IsAddressInSmram ((EFI_PHYSICAL_ADDRESS)(UINTN)SmmPerfCommData->GaugeData, DataSize)) {
         DEBUG ((EFI_D_ERROR, "Smm Performance Data buffer is in SMRAM!\n"));
         Status = EFI_ACCESS_DENIED;
         break ;
       }

       CopyMem(
         (UINT8*)SmmPerfCommData->GaugeData, 
         (UINT8*)GaugeData, 
         DataSize
         );
       break;

    default:
       ASSERT (FALSE);
       Status = EFI_UNSUPPORTED;
  }		

  SmmPerfCommData->ReturnStatus = Status;
  return EFI_SUCCESS;
}

/**
  SmmBase2 protocol notify callback function, when SMST and SMM memory service get initialized 
  this function is callbacked to to initialize the Smm Performance Lib 

  @param  Event    The event of notify protocol.
  @param  Context  Notify event context.

**/
VOID
EFIAPI
InitializeSmmCorePerformanceLib (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_SMM_ACCESS2_PROTOCOL  *SmmAccess;
  UINTN                     Size;


  //
  // Initialize spin lock
  //
  InitializeSpinLock (&mSmmPerfLock);

  mMaxGaugeRecords = INIT_SMM_GAUGE_DATA_ENTRIES + PcdGet8 (PcdMaxPeiPerformanceLogEntries);

  mGaugeData = AllocateZeroPool (sizeof (GAUGE_DATA_HEADER) + (sizeof (GAUGE_DATA_ENTRY) * mMaxGaugeRecords));
  ASSERT (mGaugeData != NULL);
  
  //
  // Get SMRAM information
  //
  Status = gBS->LocateProtocol (&gEfiSmmAccess2ProtocolGuid, NULL, (VOID **)&SmmAccess);
  ASSERT_EFI_ERROR (Status);

  Size = 0;
  Status = SmmAccess->GetCapabilities (SmmAccess, &Size, NULL);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    Size,
                    (VOID **)&mSmramRanges
                    );
  ASSERT_EFI_ERROR (Status);

  Status = SmmAccess->GetCapabilities (SmmAccess, &Size, mSmramRanges);
  ASSERT_EFI_ERROR (Status);

  mSmramRangeCount = Size / sizeof (EFI_SMRAM_DESCRIPTOR);

  //
  // Install the protocol interfaces.
  //
  Status = gSmst->SmmInstallProtocolInterface (
                    &mHandle,
                    &gSmmPerformanceProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mPerformanceInterface
                    );
  ASSERT_EFI_ERROR (Status);

  ///
  /// Register SMM Performance SMI handler
  ///
  Handle = NULL;
  Status = gSmst->SmiHandlerRegister (SmmPerformanceHandler, &gSmmPerformanceProtocolGuid, &Handle);
  ASSERT_EFI_ERROR (Status);
}

/**
  The constructor function initializes Performance infrastructure for DXE phase.

  The constructor function publishes Performance protocol, allocates memory to log DXE performance
  and merges PEI performance data to DXE performance log.
  It will ASSERT() if one of these operations fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmCorePerformanceLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  VOID        *Registration;

  mPerformanceMeasurementEnabled =  (BOOLEAN) ((PcdGet8(PcdPerformanceLibraryPropertyMask) & PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED) != 0);
  if (!mPerformanceMeasurementEnabled) {
    //
    // Do not initialize performance infrastructure if not required.
    //
    return EFI_SUCCESS;
  }

  //
  // Create the events to do the library init.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  InitializeSmmCorePerformanceLib,
                  NULL,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifications on this event
  //
  Status = gBS->RegisterProtocolNotify (
                  &gEfiSmmBase2ProtocolGuid,
                  Event,
                  &Registration
                  );

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Adds a record at the end of the performance measurement log
  that records the start time of a performance measurement.

  Adds a record to the end of the performance measurement log
  that contains the Handle, Token, and Module.
  The end time of the new record must be set to zero.
  If TimeStamp is not zero, then TimeStamp is used to fill in the start time in the record.
  If TimeStamp is zero, the start time in the record is filled in with the value
  read from the current time stamp.

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
  EFI_STATUS  Status;

  Status = StartGauge (Handle, Token, Module, TimeStamp);
  return (RETURN_STATUS) Status;
}

/**
  Searches the performance measurement log from the beginning of the log
  for the first matching record that contains a zero end time and fills in a valid end time.

  Searches the performance measurement log from the beginning of the log
  for the first record that matches Handle, Token, and Module and has an end time value of zero.
  If the record can not be found then return RETURN_NOT_FOUND.
  If the record is found and TimeStamp is not zero,
  then the end time in the record is filled in with the value specified by TimeStamp.
  If the record is found and TimeStamp is zero, then the end time in the matching record
  is filled in with the current time stamp value.

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
  EFI_STATUS  Status;

  Status = EndGauge (Handle, Token, Module, TimeStamp);
  return (RETURN_STATUS) Status;
}

/**
  Attempts to retrieve a performance measurement log entry from the performance measurement log.

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
  EFI_STATUS        Status;
  GAUGE_DATA_ENTRY  *GaugeData;

  GaugeData = NULL;
  
  ASSERT (Handle != NULL);
  ASSERT (Token != NULL);
  ASSERT (Module != NULL);
  ASSERT (StartTimeStamp != NULL);
  ASSERT (EndTimeStamp != NULL);

  Status = GetGauge (LogEntryKey++, &GaugeData);

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

  return LogEntryKey;
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
  return mPerformanceMeasurementEnabled;
}


