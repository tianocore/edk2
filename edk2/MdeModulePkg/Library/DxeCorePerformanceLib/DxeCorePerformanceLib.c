/** @file
  Support for measurement of DXE performance

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiDxe.h>

#include <Protocol/Performance.h>
#include <Guid/PeiPerformanceHob.h>

#include <Library/PerformanceLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

//
// Interface declarations for Performance Protocol.
//
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
  );

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
  );

/**
  Retrieves a previously logged performance measurement.

  Retrieves the performance log entry from the performance log specified by LogEntryKey.
  If it stands for a valid entry, then EFI_SUCCESS is returned and
  GaugeDataEntry stores the pointer to that entry.

  @param  LogEntryKey             The key for the previous performance measurement log entry.
                                  If 0, then the first performance measurement log entry is retrieved.
  @param  GaugeDataEntry          The indirect pointer to the gauge data entry specified by LogEntryKey
                                  if the retrieval is successful.

  @retval EFI_SUCCESS             The GuageDataEntry is successfuly found based on LogEntryKey.
  @retval EFI_NOT_FOUND           The LogEntryKey is the last entry (equals to the total entry number).
  @retval EFI_INVALIDE_PARAMETER  The LogEntryKey is not a valid entry (greater than the total entry number).
  @retval EFI_INVALIDE_PARAMETER  GaugeDataEntry is NULL.

**/
EFI_STATUS
EFIAPI
GetGauge (
  IN  UINTN               LogEntryKey,
  OUT GAUGE_DATA_ENTRY    **GaugeDataEntry
  );

//
// Definition for global variables.
//
GAUGE_DATA_HEADER    *mGaugeData;
UINT32               mMaxGaugeRecords;

EFI_HANDLE           mHandle = NULL;
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
InternalSearchForGaugeEntry (
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
         AsciiStrnCmp (GaugeEntryArray[Index].Token, Token, PEI_PERFORMANCE_STRING_LENGTH) == 0 &&
         AsciiStrnCmp (GaugeEntryArray[Index].Module, Module, PEI_PERFORMANCE_STRING_LENGTH) == 0 &&
         GaugeEntryArray[Index].EndTimeStamp == 0
       ) {
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

  Index = mGaugeData->NumberOfEntries;
  if (Index >= mMaxGaugeRecords) {
    //
    // Try to enlarge the scale of gauge arrary.
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
    // Initialize new data arry and migrate old data one.
    //
    mGaugeData = CopyMem (mGaugeData, OldGaugeData, OldGaugeDataSize);

    FreePool (OldGaugeData);
  }

  GaugeEntryArray               = (GAUGE_DATA_ENTRY *) (mGaugeData + 1);
  GaugeEntryArray[Index].Handle = (EFI_PHYSICAL_ADDRESS) (UINTN) Handle;

  if (Token != NULL) {
    AsciiStrnCpy (GaugeEntryArray[Index].Token, Token, DXE_PERFORMANCE_STRING_LENGTH);
  }
  if (Module != NULL) {
    AsciiStrnCpy (GaugeEntryArray[Index].Module, Module, DXE_PERFORMANCE_STRING_LENGTH);
  }

  if (TimeStamp == 0) {
    TimeStamp = GetPerformanceCounter ();
  }
  GaugeEntryArray[Index].StartTimeStamp = TimeStamp;

  mGaugeData->NumberOfEntries++;

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

  Index = InternalSearchForGaugeEntry (Handle, Token, Module);
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

  @retval EFI_SUCCESS             The GuageDataEntry is successfuly found based on LogEntryKey.
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
  Dumps all the PEI performance log to DXE performance gauge array.

  This internal function dumps all the PEI performance log to the DXE performance gauge array.
  It retrieves the optional GUID HOB for PEI performance and then saves the performance data
  to DXE performance data structures.

**/
VOID
InternalGetPeiPerformance (
  VOID
  )
{
  EFI_HOB_GUID_TYPE                 *GuidHob;
  PEI_PERFORMANCE_LOG_HEADER        *LogHob;
  PEI_PERFORMANCE_LOG_ENTRY         *LogEntryArray;
  GAUGE_DATA_ENTRY                  *GaugeEntryArray;
  UINT32                            Index;
  UINT32                            NumberOfEntries;

  NumberOfEntries = 0;
  GaugeEntryArray = (GAUGE_DATA_ENTRY *) (mGaugeData + 1);

  //
  // Dump PEI Log Entries to DXE Guage Data structure.
  //
  GuidHob = GetFirstGuidHob (&gPeiPerformanceHobGuid);
  if (GuidHob != NULL) {
    LogHob          = GET_GUID_HOB_DATA (GuidHob);
    LogEntryArray   = (PEI_PERFORMANCE_LOG_ENTRY *) (LogHob + 1);
    GaugeEntryArray = (GAUGE_DATA_ENTRY *) (mGaugeData + 1);

    NumberOfEntries = LogHob->NumberOfEntries;
    for (Index = 0; Index < NumberOfEntries; Index++) {
      GaugeEntryArray[Index].Handle         = LogEntryArray[Index].Handle;
      AsciiStrnCpy (GaugeEntryArray[Index].Token,  LogEntryArray[Index].Token,  DXE_PERFORMANCE_STRING_LENGTH);
      AsciiStrnCpy (GaugeEntryArray[Index].Module, LogEntryArray[Index].Module, DXE_PERFORMANCE_STRING_LENGTH);
      GaugeEntryArray[Index].StartTimeStamp = LogEntryArray[Index].StartTimeStamp;
      GaugeEntryArray[Index].EndTimeStamp   = LogEntryArray[Index].EndTimeStamp;
    }
  }
  mGaugeData->NumberOfEntries = NumberOfEntries;
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
DxeCorePerformanceLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;

  if (!PerformanceMeasurementEnabled ()) {
    //
    // Do not initialize performance infrastructure if not required.
    //
    return EFI_SUCCESS;
  }
  //
  // Install the protocol interfaces.
  //
  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gPerformanceProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPerformanceInterface
                  );
  ASSERT_EFI_ERROR (Status);

  mMaxGaugeRecords = INIT_DXE_GAUGE_DATA_ENTRIES + PcdGet8 (PcdMaxPeiPerformanceLogEntries);

  mGaugeData = AllocateZeroPool (sizeof (GAUGE_DATA_HEADER) + (sizeof (GAUGE_DATA_ENTRY) * mMaxGaugeRecords));
  ASSERT (mGaugeData != NULL);

  InternalGetPeiPerformance ();

  return Status;
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
                                  On exit, the key of the next performance lof entry entry.
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
  return (BOOLEAN) ((PcdGet8(PcdPerformanceLibraryPropertyMask) & PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED) != 0);
}
