/** @file
*
*  Copyright (c) 2021, NUVIA Inc. All rights reserved.
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
  CpuCacheL1 = 1,
  CpuCacheL2,
  CpuCacheL3,
  CpuCacheL4,
  CpuCacheL5,
  CpuCacheL6,
  CpuCacheL7,
  CpuCacheLevelMax
} OEM_MISC_CPU_CACHE_LEVEL;

typedef struct
{
  UINT8 Voltage;        ///< Processor voltage
  UINT16 CurrentSpeed;  ///< Current clock speed in MHz
  UINT16 MaxSpeed;      ///< Maximum clock speed in MHz
  UINT16 ExternalClock; ///< External clock speed in MHz
  UINT16 CoreCount;     ///< Number of cores available
  UINT16 CoresEnabled;  ///< Number of cores enabled
  UINT16 ThreadCount;   ///< Number of threads per processor
} OEM_MISC_PROCESSOR_DATA;

typedef enum
{
    ProductNameType01,
    SerialNumType01,
    UuidType01,
    SystemManufacturerType01,
    SkuNumberType01,
    FamilyType01,
    AssertTagType02,
    SerialNumberType02,
    BoardManufacturerType02,
    SkuNumberType02,
    ChassisLocationType02,
    AssetTagType03,
    SerialNumberType03,
    VersionType03,
    ChassisTypeType03,
    ManufacturerType03,
    SkuNumberType03,
    SmbiosHiiStringFieldMax
} OEM_MISC_SMBIOS_HII_STRING_FIELD;

/*
 * The following are functions that the each platform needs to
 * implement in its OemMiscLib library.
 */

/** Gets the CPU frequency of the specified processor.

  @param ProcessorIndex Index of the processor to get the frequency for.

  @return               CPU frequency in Hz
**/
UINTN
EFIAPI
OemGetCpuFreq (
  IN UINT8 ProcessorIndex
  );

/** Gets information about the specified processor and stores it in
    the structures provided.

  @param ProcessorIndex  Index of the processor to get the information for.
  @param ProcessorStatus Processor status.
  @param ProcessorCharacteristics Processor characteritics.
  @param MiscProcessorData        Miscellaneous processor information.

  @return  TRUE on success, FALSE on failure.
**/
BOOLEAN
EFIAPI
OemGetProcessorInformation (
  IN UINTN ProcessorIndex,
  IN OUT PROCESSOR_STATUS_DATA *ProcessorStatus,
  IN OUT PROCESSOR_CHARACTERISTIC_FLAGS *ProcessorCharacteristics,
  IN OUT OEM_MISC_PROCESSOR_DATA *MiscProcessorData
  );

/** Gets information about the cache at the specified cache level.

  @param ProcessorIndex The processor to get information for.
  @param CacheLevel     The cache level to get information for.
  @param DataCache  Whether the cache is a data cache.
  @param UnifiedCache Whether the cache is a unified cache.
  @param SmbiosCacheTable The SMBIOS Type7 cache information structure.

  @return TRUE on success, FALSE on failure.
**/
BOOLEAN
EFIAPI
OemGetCacheInformation (
  IN UINT8   ProcessorIndex,
  IN UINT8   CacheLevel,
  IN BOOLEAN DataCache,
  IN BOOLEAN UnifiedCache,
  IN OUT SMBIOS_TABLE_TYPE7 *SmbiosCacheTable
  );

/** Gets the maximum number of processors supported by the platform.

  @return The maximum number of processors.
**/
UINT8
EFIAPI
OemGetMaxProcessors (
  VOID
  );

/** Gets the type of chassis for the system.

  @retval The type of the chassis.
**/
MISC_CHASSIS_TYPE
EFIAPI
OemGetChassisType (
  VOID
  );

/** Returns whether the specified processor is present or not.

  @param ProcessIndex The processor index to check.

  @return TRUE is the processor is present, FALSE otherwise.
**/
BOOLEAN
EFIAPI
OemIsProcessorPresent (
  IN UINTN ProcessorIndex
  );

/** Updates the HII string for the specified field.

  @param HiiHandle     The HII handle.
  @param TokenToUpdate The string to update.
  @param Field         The field to get information about.
**/
VOID
EFIAPI
OemUpdateSmbiosInfo (
  IN EFI_HII_HANDLE    HiiHandle,
  IN EFI_STRING_ID     TokenToUpdate,
  IN OEM_MISC_SMBIOS_HII_STRING_FIELD Field
  );

#endif // OEM_MISC_LIB_H_
