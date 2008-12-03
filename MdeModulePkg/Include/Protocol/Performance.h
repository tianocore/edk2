/** @file
  Performance protocol interfaces to support cross module performance logging. 

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PERFORMANCE_H__
#define __PERFORMANCE_H__

#define PERFORMANCE_PROTOCOL_GUID \
  { 0x76b6bdfa, 0x2acd, 0x4462, {0x9E, 0x3F, 0xcb, 0x58, 0xC9, 0x69, 0xd9, 0x37 } }

//
// Forward reference for pure ANSI compatability
//
typedef struct _PERFORMANCE_PROTOCOL PERFORMANCE_PROTOCOL;

#define SEC_TOK                         "SEC"
#define DXE_TOK                         "DXE"
#define SHELL_TOK                       "SHELL"
#define PEI_TOK                         "PEI"
#define BDS_TOK                         "BDS"
#define DRIVERBINDING_START_TOK         "DB:Start:"
#define DRIVERBINDING_SUPPORT_TOK       "DB:Support:"
#define START_IMAGE_TOK                 "StartImage:"
#define LOAD_IMAGE_TOK                  "LoadImage:"

//
// DXE_PERFORMANCE_STRING_SIZE must be a multiple of 8.
//
#define DXE_PERFORMANCE_STRING_SIZE     32
#define DXE_PERFORMANCE_STRING_LENGTH   (DXE_PERFORMANCE_STRING_SIZE - 1)

//
// The default guage entries number for DXE phase.
//
#define INIT_DXE_GAUGE_DATA_ENTRIES     800

typedef struct {
  EFI_PHYSICAL_ADDRESS  Handle;
  CHAR8                 Token[DXE_PERFORMANCE_STRING_SIZE];  /// Measured token string name 
  CHAR8                 Module[DXE_PERFORMANCE_STRING_SIZE]; /// Module string name
  UINT64                StartTimeStamp;                      /// Start time point
  UINT64                EndTimeStamp;                        /// End time point
} GAUGE_DATA_ENTRY;

//
// The header must be aligned at 8 bytes
//
typedef struct {
  UINT32                NumberOfEntries; /// The number of all performance guage entries
  UINT32                Reserved;
} GAUGE_DATA_HEADER;

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
typedef
EFI_STATUS
(EFIAPI * PERFORMANCE_START_GAUGE)(
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
typedef
EFI_STATUS
(EFIAPI * PERFORMANCE_END_GAUGE)(
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
typedef
EFI_STATUS
(EFIAPI * PERFORMANCE_GET_GAUGE)(
  IN  UINTN               LogEntryKey,
  OUT GAUGE_DATA_ENTRY    **GaugeDataEntry
  );

struct _PERFORMANCE_PROTOCOL {
  PERFORMANCE_START_GAUGE             StartGauge;
  PERFORMANCE_END_GAUGE               EndGauge;
  PERFORMANCE_GET_GAUGE               GetGauge;
};

extern EFI_GUID gPerformanceProtocolGuid;

#endif
