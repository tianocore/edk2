/** @file
  Module to clarify system event log of smbios structure.

  Copyright (c) 2005-2011, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMBIOS_EVENT_LOG_INFO_H_
#define _SMBIOS_EVENT_LOG_INFO_H_

#define END_OF_LOG  0xFF

#pragma pack(1)

typedef struct {
  UINT8    Type;
  UINT8    Length;
  UINT8    Year;
  UINT8    Month;
  UINT8    Day;
  UINT8    Hour;
  UINT8    Minute;
  UINT8    Second;
  UINT8    LogVariableData[1];
} LOG_RECORD_FORMAT;

typedef struct {
  UINT8    OEMReserved[5];
  UINT8    Metw;           // Multiple Event Time Window
  UINT8    Meci;           // Multiple Event Count Increment
  UINT8    CMOSAddress;    // Pre-boot Event Log Reset - CMOS Address
  UINT8    CMOSBitIndex;   // Pre-boot Event Log Reset - CMOS Bit Index
  UINT8    StartingOffset; // CMOS Checksum - Starting Offset
  UINT8    ByteCount;      // CMOS Checksum - Byte Count
  UINT8    ChecksumOffset; // CMOS Checksum - Checksum Offset
  UINT8    Reserved[3];
  UINT8    HeaderRevision;
} LOG_HEADER_TYPE1_FORMAT;

#pragma pack()
//
// System Event Log (Type 15)
//

/**
  Function to display system event log access information.

  @param[in] Key    Additional information to print.
  @param[in] Option Whether to print the additional information.
**/
VOID
DisplaySELAccessMethod (
  IN CONST UINT8  Key,
  IN CONST UINT8  Option
  );

/**
  Function to display system event log status information.

  @param[in] Key    Additional information to print.
  @param[in] Option Whether to print the additional information.
**/
VOID
DisplaySELLogStatus (
  UINT8  Key,
  UINT8  Option
  );

/**
  Function to display system event log header format information.

  @param[in] Key    Additional information to print.
  @param[in] Option Whether to print the additional information.
**/
VOID
DisplaySysEventLogHeaderFormat (
  UINT8  Key,
  UINT8  Option
  );

/**
  Function to display system event log header information.

  @param[in] LogHeaderFormat  Format identifier.
  @param[in] LogHeader        Format informcation.
**/
VOID
DisplaySysEventLogHeader (
  UINT8  LogHeaderFormat,
  UINT8  *LogHeader
  );

/**
  Function to display system event log data.

  @param[in] LogData        The data information.
  @param[in] LogAreaLength  Length of the data.
**/
VOID
DisplaySysEventLogData (
  UINT8   *LogData,
  UINT16  LogAreaLength
  );

#endif
