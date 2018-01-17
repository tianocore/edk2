/** @file
  Master header files for SmmCorePerformanceLib instance.

  This header file holds the prototypes of the SMM Performance and PerformanceEx Protocol published by this
  library instance at its constructor.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
  
#ifndef _SMM_CORE_PERFORMANCE_LIB_INTERNAL_H_
#define _SMM_CORE_PERFORMANCE_LIB_INTERNAL_H_


#include <Guid/Performance.h>
#include <Guid/ExtendedFirmwarePerformance.h>
#include <Guid/FirmwarePerformance.h>
#include <Guid/ZeroGuid.h>
#include <Guid/EventGroup.h>

#include <Library/SmmServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PerformanceLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>                   
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/SmmMemLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#include <Protocol/SmmBase2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePathToText.h>

//
// Interface declarations for SMM PerformanceEx Protocol.
//
/**
  Adds a record at the end of the performance measurement log
  that records the start time of a performance measurement.

  Adds a record to the end of the performance measurement log
  that contains the Handle, Token, Module and Identifier.
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
  @param  Identifier              32-bit identifier. If the value is 0, the created record
                                  is same as the one created by StartGauge of PERFORMANCE_PROTOCOL.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_OUT_OF_RESOURCES    There are not enough resources to record the measurement.

**/
EFI_STATUS
EFIAPI
StartGaugeEx (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp,
  IN UINT32       Identifier
  );

/**
  Searches the performance measurement log from the beginning of the log
  for the first matching record that contains a zero end time and fills in a valid end time.

  Searches the performance measurement log from the beginning of the log
  for the first record that matches Handle, Token, Module and Identifier and has an end time value of zero.
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
  @param  Identifier              32-bit identifier. If the value is 0, the found record
                                  is same as the one found by EndGauge of PERFORMANCE_PROTOCOL.

  @retval EFI_SUCCESS             The end of  the measurement was recorded.
  @retval EFI_NOT_FOUND           The specified measurement record could not be found.

**/
EFI_STATUS
EFIAPI
EndGaugeEx (
  IN CONST VOID   *Handle,  OPTIONAL
  IN CONST CHAR8  *Token,   OPTIONAL
  IN CONST CHAR8  *Module,  OPTIONAL
  IN UINT64       TimeStamp,
  IN UINT32       Identifier
  );

/**
  Retrieves a previously logged performance measurement.
  It can also retrieve the log created by StartGauge and EndGauge of PERFORMANCE_PROTOCOL,
  and then assign the Identifier with 0.

  Retrieves the performance log entry from the performance log specified by LogEntryKey.
  If it stands for a valid entry, then EFI_SUCCESS is returned and
  GaugeDataEntryEx stores the pointer to that entry.

  @param  LogEntryKey             The key for the previous performance measurement log entry.
                                  If 0, then the first performance measurement log entry is retrieved.
  @param  GaugeDataEntryEx        The indirect pointer to the extended gauge data entry specified by LogEntryKey
                                  if the retrieval is successful.

  @retval EFI_SUCCESS             The GuageDataEntryEx is successfully found based on LogEntryKey.
  @retval EFI_NOT_FOUND           The LogEntryKey is the last entry (equals to the total entry number).
  @retval EFI_INVALIDE_PARAMETER  The LogEntryKey is not a valid entry (greater than the total entry number).
  @retval EFI_INVALIDE_PARAMETER  GaugeDataEntryEx is NULL.

**/
EFI_STATUS
EFIAPI
GetGaugeEx (
  IN  UINTN                 LogEntryKey,
  OUT GAUGE_DATA_ENTRY_EX   **GaugeDataEntryEx
  );

//
// Interface declarations for SMM Performance Protocol.
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
  It can also retrieve the log created by StartGaugeEx and EndGaugeEx of PERFORMANCE_EX_PROTOCOL,
  and then eliminate the Identifier.

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
  );


#endif
