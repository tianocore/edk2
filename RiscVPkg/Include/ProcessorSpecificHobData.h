/** @file
  Definition of Processor Specific Data HOB.

  Copyright (c) 2019-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RISC_V_PROCESSOR_SPECIFIC_HOB_DATA_H_
#define RISC_V_PROCESSOR_SPECIFIC_HOB_DATA_H_

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <SmbiosProcessorSpecificData.h>

#define TO_BE_FILLED                              0
#define TO_BE_FILLED_BY_VENDOR                    0
#define TO_BE_FILLED_BY_RISC_V_SMBIOS_DXE_DRIVER  0
#define TO_BE_FILLED_BY_CODE                      0

#pragma pack(1)

///
/// RISC-V processor specific data HOB
///
typedef struct {
  EFI_GUID                                 ParentProcessorGuid;
  UINTN                                    ParentProcessorUid;
  EFI_GUID                                 CoreGuid;
  VOID                                     *Context; // The additional information of this core which
                                                     // built in PEI phase and carried to DXE phase.
                                                     // The content is processor or platform specific.
  SMBIOS_RISC_V_PROCESSOR_SPECIFIC_DATA    ProcessorSpecificData;
} RISC_V_PROCESSOR_SPECIFIC_HOB_DATA;

///
/// RISC-V SMBIOS type 4 (Processor) GUID data HOB
///
typedef struct {
  EFI_GUID              ProcessorGuid;
  UINTN                 ProcessorUid;
  SMBIOS_TABLE_TYPE4    SmbiosType4Processor;
  UINT16                EndingZero;
} RISC_V_PROCESSOR_TYPE4_HOB_DATA;

#define RISC_V_CACHE_INFO_NOT_PROVIDED  0xFFFF

#define RISC_V_CACHE_CONFIGURATION_CACHE_LEVEL_MASK  0x7
#define RISC_V_CACHE_CONFIGURATION_CACHE_LEVEL_1     0x01
#define RISC_V_CACHE_CONFIGURATION_CACHE_LEVEL_2     0x02
#define RISC_V_CACHE_CONFIGURATION_CACHE_LEVEL_3     0x03

#define RISC_V_CACHE_CONFIGURATION_SOCKET_BIT_POSITION  3
#define RISC_V_CACHE_CONFIGURATION_SOCKET_MASK          (0x1 << RISC_V_CACHE_CONFIGURATION_SOCKET_BIT_POSITION)
#define RISC_V_CACHE_CONFIGURATION_SOCKET_SOCKETED      (0x1 << RISC_V_CACHE_CONFIGURATION_SOCKET_BIT_POSITION)

#define RISC_V_CACHE_CONFIGURATION_LOCATION_BIT_POSITION  5
#define RISC_V_CACHE_CONFIGURATION_LOCATION_MASK          (0x3 << RISC_V_CACHE_CONFIGURATION_LOCATION_BIT_POSITION)
#define RISC_V_CACHE_CONFIGURATION_LOCATION_INTERNAL      (0x0 << RISC_V_CACHE_CONFIGURATION_LOCATION_BIT_POSITION)
#define RISC_V_CACHE_CONFIGURATION_LOCATION_EXTERNAL      (0x1 << RISC_V_CACHE_CONFIGURATION_LOCATION_BIT_POSITION)
#define RISC_V_CACHE_CONFIGURATION_LOCATION_RESERVED      (0x2 << RISC_V_CACHE_CONFIGURATION_LOCATION_BIT_POSITION)
#define RISC_V_CACHE_CONFIGURATION_LOCATION_UNKNOWN       (0x3 << RISC_V_CACHE_CONFIGURATION_LOCATION_BIT_POSITION)

#define RISC_V_CACHE_CONFIGURATION_ENABLE_BIT_POSITION  7
#define RISC_V_CACHE_CONFIGURATION_ENABLE_MASK          (0x1 << RISC_V_CACHE_CONFIGURATION_ENABLE_BIT_POSITION)
#define RISC_V_CACHE_CONFIGURATION_ENABLED              (0x1 << RISC_V_CACHE_CONFIGURATION_ENABLE_BIT_POSITION)

#define RISC_V_CACHE_CONFIGURATION_MODE_BIT_POSITION  8
#define RISC_V_CACHE_CONFIGURATION_MODE_MASK          (0x3 << RISC_V_CACHE_CONFIGURATION_MODE_BIT_POSITION)
#define RISC_V_CACHE_CONFIGURATION_MODE_WT            (0x0 << RISC_V_CACHE_CONFIGURATION_MODE_BIT_POSITION)
#define RISC_V_CACHE_CONFIGURATION_MODE_WB            (0x1 << RISC_V_CACHE_CONFIGURATION_MODE_BIT_POSITION)
#define RISC_V_CACHE_CONFIGURATION_MODE_VARIES        (0x2 << RISC_V_CACHE_CONFIGURATION_MODE_BIT_POSITION)
#define RISC_V_CACHE_CONFIGURATION_MODE_UNKNOWN       (0x3 << RISC_V_CACHE_CONFIGURATION_MODE_BIT_POSITION)
///
/// RISC-V SMBIOS type 7 (Cache) GUID data HOB
///
typedef struct {
  EFI_GUID              ProcessorGuid;
  UINTN                 ProcessorUid;
  SMBIOS_TABLE_TYPE7    SmbiosType7Cache;
  UINT16                EndingZero;
} RISC_V_PROCESSOR_TYPE7_HOB_DATA;

///
/// RISC-V SMBIOS type 7 (Cache) GUID data HOB
///
typedef struct {
  RISC_V_PROCESSOR_TYPE4_HOB_DATA    *Processor;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA    *L1Cache;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA    *L2Cache;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA    *L3Cache;
} RISC_V_PROCESSOR_SMBIOS_HOB_DATA;

#pragma pack()

#endif
