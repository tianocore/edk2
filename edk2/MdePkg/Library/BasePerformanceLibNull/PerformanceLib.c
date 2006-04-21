/** @file
  Base Performance Library which provides no service.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  PerformanceLib.c

**/

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
  IN CONST CHAR8  *Token,
  IN CONST CHAR8  *Module,
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
  IN CONST CHAR8  *Token,
  IN CONST CHAR8  *Module,
  IN UINT64       TimeStamp
  )
{
  return RETURN_SUCCESS;
}

/**
  Retrieves a previously logged performance measurement. 
  
  Retrieves the performance log entry from the performance log
  that immediately follows the log entry specified by LogEntryKey.
  If LogEntryKey is zero, then the first entry from the performance log is returned.
  If the log entry specified by LogEntryKey is the last entry in the performance log,
  then 0 is returned.  Otherwise, the performance log entry is returned in Handle,
  Token, Module, StartTimeStamp, and EndTimeStamp.
  The key for the current performance log entry is returned. 

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
  ASSERT (Handle != NULL);
  ASSERT (Token != NULL);
  ASSERT (Module != NULL);
  ASSERT (StartTimeStamp != NULL);
  ASSERT (EndTimeStamp != NULL);

  return 0;
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
