/** @file
  This file defines edk2 extended firmware performance records.
  These records will be added into ACPI FPDT Firmware Basic Boot Performance Table.

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EXTENDED_FIRMWARE_PERFORMANCE_H__
#define __EXTENDED_FIRMWARE_PERFORMANCE_H__

#include <IndustryStandard/Acpi.h>

//
// Known performance tokens
//
#define SEC_TOK                         "SEC"             ///< SEC Phase
#define DXE_TOK                         "DXE"             ///< DXE Phase
#define PEI_TOK                         "PEI"             ///< PEI Phase
#define BDS_TOK                         "BDS"             ///< BDS Phase
#define DRIVERBINDING_START_TOK         "DB:Start:"       ///< Driver Binding Start() function call
#define DRIVERBINDING_SUPPORT_TOK       "DB:Support:"     ///< Driver Binding Support() function call
#define DRIVERBINDING_STOP_TOK          "DB:Stop:"        ///< Driver Binding Stop() function call
#define LOAD_IMAGE_TOK                  "LoadImage:"      ///< Load a dispatched module
#define START_IMAGE_TOK                 "StartImage:"     ///< Dispatched Modules Entry Point execution
#define PEIM_TOK                        "PEIM"            ///< PEIM Modules Entry Point execution

//
// Misc defines
//
#define FPDT_RECORD_REVISION_1      (0x01)

//
// Length field in EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER is a UINT8, thus:
//
#define FPDT_MAX_PERF_RECORD_SIZE   (MAX_UINT8)

//
// FPDT Record Types
//
#define FPDT_GUID_EVENT_TYPE               0x1010
#define FPDT_DYNAMIC_STRING_EVENT_TYPE     0x1011
#define FPDT_DUAL_GUID_STRING_EVENT_TYPE   0x1012
#define FPDT_GUID_QWORD_EVENT_TYPE         0x1013
#define FPDT_GUID_QWORD_STRING_EVENT_TYPE  0x1014

//
// EDKII extended Fpdt record structures
//
#define FPDT_STRING_EVENT_RECORD_NAME_LENGTH 24

#pragma pack(1)
//
// FPDT Boot Performance Guid Event Record Structure
//
typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  ///
  /// ProgressID < 0x10 are reserved for core performance entries.
  /// Start measurement point shall have lowered one nibble set to zero and
  /// corresponding end points shall have lowered one nibble set to non-zero value;
  /// keeping other nibbles same as start point.
  ///
  UINT16                                      ProgressID;
  ///
  /// APIC ID for the processor in the system used as a timestamp clock source.
  /// If only one timestamp clock source is used, this field is Reserved and populated as 0.
  ///
  UINT32                                      ApicID;
  ///
  /// 64-bit value (nanosecond) describing elapsed time since the most recent deassertion of processor reset.
  ///
  UINT64                                      Timestamp;
  ///
  /// If ProgressID < 0x10, GUID of the referenced module; otherwise, GUID of the module logging the event.
  ///
  EFI_GUID                                    Guid;
} FPDT_GUID_EVENT_RECORD;

//
// FPDT Boot Performance Dynamic String Event Record Structure
//
typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  ///
  /// ProgressID < 0x10 are reserved for core performance entries.
  /// Start measurement point shall have lowered one nibble set to zero and
  /// corresponding end points shall have lowered one nibble set to non-zero value;
  /// keeping other nibbles same as start point.
  ///
  UINT16                                      ProgressID;
  ///
  /// APIC ID for the processor in the system used as a timestamp clock source.
  /// If only one timestamp clock source is used, this field is Reserved and populated as 0.
  ///
  UINT32                                      ApicID;
  ///
  /// 64-bit value (nanosecond) describing elapsed time since the most recent deassertion of processor reset.
  ///
  UINT64                                      Timestamp;
  ///
  /// If ProgressID < 0x10, GUID of the referenced module; otherwise, GUID of the module logging the event.
  ///
  EFI_GUID                                    Guid;
  ///
  /// ASCII string describing the module. Padding supplied at the end if necessary with null characters (0x00).
  /// It may be module name, function name, or token name.
  ///
  CHAR8                                       String[0];
} FPDT_DYNAMIC_STRING_EVENT_RECORD;

//
// FPDT Boot Performance Dual GUID String Event Record Structure
//
typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  ///
  /// ProgressID < 0x10 are reserved for core performance entries.
  /// Start measurement point shall have lowered one nibble set to zero and
  /// corresponding end points shall have lowered one nibble set to non-zero value;
  /// keeping other nibbles same as start point.
  ///
  UINT16                                      ProgressID;
  ///
  /// APIC ID for the processor in the system used as a timestamp clock source.
  /// If only one timestamp clock source is used, this field is Reserved and populated as 0.
  ///
  UINT32                                      ApicID;
  ///
  /// 64-bit value (nanosecond) describing elapsed time since the most recent deassertion of processor reset.
  ///
  UINT64                                      Timestamp;
  ///
  /// GUID of the module logging the event.
  ///
  EFI_GUID                                    Guid1;
  ///
  /// Event or Ppi or Protocol GUID for Callback.
  ///
  EFI_GUID                                    Guid2;
  ///
  /// ASCII string describing the module. Padding supplied at the end if necessary with null characters (0x00).
  /// It is the function name.
  ///
  CHAR8                                       String[0];
} FPDT_DUAL_GUID_STRING_EVENT_RECORD;

//
// FPDT Boot Performance GUID Qword Event Record Structure
//
typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  ///
  /// ProgressID < 0x10 are reserved for core performance entries.
  /// Start measurement point shall have lowered one nibble set to zero and
  /// corresponding end points shall have lowered one nibble set to non-zero value;
  /// keeping other nibbles same as start point.
  ///
  UINT16                                      ProgressID;
  ///
  /// APIC ID for the processor in the system used as a timestamp clock source.
  /// If only one timestamp clock source is used, this field is Reserved and populated as 0.
  ///
  UINT32                                      ApicID;
  ///
  /// 64-bit value (nanosecond) describing elapsed time since the most recent deassertion of processor reset.
  ///
  UINT64                                      Timestamp;
  ///
  /// GUID of the module logging the event
  ///
  EFI_GUID                                    Guid;
  ///
  /// Qword of misc data, meaning depends on the ProgressId
  ///
  UINT64                                      Qword;
} FPDT_GUID_QWORD_EVENT_RECORD;

//
// FPDT Boot Performance GUID Qword String Event Record Structure
//
typedef struct {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER Header;
  ///
  /// ProgressID < 0x10 are reserved for core performance entries.
  /// Start measurement point shall have lowered one nibble set to zero and
  /// corresponding end points shall have lowered one nibble set to non-zero value;
  /// keeping other nibbles same as start point.
  ///
  UINT16                                      ProgressID;
  ///
  /// APIC ID for the processor in the system used as a timestamp clock source.
  /// If only one timestamp clock source is used, this field is Reserved and populated as 0.
  ///
  UINT32                                      ApicID;
  ///
  /// 64-bit value (nanosecond) describing elapsed time since the most recent deassertion of processor reset.
  ///
  UINT64                                      Timestamp;
  ///
  /// GUID of the module logging the event
  ///
  EFI_GUID                                    Guid;
  ///
  /// Qword of misc data, meaning depends on the ProgressId
  ///
  UINT64                                      Qword;
  ///
  /// ASCII string describing the module. Padding supplied at the end if necessary with null characters (0x00).
  ///
  CHAR8                                       String[0];
} FPDT_GUID_QWORD_STRING_EVENT_RECORD;

#pragma pack()

//
// Union of all FPDT records
//
typedef union {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER  RecordHeader;
  FPDT_GUID_EVENT_RECORD                       GuidEvent;
  FPDT_DYNAMIC_STRING_EVENT_RECORD             DynamicStringEvent;
  FPDT_DUAL_GUID_STRING_EVENT_RECORD           DualGuidStringEvent;
  FPDT_GUID_QWORD_EVENT_RECORD                 GuidQwordEvent;
  FPDT_GUID_QWORD_STRING_EVENT_RECORD          GuidQwordStringEvent;
} FPDT_RECORD;

//
// Union of all pointers to FPDT records
//
typedef union {
  EFI_ACPI_5_0_FPDT_PERFORMANCE_RECORD_HEADER  *RecordHeader;
  FPDT_GUID_EVENT_RECORD                       *GuidEvent;
  FPDT_DYNAMIC_STRING_EVENT_RECORD             *DynamicStringEvent;
  FPDT_DUAL_GUID_STRING_EVENT_RECORD           *DualGuidStringEvent;
  FPDT_GUID_QWORD_EVENT_RECORD                 *GuidQwordEvent;
  FPDT_GUID_QWORD_STRING_EVENT_RECORD          *GuidQwordStringEvent;
} FPDT_RECORD_PTR;

///
/// Hob:
///   GUID - gEdkiiFpdtExtendedFirmwarePerformanceGuid;
///   Data - FPDT_PEI_EXT_PERF_HEADER + one or more FPDT records
///
typedef struct {
  UINT32                SizeOfAllEntries;
  UINT32                LoadImageCount;
  UINT32                HobIsFull;
} FPDT_PEI_EXT_PERF_HEADER;

extern EFI_GUID gEdkiiFpdtExtendedFirmwarePerformanceGuid;

#endif
