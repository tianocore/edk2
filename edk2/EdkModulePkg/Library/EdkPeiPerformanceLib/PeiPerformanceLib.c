/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeiPerformanceLib.c

Abstract:

  Performance Library

--*/

/**
  Gets PEI the GUID HOB for PEI performance.

  This internal function searches for the GUID HOB for PEI performance.
  If that GUID HOB is not found, it will build a new one.
  It returns the data area of that GUID HOB to record performance log.    

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.

  @retval The index of log entry in the array.

**/
PEI_PERFORMANCE_LOG_HEADER *
InternalGetPerformanceHobLog (
  VOID
  )
{
  EFI_HOB_GUID_TYPE           *GuidHob;
  PEI_PERFORMANCE_LOG_HEADER  *PeiPerformanceLog;
  UINTN                       PeiPerformanceLogSize;

  GuidHob = GetFirstGuidHob (&gPeiPerformanceHobGuid);
  
  if (GuidHob != NULL) {
    //
    // PEI Performance HOB was found, then return the existing one.
    //
    PeiPerformanceLog = GET_GUID_HOB_DATA (GuidHob);
  } else {
    //
    // PEI Performance HOB was not found, then build one.
    //
    PeiPerformanceLogSize = sizeof (PEI_PERFORMANCE_LOG_HEADER) + 
                            sizeof (PEI_PERFORMANCE_LOG_ENTRY) * MAX_PEI_PERFORMANCE_LOG_ENTRIES;
    PeiPerformanceLog     = BuildGuidHob (&gPeiPerformanceHobGuid, PeiPerformanceLogSize);
    PeiPerformanceLog     = ZeroMem (PeiPerformanceLog, PeiPerformanceLogSize);
  }

  return PeiPerformanceLog;
}

/**
  Searches in the log array with keyword Handle, Token and Module.

  This internal function searches for the log entry in the log array.
  If there is an entry that exactly matches the given key word triple
  and its end time stamp is zero, then the index of that log entry is returned;
  otherwise, the the number of log entries in the array is returned.  

  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.

  @retval The index of log entry in the array.

**/
UINT32
InternalSearchForLogEntry (
  IN PEI_PERFORMANCE_LOG_HEADER *PeiPerformanceLog,
  IN CONST VOID                 *Handle,  OPTIONAL
  IN CONST CHAR8                *Token,   OPTIONAL
  IN CONST CHAR8                *Module   OPTIONAL
  )
{
  UINT32                    Index;
  UINT32                    NumberOfEntries;
  PEI_PERFORMANCE_LOG_ENTRY *LogEntryArray;
  

  if (Token == NULL) {
    Token = "";
  }
  if (Module == NULL) {
    Module = "";
  }
  NumberOfEntries = PeiPerformanceLog->NumberOfEntries;
  LogEntryArray   = (PEI_PERFORMANCE_LOG_ENTRY *) (PeiPerformanceLog + 1);
  
  for (Index = 0; Index < NumberOfEntries; Index++) {
    if ((LogEntryArray[Index].Handle == (EFI_PHYSICAL_ADDRESS) (UINTN) Handle) &&
         AsciiStrnCmp (LogEntryArray[Index].Token, Token, PEI_PERFORMANCE_STRING_LENGTH) == 0 &&
         AsciiStrnCmp (LogEntryArray[Index].Module, Module, PEI_PERFORMANCE_STRING_LENGTH) == 0 &&
         LogEntryArray[Index].EndTimeStamp == 0
       ) {
      break;
    }
  }
  return Index;
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
  PEI_PERFORMANCE_LOG_HEADER  *PeiPerformanceLog;
  PEI_PERFORMANCE_LOG_ENTRY   *LogEntryArray;
  UINT32                      Index;

  PeiPerformanceLog = InternalGetPerformanceHobLog ();
  
  if (PeiPerformanceLog->NumberOfEntries >= MAX_PEI_PERFORMANCE_LOG_ENTRIES) {
    return RETURN_OUT_OF_RESOURCES;
  }
  Index                       = PeiPerformanceLog->NumberOfEntries++;
  LogEntryArray               = (PEI_PERFORMANCE_LOG_ENTRY *) (PeiPerformanceLog + 1);        
  LogEntryArray[Index].Handle = (EFI_PHYSICAL_ADDRESS) (UINTN) Handle;

  if (Token != NULL) {
    AsciiStrnCpy (LogEntryArray[Index].Token, Token, PEI_PERFORMANCE_STRING_LENGTH);
  }
  if (Module != NULL) {
    AsciiStrnCpy (LogEntryArray[Index].Module, Module, PEI_PERFORMANCE_STRING_LENGTH);
  }

  if (TimeStamp == 0) {
    TimeStamp = GetPerformanceCounter ();
  }
  LogEntryArray[Index].StartTimeStamp = TimeStamp;

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
  If this function is called multiple times for the same record, then the end time is overwritten.

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
  PEI_PERFORMANCE_LOG_HEADER  *PeiPerformanceLog;
  PEI_PERFORMANCE_LOG_ENTRY   *LogEntryArray;
  UINT32                      Index;

  if (TimeStamp == 0) {
    TimeStamp = GetPerformanceCounter ();
  }
  
  PeiPerformanceLog = InternalGetPerformanceHobLog ();
  Index             = InternalSearchForLogEntry (PeiPerformanceLog, Handle, Token, Module);
  if (Index >= PeiPerformanceLog->NumberOfEntries) {
    return RETURN_NOT_FOUND;
  }
  LogEntryArray     = (PEI_PERFORMANCE_LOG_ENTRY *) (PeiPerformanceLog + 1);
  LogEntryArray[Index].EndTimeStamp = TimeStamp;

  return RETURN_SUCCESS;
}

/**
  Retrieves a previously logged performance measurement. 
  
  Looks up the record that matches Handle, Token, and Module.
  If the record can not be found then return RETURN_NOT_FOUND.
  If the record is found then the start of the measurement is returned in StartTimeStamp,
  and the end of the measurement is returned in EndTimeStamp.

  @param  LogEntryKey             The key for the previous performance measurement log entry.
                                  If 0, then the first performance measurement log entry is retrieved.
  @param  Handle                  Pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   Pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  Pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  StartTimeStamp          The 64-bit time stamp that was recorded when the measurement was started.
  @param  EndTimeStamp            The 64-bit time stamp that was recorded when the measurement was ended.

  @return The key for the current performance log entry.

**/
UINTN
EFIAPI
GetPerformanceMeasurement (
  UINTN           LogEntryKey, 
  OUT CONST VOID  **Handle,
  OUT CONST CHAR8 **Token,
  OUT CONST CHAR8 **Module,
  OUT UINT64      *StartTimeStamp,
  OUT UINT64      *EndTimeStamp
  )
{
  PEI_PERFORMANCE_LOG_HEADER  *PeiPerformanceLog;
  PEI_PERFORMANCE_LOG_ENTRY   *CurrentLogEntry;
  PEI_PERFORMANCE_LOG_ENTRY   *LogEntryArray;
  UINTN                       NumberOfEntries;

  ASSERT (Handle != NULL);
  ASSERT (Token != NULL);
  ASSERT (Module != NULL);
  ASSERT (StartTimeStamp != NULL);
  ASSERT (EndTimeStamp != NULL);
  
  PeiPerformanceLog = InternalGetPerformanceHobLog ();
 
  NumberOfEntries   = (UINTN) (PeiPerformanceLog->NumberOfEntries);
  LogEntryArray     = (PEI_PERFORMANCE_LOG_ENTRY *) (PeiPerformanceLog + 1);
  //
  // Make sure that LogEntryKey is a valid log entry key.
  //
  ASSERT (LogEntryKey <= NumberOfEntries);
  
  if (LogEntryKey == NumberOfEntries) {
    return 0;
  }

  CurrentLogEntry = &(LogEntryArray[LogEntryKey++]);

  *Handle         = (VOID *) (UINTN) (CurrentLogEntry->Handle);
  *Token          = CurrentLogEntry->Token;
  *Module         = CurrentLogEntry->Module;
  *StartTimeStamp = CurrentLogEntry->StartTimeStamp;
  *EndTimeStamp   = CurrentLogEntry->EndTimeStamp;

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
  return ((PcdGet8(PcdPerformanceLibraryPropertyMask) & PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED) != 0);
}
