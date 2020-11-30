/** @file
*
*  Copyright (c) 2015, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2015, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/


#ifndef OEM_MISC_LIB_H_
#define OEM_MISC_LIB_H_

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>

typedef enum
{
  CpuCacheL1 = 0,
  CpuCacheL2,
  CpuCacheL3,
  CpuCacheL4,
  CpuCacheL5,
  CpuCacheL6,
  CpuCacheL7,
  CpuCacheMax
} CPU_CACHE_LEVEL;

typedef struct
{
  UINT8 Voltage;        ///< Processor voltage
  UINT16 CurrentSpeed;  ///< Current clock speed in MHz
  UINT16 MaxSpeed;      ///< Maximum clock speed in MHz
  UINT16 ExternalClock; ///< External clock speed in MHz
  UINT16 CoreCount;     ///< Number of cores available
  UINT16 CoresEnabled;  ///< Number of cores enabled
  UINT16 ThreadCount;   ///< Number of threads per processor
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
    BmcOffsetMax
} GET_INFO_BMC_OFFSET;

/*
 * The following are functions that the each platform needs to
 * implement in its OemMiscLib library.
 */

UINTN OemGetCpuFreq (UINT8 ProcessorIndex);

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

EFI_STATUS OemGetChassisType (OUT UINT8 *ChassisType);

BOOLEAN OemIsSocketPresent (IN UINTN ProcessorIndex);

VOID
UpdateSmbiosInfo (
  IN EFI_HII_HANDLE mHiiHandle,
  IN EFI_STRING_ID TokenToUpdate,
  IN UINT8 Offset
  );

#endif // OEM_MISC_LIB_H_
