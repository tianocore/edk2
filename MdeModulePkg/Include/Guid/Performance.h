/** @file
  This file defines performance-related definitions, including the format of:
  * performance GUID HOB.
  * performance protocol interfaces.
  * performance variables.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PERFORMANCE_DATA_H__
#define __PERFORMANCE_DATA_H__

#define PERFORMANCE_PROPERTY_REVISION  0x1

typedef struct {
  UINT32    Revision;
  UINT32    Reserved;
  UINT64    Frequency;
  UINT64    TimerStartValue;
  UINT64    TimerEndValue;
} PERFORMANCE_PROPERTY;

//
// PEI_PERFORMANCE_STRING_SIZE must be a multiple of 8.
//
#define PEI_PERFORMANCE_STRING_SIZE    8
#define PEI_PERFORMANCE_STRING_LENGTH  (PEI_PERFORMANCE_STRING_SIZE - 1)

typedef struct {
  EFI_PHYSICAL_ADDRESS    Handle;
  CHAR8                   Token[PEI_PERFORMANCE_STRING_SIZE];  ///< Measured token string name.
  CHAR8                   Module[PEI_PERFORMANCE_STRING_SIZE]; ///< Module string name.
  UINT64                  StartTimeStamp;                      ///< Start time point.
  UINT64                  EndTimeStamp;                        ///< End time point.
} PEI_PERFORMANCE_LOG_ENTRY;

//
// The header must be aligned at 8 bytes.
//
typedef struct {
  UINT32    NumberOfEntries;              ///< The number of all performance log entries.
  UINT32    Reserved;
} PEI_PERFORMANCE_LOG_HEADER;

#define PERFORMANCE_PROTOCOL_GUID \
  { 0x76b6bdfa, 0x2acd, 0x4462, { 0x9E, 0x3F, 0xcb, 0x58, 0xC9, 0x69, 0xd9, 0x37 } }

#define PERFORMANCE_EX_PROTOCOL_GUID \
  { 0x1ea81bec, 0xf01a, 0x4d98, { 0xa2, 0x1,  0x4a, 0x61, 0xce, 0x2f, 0xc0, 0x22 } }

//
// Forward reference for pure ANSI compatibility
//
typedef struct _PERFORMANCE_PROTOCOL     PERFORMANCE_PROTOCOL;
typedef struct _PERFORMANCE_EX_PROTOCOL  PERFORMANCE_EX_PROTOCOL;

//
// DXE_PERFORMANCE_STRING_SIZE must be a multiple of 8.
//
#define DXE_PERFORMANCE_STRING_SIZE    32
#define DXE_PERFORMANCE_STRING_LENGTH  (DXE_PERFORMANCE_STRING_SIZE - 1)

//
// The default guage entries number for DXE phase.
//
#define INIT_DXE_GAUGE_DATA_ENTRIES  800

typedef struct {
  EFI_PHYSICAL_ADDRESS    Handle;
  CHAR8                   Token[DXE_PERFORMANCE_STRING_SIZE];  ///< Measured token string name.
  CHAR8                   Module[DXE_PERFORMANCE_STRING_SIZE]; ///< Module string name.
  UINT64                  StartTimeStamp;                      ///< Start time point.
  UINT64                  EndTimeStamp;                        ///< End time point.
} GAUGE_DATA_ENTRY;

typedef struct {
  EFI_PHYSICAL_ADDRESS    Handle;
  CHAR8                   Token[DXE_PERFORMANCE_STRING_SIZE];  ///< Measured token string name.
  CHAR8                   Module[DXE_PERFORMANCE_STRING_SIZE]; ///< Module string name.
  UINT64                  StartTimeStamp;                      ///< Start time point.
  UINT64                  EndTimeStamp;                        ///< End time point.
  UINT32                  Identifier;                          ///< Identifier.
} GAUGE_DATA_ENTRY_EX;

//
// The header must be aligned at 8 bytes
//
typedef struct {
  UINT32    NumberOfEntries;             ///< The number of all performance gauge entries.
  UINT32    Reserved;
} GAUGE_DATA_HEADER;

//
// SMM Performance Protocol definitions
//

#define SMM_PERFORMANCE_PROTOCOL_GUID \
  { 0xf866226a, 0xeaa5, 0x4f5a, { 0xa9, 0xa,  0x6c, 0xfb, 0xa5, 0x7c, 0x58, 0x8e } }

#define SMM_PERFORMANCE_EX_PROTOCOL_GUID \
  { 0x931fc048, 0xc71d, 0x4455, { 0x89, 0x30, 0x47, 0x6,  0x30, 0xe3, 0xe,  0xe5 } }

//
// SMM_PERFORMANCE_STRING_SIZE.
//
#define SMM_PERFORMANCE_STRING_SIZE    32
#define SMM_PERFORMANCE_STRING_LENGTH  (SMM_PERFORMANCE_STRING_SIZE - 1)

//
// The default guage entries number for SMM phase.
//
#define INIT_SMM_GAUGE_DATA_ENTRIES  200

typedef struct {
  UINTN               Function;
  EFI_STATUS          ReturnStatus;
  UINTN               NumberOfEntries;
  UINTN               LogEntryKey;
  GAUGE_DATA_ENTRY    *GaugeData;
} SMM_PERF_COMMUNICATE;

typedef struct {
  UINTN                  Function;
  EFI_STATUS             ReturnStatus;
  UINTN                  NumberOfEntries;
  UINTN                  LogEntryKey;
  GAUGE_DATA_ENTRY_EX    *GaugeDataEx;
} SMM_PERF_COMMUNICATE_EX;

#define SMM_PERF_FUNCTION_GET_GAUGE_ENTRY_NUMBER  1
#define SMM_PERF_FUNCTION_GET_GAUGE_DATA          2

/**
  Adds a record at the end of the performance measurement log
  that records the start time of a performance measurement.

  The added record contains the Handle, Token, and Module.
  The end time of the new record is not recorded, so it is set to zero.
  If TimeStamp is not zero, then TimeStamp is used to fill in the start time in the record.
  If TimeStamp is zero, the start time in the record is filled in with the value
  read from the current time stamp.

  @param  Handle                  The pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   The pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  The pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               The 64-bit time stamp.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_OUT_OF_RESOURCES    There are not enough resources to record the measurement.

**/
typedef
EFI_STATUS
(EFIAPI *PERFORMANCE_START_GAUGE)(
  IN CONST VOID   *Handle   OPTIONAL,
  IN CONST CHAR8  *Token    OPTIONAL,
  IN CONST CHAR8  *Module   OPTIONAL,
  IN UINT64       TimeStamp
  );

/**
  Searches the performance measurement log from the beginning of the log
  for the first matching record that contains a zero end time and fills in a valid end time.

  Searches the performance measurement log from the beginning of the log
  for the first record that matches Handle, Token, and Module, and has an end time value of zero.
  If the record can not be found then return EFI_NOT_FOUND.
  If the record is found and TimeStamp is not zero,
  then the end time in the record is filled in with the value specified by TimeStamp.
  If the record is found and TimeStamp is zero, then the end time in the matching record
  is filled in with the current time stamp value.

  @param  Handle                  The pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   The pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  The pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               The 64-bit time stamp.

  @retval EFI_SUCCESS             The end of  the measurement was recorded.
  @retval EFI_NOT_FOUND           The specified measurement record could not be found.

**/
typedef
EFI_STATUS
(EFIAPI *PERFORMANCE_END_GAUGE)(
  IN CONST VOID   *Handle   OPTIONAL,
  IN CONST CHAR8  *Token    OPTIONAL,
  IN CONST CHAR8  *Module   OPTIONAL,
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
  @param  GaugeDataEntry          Out parameter for the indirect pointer to the gauge data entry specified by LogEntryKey.

  @retval EFI_SUCCESS             The GuageDataEntry is successfully found based on LogEntryKey.
  @retval EFI_NOT_FOUND           There is no entry after the measurement referred to by LogEntryKey.
  @retval EFI_INVALID_PARAMETER   The LogEntryKey is not a valid entry, or GaugeDataEntry is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *PERFORMANCE_GET_GAUGE)(
  IN  UINTN               LogEntryKey,
  OUT GAUGE_DATA_ENTRY    **GaugeDataEntry
  );

/**
  Adds a record at the end of the performance measurement log
  that records the start time of a performance measurement.

  The added record contains the Handle, Token, Module and Identifier.
  The end time of the new record is not recorded, so it is set to zero.
  If TimeStamp is not zero, then TimeStamp is used to fill in the start time in the record.
  If TimeStamp is zero, the start time in the record is filled in with the value
  read from the current time stamp.

  @param  Handle                  The pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   The pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  The pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               The 64-bit time stamp.
  @param  Identifier              32-bit identifier. If the value is 0, the created record
                                  is same as the one created by StartGauge of PERFORMANCE_PROTOCOL.

  @retval EFI_SUCCESS             The data was read correctly from the device.
  @retval EFI_OUT_OF_RESOURCES    There are not enough resources to record the measurement.

**/
typedef
EFI_STATUS
(EFIAPI *PERFORMANCE_START_GAUGE_EX)(
  IN CONST VOID   *Handle   OPTIONAL,
  IN CONST CHAR8  *Token    OPTIONAL,
  IN CONST CHAR8  *Module   OPTIONAL,
  IN UINT64       TimeStamp,
  IN UINT32       Identifier
  );

/**
  Searches the performance measurement log from the beginning of the log
  for the first matching record that contains a zero end time and fills in a valid end time.

  Searches the performance measurement log from the beginning of the log
  for the first record that matches Handle, Token, Module and Identifier, and has an end time value of zero.
  If the record can not be found then return EFI_NOT_FOUND.
  If the record is found and TimeStamp is not zero,
  then the end time in the record is filled in with the value specified by TimeStamp.
  If the record is found and TimeStamp is zero, then the end time in the matching record
  is filled in with the current time stamp value.

  @param  Handle                  The pointer to environment specific context used
                                  to identify the component being measured.
  @param  Token                   The pointer to a Null-terminated ASCII string
                                  that identifies the component being measured.
  @param  Module                  The pointer to a Null-terminated ASCII string
                                  that identifies the module being measured.
  @param  TimeStamp               The 64-bit time stamp.
  @param  Identifier              32-bit identifier. If the value is 0, the found record
                                  is same as the one found by EndGauge of PERFORMANCE_PROTOCOL.

  @retval EFI_SUCCESS             The end of  the measurement was recorded.
  @retval EFI_NOT_FOUND           The specified measurement record could not be found.

**/
typedef
EFI_STATUS
(EFIAPI *PERFORMANCE_END_GAUGE_EX)(
  IN CONST VOID   *Handle   OPTIONAL,
  IN CONST CHAR8  *Token    OPTIONAL,
  IN CONST CHAR8  *Module   OPTIONAL,
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
  @param  GaugeDataEntryEx        Out parameter for the indirect pointer to the extented gauge data entry specified by LogEntryKey.

  @retval EFI_SUCCESS             The GuageDataEntryEx is successfully found based on LogEntryKey.
  @retval EFI_NOT_FOUND           There is no entry after the measurement referred to by LogEntryKey.
  @retval EFI_INVALID_PARAMETER   The LogEntryKey is not a valid entry, or GaugeDataEntryEx is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *PERFORMANCE_GET_GAUGE_EX)(
  IN  UINTN                 LogEntryKey,
  OUT GAUGE_DATA_ENTRY_EX   **GaugeDataEntryEx
  );

struct _PERFORMANCE_PROTOCOL {
  PERFORMANCE_START_GAUGE    StartGauge;
  PERFORMANCE_END_GAUGE      EndGauge;
  PERFORMANCE_GET_GAUGE      GetGauge;
};

struct _PERFORMANCE_EX_PROTOCOL {
  PERFORMANCE_START_GAUGE_EX    StartGaugeEx;
  PERFORMANCE_END_GAUGE_EX      EndGaugeEx;
  PERFORMANCE_GET_GAUGE_EX      GetGaugeEx;
};

extern EFI_GUID  gPerformanceProtocolGuid;
extern EFI_GUID  gSmmPerformanceProtocolGuid;
extern EFI_GUID  gPerformanceExProtocolGuid;
extern EFI_GUID  gSmmPerformanceExProtocolGuid;

#endif
