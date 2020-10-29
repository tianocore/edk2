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


//
// Cache Info
//
typedef struct {
  UINT16                    InstalledSize;    //In KB
  CACHE_TYPE_DATA           SystemCacheType;
  CACHE_ASSOCIATIVITY_DATA  Associativity;
} CACHE_INFO;

//
// Cache Configuration
//
typedef union {
  struct {
    UINT16    Level           :3;
    UINT16    Socketed        :1;
    UINT16    Reserved1       :1;
    UINT16    Location        :2;
    UINT16    Enable          :1;
    UINT16    OperationalMode :2;
    UINT16    Reserved2       :6;
  } Bits;
  UINT16 Data;
} CACHE_CONFIGURATION;

//
// Processor Status
//
typedef union {
  struct {
    UINT8 CpuStatus       :3; // Indicates the status of the processor.
    UINT8 Reserved1       :3; // Reserved for future use. Should be set to zero.
    UINT8 SocketPopulated :1; // Indicates if the processor socket is populated or not.
    UINT8 Reserved2       :1; // Reserved for future use. Should be set to zero.
  } Bits;
  UINT8 Data;
} PROCESSOR_STATUS_DATA;

//
// Processor Characteristics
//
typedef union {
  struct {
    UINT16 Reserved                 :1;
    UINT16 Unknown                  :1;
    UINT16 Capable64Bit             :1;
    UINT16 MultiCore                :1;
    UINT16 HardwareThread           :1;
    UINT16 ExecuteProtection        :1;
    UINT16 EnhancedVirtualization   :1;
    UINT16 PowerPerformanceControl  :1;
    UINT16 Capable128Bit            :1;
    UINT16 Arm64SocId               :1;
    UINT16 Reserved2                :6;
  } Bits;
  UINT16 Data;
} PROCESSOR_CHARACTERISTICS_DATA;

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

BOOLEAN OemIsSocketPresent (UINTN Socket);


UINTN OemGetCpuFreq (UINT8 Socket);

BOOLEAN
OemGetProcessorInformation (
  IN UINTN ProcessorNumber,
  PROCESSOR_STATUS_DATA *ProcessorStatus,
  IN OUT PROCESSOR_CHARACTERISTICS_DATA *ProcessorCharacteristics,
  IN OUT MISC_PROCESSOR_DATA *MiscProcessorData
  );

BOOLEAN OemGetCacheInformation
  (
  IN UINT8 CacheLevel,
  IN OUT SMBIOS_TABLE_TYPE7 *SmbiosCacheTable
  );

UINT8 OemGetProcessorMaxSockets (
  VOID
  );

UINTN PlatformGetCpuFreq (UINT8 Socket);
UINTN PlatformGetCoreCount (VOID);

EFI_STATUS
EFIAPI
OemGetChassisType(UINT8 *ChassisType);

VOID
UpdateSmbiosInfo (
  IN EFI_HII_HANDLE mHiiHandle,
  IN EFI_STRING_ID TokenToUpdate,
  IN UINT8 Offset
  );

#endif // OEM_MISC_LIB_H
