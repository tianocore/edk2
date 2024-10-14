/** @file
  CXL 3.0 Register definitions

  This file contains the register definitions based on the Compute Express Link
  (CXL) Specification Revision 3.0.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CXL30_H_
#define CXL30_H_

#include <IndustryStandard/Cxl20.h>

//
// CXL Cache Memory Capability IDs
// Compute Express Link Specification Revision 3.0 - Chapter 8.2.4 Table 8-22
//
#define CXL_CACHE_MEM_CAPABILITY_ID_TIMEOUT_AND_ISOLATION  0x0009
#define CXL_CACHE_MEM_CAPABILITY_ID_EXTENDED               0x000A
#define CXL_CACHE_MEM_CAPABILITY_ID_BI_ROUTE_TABLE         0x000B
#define CXL_CACHE_MEM_CAPABILITY_ID_BI_DECODER             0x000C
#define CXL_CACHE_MEM_CAPABILITY_ID_CACHE_ID_ROUTE_TABLE   0x000D
#define CXL_CACHE_MEM_CAPABILITY_ID_CACHE_ID_DECODER       0x000E
#define CXL_CACHE_MEM_CAPABILITY_ID_EXTENDED_HDM_DECODER   0x000F

//
// CXL_Capability_Version
// Compute Express ink Specification Revision 3.0 - Chapter 8.2.4.5
//
#define CXL_HDM_DECODER_VERSION_30  0x3

//
// CXL CXL HDM Decoder n Control
// Compute Express Link Specification Revision 3.0 - 8.2.4.19.7
//
//
// Bit4..7: Interleave Ways (IW)
//
#define CXL_HDM_16_WAY_INTERLEAVING  0x4
#define CXL_HDM_3_WAY_INTERLEAVING   0x8
#define CXL_HDM_6_WAY_INTERLEAVING   0x9
#define CXL_HDM_12_WAY_INTERLEAVING  0xA

//
// CXL RAS Capability Structure Uncorrectable Error Status IDE error bits
// Compute Express Link Specification Revision 3.0 - Chapter 8.2.4.16.1
//
#define CXL_IDE_TX_ERROR  BIT15
#define CXL_IDE_RX_ERROR  BIT16

//
// Ensure proper structure formats
//
#pragma pack(1)

//
// CXL.cachemem Extended Register Capability
// Compute Express Link Specification Revision 3.0  - Chapter 8.2.4.24
//
typedef union {
  struct {
    UINT32    ExtendedRangesBitmap : 16;      // Bit 0..15
    UINT32    Reserved             : 16;      // Bit 16..31
  } Bits;
  UINT32    Uint32;
} CXL_CM_EXTENTED_REGISTER_CAPABILITY;

#define CXL_CM_EXTENTED_RANGES_BITMAP  (BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7 | BIT8 | BIT9 | BIT10 | BIT11 | BIT12 | BIT13 | BIT15)

//
// CXL BI Route Table Capability
// Compute Express Link Specification Revision 3.0  - Chapter 8.2.4.25
//
typedef union {
  struct {
    UINT32    ExplicitBiRtCommitRequired : 1;                  // bit 0
    UINT32    Reserved                   : 31;                 // bit 1..31
  } Bits;
  UINT32    Uint32;
} CXL_BI_RT_CAPABILITY;

typedef union {
  struct {
    UINT32    BiRtCommit : 1;                                   // bit 0
    UINT32    Reserved   : 31;                                  // bit 1..31
  } Bits;
  UINT32    Uint32;
} CXL_BI_RT_CONTROL;

typedef union {
  struct {
    UINT32    BiRtCommitted          : 1;                      // bit 0
    UINT32    BiRtErrorNotCommitted  : 1;                      // bit 1
    UINT32    Reserved1              : 6;                      // bit 2..7
    UINT32    BiRtCommitTimeoutScale : 4;                      // bit 8..11
    UINT32    BiRtCommitTimeoutBase  : 4;                      // bit 12..15
    UINT32    Reserved2              : 16;                     // bit 16..31
  } Bits;
  UINT32    Uint32;
} CXL_BI_RT_STATUS;

typedef struct {
  CXL_BI_RT_CAPABILITY    BiRtCap;                              // offset 0x00
  CXL_BI_RT_CONTROL       BiRtControl;                          // offset 0x04
  CXL_BI_RT_STATUS        BiRtStatus;                           // offset 0x08
} CXL_BI_ROUTE_TABLE_CAPABILITY;

//
// CXL BI Decoder Capability
// Compute Express Link Specification Revision 3.0  - Chapter 8.2.4.26
//
typedef union {
  struct {
    UINT32    HdmDCapable                     : 1;             // bit 0
    UINT32    ExplicitBiDecoderCommitRequired : 1;             // bit 1
    UINT32    Reserved                        : 30;            // bit 2..31
  } Bits;
  UINT32    Uint32;
} CXL_BI_DECODER_CAP;

typedef union {
  struct {
    UINT32    BiForward       : 1;                             // bit 0
    UINT32    BiEnable        : 1;                             // bit 1
    UINT32    BiDecoderCommit : 1;                             // bit 2
    UINT32    Reserved        : 29;                            // bit 3..31
  } Bits;
  UINT32    Uint32;
} CXL_BI_DECODER_CONTROL;

typedef union {
  struct {
    UINT32    BiDecoderCommitted          : 1;                 // bit 0
    UINT32    BiDecoderErrorNotCommitted  : 1;                 // bit 1
    UINT32    Reserved1                   : 6;                 // bit 2..7
    UINT32    BiDecoderCommitTimeoutScale : 4;                 // bit 8..11
    UINT32    BiDecoderCommitTimeoutBase  : 4;                 // bit 12..15
    UINT32    Reserved2                   : 16;                // bit 16..31
  } Bits;
  UINT32    Uint32;
} CXL_BI_DECODER_STATUS;

typedef struct {
  CXL_BI_DECODER_CAP        BiDecoderCap;                       // offset 0x00
  CXL_BI_DECODER_CONTROL    BiDecoderControl;                   // offset 0x04
  CXL_BI_DECODER_STATUS     BiDecoderStatus;                    // offset 0x08
} CXL_BI_DECODER_CAPABILITY;

//
// CXL Cache ID Route Table Capability
// Compute Express Link Specification Revision 3.0  - Chapter 8.2.4.27
//
typedef union {
  struct {
    UINT32    CacheIdTargetCount              : 5;            // Bit 0..4
    UINT32    Reserved1                       : 3;            // Bit 5..7
    UINT32    HdmDType2DeviceMaxCount         : 4;            // Bit 8..11
    UINT32    Reserved2                       : 4;            // Bit 12..15
    UINT32    ExplicitCacheIdRtCommitRequired : 1;            // Bit 16
    UINT32    Reserved3                       : 15;           // Bit 17:31
  } Bits;
  UINT32    Uint32;
} CXL_CACHE_ID_RT_CAPABILITY;

typedef union {
  struct {
    UINT32    CacheIdRtCommit : 1;                   // Bit 0
    UINT32    Reserved        : 31;                  // Bit 1..31
  } Bits;
  UINT32    Uint32;
} CXL_CACHE_ID_RT_CONTROL;

typedef union {
  struct {
    UINT32    CacheIdRtCommitted          : 1;       // Bit 0
    UINT32    CacheIdRtErrNotCommitted    : 1;       // Bit 1
    UINT32    Reserved1                   : 6;       // Bit 2..7
    UINT32    CacheIdRtCommitTimeoutScale : 4;       // Bit 8..11
    UINT32    CacheIdRtCommitTimeoutBase  : 4;       // Bit 12..15
    UINT32    Reserved2                   : 16;      // Bit 16..31
  } Bits;
  UINT32    Uint32;
} CXL_CACHE_ID_RT_STATUS;

typedef union {
  struct {
    UINT16    Valid      : 1;                       // Bit 0
    UINT16    Reserved   : 7;                       // Bit 1..7
    UINT16    PortNumber : 8;                       // Bit 8..15
  } Bits;
  UINT16    Uint16;
} CXL_CACHE_ID_RT_TARGET;

typedef struct {
  CXL_CACHE_ID_RT_CAPABILITY    CacheIdRtCap;               // offset 0x00
  CXL_CACHE_ID_RT_CONTROL       CacheIdRtControl;           // offset 0x04
  CXL_CACHE_ID_RT_STATUS        CacheIdRtStatus;            // offset 0x08
  UINT32                        Reserved;                   // offset 0x0C
  CXL_CACHE_ID_RT_TARGET        CacheIdRtTarget[];          // offset 0x10
} CXL_CACHE_ID_ROUTE_TABLE_CAPABILITY;

//
// CXL Cache ID Decoder Capability
// Compute Express Link Specification Revision 3.0  - Chapter 8.2.4.28
//
typedef union {
  struct {
    UINT32    ExplicitCacheIdDecoderCommitRequired : 1;            // Bit 0
    UINT32    Reserved                             : 31;           // Bit 1..31
  } Bits;
  UINT32    Uint32;
} CXL_CACHE_ID_DECODER_CAP;

typedef union {
  struct {
    UINT32    ForwardCacheId         : 1;           // Bit 0
    UINT32    AssignCacheId          : 1;           // Bit 1
    UINT32    HdmDType2DevicePresent : 1;           // Bit 2
    UINT32    CacheIdDecoderCommit   : 1;           // Bit 3
    UINT32    Reserved1              : 4;           // Bit 4..7
    UINT32    HdmDType2DeviceCacheId : 4;           // Bit 8..11
    UINT32    Reserved2              : 4;           // Bit 12..15
    UINT32    LocalCacheId           : 4;           // Bit 16..19
    UINT32    Reserved3              : 4;           // Bit 20..23
    UINT32    TrustLevel             : 2;           // Bit 24..25
    UINT32    Reserved4              : 6;           // Bit 26..31
  } Bits;
  UINT32    Uint32;
} CXL_CACHE_ID_DECODER_CONTROL;

typedef union {
  struct {
    UINT32    CacheIdDecoderCommitted          : 1;           // Bit 0
    UINT32    CacheIdDecoderErrorNotCommitted  : 1;           // Bit 1
    UINT32    Reserved1                        : 6;           // Bit 2..7
    UINT32    CacheIdDecoderCommitTimeoutScale : 4;           // Bit 8..11
    UINT32    CacheIdDecoderCommitTimeoutBase  : 4;           // Bit 12..15
    UINT32    Reserved2                        : 16;          // Bit 16..31
  } Bits;
  UINT32    Uint32;
} CXL_CACHE_ID_DECODER_STATUS;

typedef struct {
  CXL_CACHE_ID_DECODER_CAP        CacheIdDecoderCap;           // offset 0x00
  CXL_CACHE_ID_DECODER_CONTROL    CacheIdDecoderControl;       // offset 0x04
  CXL_CACHE_ID_DECODER_STATUS     CacheIdDecoderStatus;        // offset 0x08
} CXL_CACHE_ID_DECODER_CAPABILITY;

//
// CXL Timeout and Isolation Capability Structure
// Compute Express Link Specification Revision 3.0  - Chapter 8.2.4.23
//
typedef union {
  struct {
    UINT32    CxlmemTransactionTimeoutRangesSupported   : 4; // Bits 3:0
    UINT32    CxlmemTransactionTimeoutSupported         : 1; // Bits 4
    UINT32    Reserved1                                 : 3; // Bits 7:5
    UINT32    CxlcacheTransactionTimeoutRangesSupported : 4; // Bits 11:8
    UINT32    CxlcacheTransactionTimeoutSupported       : 1; // Bits 12
    UINT32    Reserved2                                 : 3; // Bits 15:13
    UINT32    CxlmemIsolationSupported                  : 1; // Bits 16
    UINT32    CxlmemIsolationLinkdownSupported          : 1; // Bits 17
    UINT32    CxlcacheIsolationSupported                : 1; // Bits 18
    UINT32    CxlcacheIsolationLinkdownSupported        : 1; // Bits 19
    UINT32    Reserved3                                 : 5; // Bits 24:20
    UINT32    IsolationErrCorSignalingSupported         : 1; // Bits 25
    UINT32    IsolationInterruptSupported               : 1; // Bits 26
    UINT32    IsolationInterruptMessageNumber           : 5; // Bits 31:27
  } Bits;
  UINT32    Uint32;
} CXL_3_0_CXL_TIMEOUT_AND_ISOLATION_CAPABILITY;

typedef union {
  struct {
    UINT32    CxlmemTransactionTimeoutValue    : 4; // Bits 3:0
    UINT32    CxlmemTransactionTimeoutEnable   : 1; // Bits 4
    UINT32    Reserved1                        : 3; // Bits 7:5
    UINT32    CxlcacheTransactionTimeoutValue  : 4; // Bits 11:8
    UINT32    CxlcacheTransactionTimeoutEnable : 1; // Bits 12
    UINT32    Reserved2                        : 3; // Bits 15:13
    UINT32    CxlmemIsolationEnable            : 1; // Bits 16
    UINT32    CxlmemIsolationLinkdownEnable    : 1; // Bits 17
    UINT32    CxlcacheIsolationEnable          : 1; // Bits 18
    UINT32    CxlcacheIsolationLinkdownEnable  : 1; // Bits 19
    UINT32    Reserved3                        : 5; // Bits 24:20
    UINT32    IsolationErrCorSignalingEnable   : 1; // Bits 25
    UINT32    IsolationInterruptEnable         : 1; // Bits 26
    UINT32    Reserved4                        : 5; // Bits 31:27
  } Bits;
  UINT32    Uint32;
} CXL_3_0_CXL_TIMEOUT_AND_ISOLATION_CONTROL;

typedef union {
  struct {
    UINT32    CxlmemTransactionTimeout        : 1;  // Bits 0
    UINT32    Reserved1                       : 3;  // Bits 3:1
    UINT32    CxlcacheTransactionTimeout      : 1;  // Bits 4
    UINT32    Reserved2                       : 3;  // Bits 7:5
    UINT32    CxlmemIsolationStatus           : 1;  // Bits 8
    UINT32    CxlmemIsolationLinkdownStatus   : 1;  // Bits 9
    UINT32    Reserved3                       : 2;  // Bits 11:10
    UINT32    CxlcacheIsolationStatus         : 1;  // Bits 12
    UINT32    CxlcacheIsolationLinkdownStatus : 1;  // Bits 13
    UINT32    CxlRpBusy                       : 1;  // Bits 14
    UINT32    Reserved4                       : 17; // Bits 31:15
  } Bits;
  UINT32    Uint32;
} CXL_3_0_CXL_TIMEOUT_AND_ISOLATION_STATUS;

typedef struct {
  CXL_3_0_CXL_TIMEOUT_AND_ISOLATION_CAPABILITY    TimeoutAndIsolationCap;
  UINT32                                          Reserved;
  CXL_3_0_CXL_TIMEOUT_AND_ISOLATION_CONTROL       TimeoutAndIsolationControl;
  CXL_3_0_CXL_TIMEOUT_AND_ISOLATION_STATUS        TimeoutAndIsolationStatus;
} CXL_3_0_CXL_TIMEOUT_AND_ISOLATION_CAPABILITY_STRUCTURE;

///
/// CXL Event Record structures
/// Compute Express Link Specification Revision 3.0  - Chapter 8.2.9.2
///@{

//
// Common Event Record
//
typedef struct {
  EFI_GUID    EventRecordIdentifier;
  UINT8       EventRecordLength;
  UINT8       EventRecordFlags[3];
  UINT16      EventRecordHandle;
  UINT16      RelatedEventRecordHandle;
  UINT64      EventRecordTimestamp;
  UINT8       MaintenanceOperationClass;
  UINT8       Reserved[15];
} CXL_EVENT_RECORD_COMMON;

//
// General Media Event Record
//
typedef struct {
  UINT64    PhysicalAddress;
  UINT8     MemoryEventDescriptor;
  UINT8     MemoryEventType;
  UINT8     TransactionType;
  UINT16    ValidityFlags;
  UINT8     Channel;
  UINT8     Rank;
  UINT8     Device[3];
  UINT8     ComponentIdentifier[16];
  UINT8     Reserved[46];
} CXL_GENERAL_MEDIA_EVENT_RECORD_SPECIFIC;

typedef struct {
  CXL_EVENT_RECORD_COMMON                    Common;
  CXL_GENERAL_MEDIA_EVENT_RECORD_SPECIFIC    Specific;
} CXL_GENERAL_MEDIA_EVENT_RECORD;

//
// DRAM Event Record
//
typedef struct {
  UINT64    PhysicalAddress;
  UINT8     MemoryEventDescriptor;
  UINT8     MemoryEventType;
  UINT8     TransactionType;
  UINT16    ValidityFlags;
  UINT8     Channel;
  UINT8     Rank;
  UINT8     NibbleMask[3];
  UINT8     BankGroup;
  UINT8     Bank;
  UINT8     Row[3];
  UINT16    Column;
  UINT8     CorrectionMask[32];
  UINT8     Reserved[23];
} CXL_DRAM_EVENT_RECORD_SPECIFIC;

typedef struct {
  CXL_EVENT_RECORD_COMMON           Common;
  CXL_DRAM_EVENT_RECORD_SPECIFIC    Specific;
} CXL_DRAM_EVENT_RECORD;

//
// Device Health info used in memory module event record
//
typedef struct {
  UINT8     HealthStatus;
  UINT8     MediaStatus;
  UINT8     AdditionalStatus;
  UINT8     LifeUsed;
  UINT16    DeviceTemperature;
  UINT32    DirtyShutdownCount;
  UINT32    CorrectedVolatileErrorCount;
  UINT32    CorrectedPersistentErrorCount;
} CXL_DEVICE_HEALTH_INFORMATION;

//
// Memory Module Event Record
//
typedef struct {
  UINT8                            DeviceEventType;
  CXL_DEVICE_HEALTH_INFORMATION    DeviceHealthInformation;
  UINT8                            Reserved[61];
} CXL_MEMORY_MODULE_EVENT_RECORD_SPECIFIC;

typedef struct {
  CXL_EVENT_RECORD_COMMON                    Common;
  CXL_MEMORY_MODULE_EVENT_RECORD_SPECIFIC    Specific;
} CXL_MEMORY_MODULE_EVENT_RECORD;

//
// Generic CXL event record of 128 bytes
// Common Event Record - 48 bytes
// Remaining part of event record - 80 bytes
//
typedef struct {
  UINT8    Data[80];
} CXL_EVENT_RECORD_SPECIFIC;

typedef struct {
  CXL_EVENT_RECORD_COMMON      Common;
  CXL_EVENT_RECORD_SPECIFIC    Specific;
} CXL_EVENT_RECORD;

typedef struct {
  CXL_EVENT_RECORD_COMMON    Common;
  union {
    CXL_EVENT_RECORD_SPECIFIC                  EventRecord;
    CXL_GENERAL_MEDIA_EVENT_RECORD_SPECIFIC    GeneralMediaEventRecord;
    CXL_DRAM_EVENT_RECORD_SPECIFIC             DramEventRecord;
    CXL_MEMORY_MODULE_EVENT_RECORD_SPECIFIC    MemoryModuleEventRecord;
  } RecordSpecific;
} CXL_COMMON_EVENT_RECORD;

///@}

///
/// Media and Poison Management structures
/// Compute Express Link Specification Revision 3.0  - Chapter 8.2.9.8.4
///@{

//
// Get Poison List input payload structure
//
typedef struct {
  UINT64    PhysAddr;               // The starting DPA to retrieve the Poison List for
  UINT64    PhysAddrLen;            // The range of physical addresses to retrieve the Poison List for.  In units of 64 bytes.
} CXL_GET_POISON_LIST_INPUT_PAYLOAD;

//
// Get Poison List output payload structures
//
typedef union {
  struct {
    UINT8    MoreMediaErrorRecords : 1; // bit 0
    UINT8    PoisonListOverflow    : 1; // bit 1
    UINT8    ScanMediaInProgress   : 1; // bit 2
    UINT8    Rsvd                  : 5; // bit 3..7
  } Bits;
  UINT8    Data8;
} CXL_POISON_LIST_FLAGS;

typedef union {
  struct {
    union {
      struct {
        UINT8    ErrorSource : 3;
        UINT8    Rsvd        : 3;
        UINT8    Dpa0_1      : 2;
      } Bits;
      UINT8    Data8;
    } Byte0;
    UINT8    Dpa[0x7];        // The range of physical addresses to retrieve the Poison List for.  In units of 64 bytes.
  } Bits;
  UINT64    Data64;
} CXL_MEDIA_ERROR_ADDRESS;

typedef struct {
  CXL_MEDIA_ERROR_ADDRESS    MediaErrorAddr; // DPA of the memory error and error source
  UINT32                     MediaErrorLen;  // Number of adjacent DPAs in this media error record
  UINT32                     Rsvd;
} CXL_MEDIA_ERROR_RECORD;

typedef struct {
  CXL_POISON_LIST_FLAGS     PoisonListFlags;  // Flags that describe the returned list
  UINT8                     Rsvd0;
  UINT64                    OverflowTimestamp; // The time the device determined the poison list overflowed
  UINT16                    MedErrRecCnt;      // Number of records in the media error records list
  UINT8                     Rsvd1[0x14];
  CXL_MEDIA_ERROR_RECORD    MediaErrorRecords[0x10]; // The list of media error records
} CXL_GET_POISON_LIST_OUTPUT_PAYLOAD;

//
// Get Scan Media Capabilities input payload structure
//
typedef struct {
  UINT64    PhysAddr;                   // The starting DPA from where to retrieve Scan Media capabilities
  UINT64    PhysAddrLen;                // The range of physical addresses to retrieve the Scan Media capabilities for.  In units of 64 bytes.
} CXL_GET_SCAN_MEDIA_CAP_INPUT_PAYLOAD;

//
// Get Scan Media Capabilities output payload structure
//
typedef struct {
  UINT32    EstScanMediaTime;           // Device estimate to complete scan request in ms
} CXL_GET_SCAN_MEDIA_CAP_OUTPUT_PAYLOAD;

//
// Scan Media input payload structures
//
typedef union {
  struct {
    UINT8    NoEventLog : 1;      // bit 0
    UINT8    Rsvd       : 7;      // bit 1..7
  } Bits;
  UINT8    Data8;
} CXL_SCAN_MEDIA_FLAGS;

typedef struct {
  UINT64                  PhysAddr;       // The starting DPA to retrieve the Poison List for
  UINT64                  PhysAddrLen;    // The range of physical addresses to retrieve the Poison List for.  In units of 64 bytes.
  CXL_SCAN_MEDIA_FLAGS    ScanMediaFlags; // Flags that describe the scan media input payload
} CXL_SCAN_MEDIA_INPUT_PAYLOAD;

//
// Get Scan Media results output payload structures
//
typedef union {
  struct {
    UINT8    MoreMediaErrorRecords  : 1; // bit 0
    UINT8    ScanStoppedPrematurely : 1; // bit 1
    UINT8    Rsvd                   : 6; // bit 2..7
  } Bits;
  UINT8    Data8;
} CXL_GET_SCAN_MEDIA_FLAGS;

typedef struct {
  UINT64                      PhysAddr;          // The location where the host should restart the scan media operation
  UINT64                      PhysAddrLen;       // The remaining range to scan if the device could not complete the requested scan
  CXL_GET_SCAN_MEDIA_FLAGS    GetScanMediaFlags; // Flags that describe the scan media results
  UINT8                       Rsvd0;
  UINT16                      MedErrRecCnt;     // Number of records in the media error records list
  UINT8                       Rsvd1[0xC];
  CXL_MEDIA_ERROR_RECORD      MediaErrorRecords[0x10]; // The list of media error records
} CXL_GET_SCAN_MEDIA_RESULTS_OUTPUT_PAYLOAD;

///@}

//
// Get Event Interrupt Policy payload
// Compute Express Link Specification Revision 3.0  - Chapter 8.2.9.2.4
//
typedef struct {
  UINT8    InformationalEventLogInterruptSettings;
  UINT8    WarningEventLogInterruptSettings;
  UINT8    FailureEventLogInterruptSettings;
  UINT8    FatalEventLogInterruptSettings;
  UINT8    DynamicCapacityEventLogInterruptSettings;
} CXL_3_0_EVENT_INTERRUPT_POLICY_PAYLOAD;

//
// Device Event Status Register
// Compute Express Link Specification Revision 3.0  - Chapter 8.2.8.3.1
//
typedef union {
  struct {
    UINT8    InformationalEventLog   : 1; // bit 0
    UINT8    WarningEventLog         : 1; // bit 1
    UINT8    FailureEventLog         : 1; // bit 2
    UINT8    FatalEventLog           : 1; // bit 3
    UINT8    DynamicCapacityEventLog : 1; // bit 4
    UINT8    Rsvd                    : 3; // bit 5..7
    UINT8    Reserved[7];                 // bit 8..63
  } Bits;
  struct {
    UINT32    Lo;
    UINT32    Hi;
  } Data64;
} CXL_DEVICE_EVENT_STATUS_REGISTER;

#pragma pack()

#endif
