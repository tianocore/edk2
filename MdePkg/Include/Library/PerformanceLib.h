/** @file
  Library that provides services to measure module execution performance

  Copyright (c) 2004, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  PerformanceLib.h

**/

#ifndef __PERFORMANCE_LIB_H__
#define __PERFORMANCE_LIB_H__

//
// Performance library propery mask bits
//
#define PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED  0x00000001

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
  );

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
  );

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
  );

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
  );

/**
  Macro that calls EndPerformanceMeasurement().

  If the PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of PcdPerformanceLibraryPropertyMask is set,
  then EndPerformanceMeasurement() is called.

**/
#define PERF_END(Handle, Token, Module, TimeStamp)                    \
  do {                                                                \
    if (PerformanceMeasurementEnabled ()) {                           \
      EndPerformanceMeasurement (Handle, Token, Module, TimeStamp);   \
    }                                                                 \
  } while (FALSE)

/**
  Macro that calls StartPerformanceMeasurement().

  If the PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of PcdPerformanceLibraryPropertyMask is set,
  then StartPerformanceMeasurement() is called.

**/
#define PERF_START(Handle, Token, Module, TimeStamp)                  \
  do {                                                                \
    if (PerformanceMeasurementEnabled ()) {                           \
      StartPerformanceMeasurement (Handle, Token, Module, TimeStamp); \
    }                                                                 \
  } while (FALSE)

/**
  Macro that marks the beginning of performance measurement source code.

  If the PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of PcdPerformanceLibraryPropertyMask is set,
  then this macro marks the beginning of source code that is included in a module.
  Otherwise, the source lines between PERF_CODE_BEGIN() and PERF_CODE_END() are not included in a module.

**/
#define PERF_CODE_BEGIN()  do { if (PerformanceMeasurementEnabled ()) { UINT8  __PerformanceCodeLocal

/**
  Macro that marks the end of performance measurement source code.

  If the PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of PcdPerformanceLibraryPropertyMask is set,
  then this macro marks the end of source code that is included in a module.
  Otherwise, the source lines between PERF_CODE_BEGIN() and PERF_CODE_END() are not included in a module.

**/
#define PERF_CODE_END()    __PerformanceCodeLocal = 0; } } while (FALSE)

/**
  Macro that declares a section of performance measurement source code.

  If the PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED bit of PcdPerformanceLibraryPropertyMask is set,
  then the source code specified by Expression is included in a module.
  Otherwise, the source specified by Expression is not included in a module.

  @param  Expression              Performance measurement source code to include in a module.

**/
#define PERF_CODE(Expression)  \
  PERF_CODE_BEGIN ();          \
  Expression                   \
  PERF_CODE_END ()


#endif
