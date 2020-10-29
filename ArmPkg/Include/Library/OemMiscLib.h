/** @file
*
*  Copyright (c) 2015, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2015, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/


#ifndef OEM_MISC_LIB_H
#define OEM_MISC_LIB_H

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>

#define CPU_CACHE_L1_Instruction        0
#define CPU_CACHE_L1_Data               1
#define CPU_CACHE_L2                    2
#define CPU_CACHE_L3                    3
#define MAX_CACHE_LEVEL                 4

typedef struct
{
  UINT8 Voltage;
  UINT16 CurrentSpeed;
  UINT16 MaxSpeed;
  UINT16 ExternalClock;
  UINT16 CoreCount;
  UINT16 CoresEnabled;
  UINT16 ThreadCount;
} MISC_PROCESSOR_DATA;


typedef enum {
    ProductNameType01,
    SerialNumType01,
    UuidType01,
    SystemManufacturerType01,
    AssertTagType02,
    SrNumType02,
    BoardManufacturerType02,
    AssetTagType03,
    SrNumType03,
    VersionType03,
    ChassisTypeType03 ,
    ManufacturerType03,
} GET_INFO_BMC_OFFSET;

UINTN OemGetCpuFreq (UINT8 Socket);

BOOLEAN
OemGetProcessorInformation (
  IN UINTN ProcessorNumber,
  IN OUT PROCESSOR_STATUS_DATA *ProcessorStatus,
  IN OUT PROCESSOR_CHARACTERISTIC_FLAGS *ProcessorCharacteristics,
  IN OUT MISC_PROCESSOR_DATA *MiscProcessorData
  );

BOOLEAN OemGetCacheInformation (
  IN UINT8 CacheLevel,
  IN OUT SMBIOS_TABLE_TYPE7 *SmbiosCacheTable
  );

UINT8 OemGetProcessorMaxSockets (VOID);

UINTN PlatformGetCpuFreq (IN UINT8 Socket);

UINTN PlatformGetCoreCount (VOID);

EFI_STATUS OemGetChassisType(OUT UINT8 *ChassisType);

BOOLEAN OemIsSocketPresent (UINTN Socket);

VOID
UpdateSmbiosInfo (
  IN EFI_HII_HANDLE mHiiHandle,
  IN EFI_STRING_ID TokenToUpdate,
  IN UINT8 Offset
  );

#endif // OEM_MISC_LIB_H
