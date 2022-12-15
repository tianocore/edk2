/** @file
  ProcessorSubClass.c

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.
  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.
  Copyright (c) 2015, Linaro Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Protocol/Smbios.h>
#include <IndustryStandard/ArmCache.h>
#include <IndustryStandard/ArmStdSmc.h>
#include <IndustryStandard/SmBios.h>
#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OemMiscLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "SmbiosProcessor.h"

extern UINT8  ProcessorSubClassStrings[];

#define CACHE_SOCKETED_SHIFT        3
#define CACHE_LOCATION_SHIFT        5
#define CACHE_ENABLED_SHIFT         7
#define CACHE_OPERATION_MODE_SHIFT  8

typedef enum {
  CacheModeWriteThrough = 0,  ///< Cache is write-through
  CacheModeWriteBack,         ///< Cache is write-back
  CacheModeVariesWithAddress, ///< Cache mode varies by address
  CacheModeUnknown,           ///< Cache mode is unknown
  CacheModeMax
} CACHE_OPERATION_MODE;

typedef enum {
  CacheLocationInternal = 0, ///< Cache is internal to the processor
  CacheLocationExternal,     ///< Cache is external to the processor
  CacheLocationReserved,     ///< Reserved
  CacheLocationUnknown,      ///< Cache location is unknown
  CacheLocationMax
} CACHE_LOCATION;

EFI_HII_HANDLE  mHiiHandle;

EFI_SMBIOS_PROTOCOL  *mSmbios;

SMBIOS_TABLE_TYPE4  mSmbiosProcessorTableTemplate = {
  {                                        // Hdr
    EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION, // Type
    sizeof (SMBIOS_TABLE_TYPE4),           // Length
    0                                      // Handle
  },
  1,                               // Socket
  CentralProcessor,                // ProcessorType
  ProcessorFamilyIndicatorFamily2, // ProcessorFamily
  2,                               // ProcessorManufacture
  {                           // ProcessorId
    {                         // Signature
      0
    },
    {                         // FeatureFlags
      0
    }
  },
  3,                          // ProcessorVersion
  {                           // Voltage
    0
  },
  0,                          // ExternalClock
  0,                          // MaxSpeed
  0,                          // CurrentSpeed
  0,                          // Status
  ProcessorUpgradeUnknown,    // ProcessorUpgrade
  0xFFFF,                     // L1CacheHandle
  0xFFFF,                     // L2CacheHandle
  0xFFFF,                     // L3CacheHandle
  4,                          // SerialNumber
  5,                          // AssetTag
  6,                          // PartNumber
  0,                          // CoreCount
  0,                          // EnabledCoreCount
  0,                          // ThreadCount
  0,                          // ProcessorCharacteristics
  ProcessorFamilyARM,         // ProcessorFamily2
  0,                          // CoreCount2
  0,                          // EnabledCoreCount2
  0                           // ThreadCount2
};

/** Sets the HII variable `StringId` is `Pcd` isn't empty.

    @param Pcd       The FixedAtBuild PCD that contains the string to fetch.
    @param StringId  The string identifier to set.
**/
#define SET_HII_STRING_IF_PCD_NOT_EMPTY(Pcd, StringId) \
  do { \
    CHAR16 *Str; \
    Str = (CHAR16*)PcdGetPtr (Pcd); \
    if (StrLen (Str) > 0) { \
      HiiSetString (mHiiHandle, StringId, Str, NULL); \
    } \
  } while (0)

/** Fetches the specified processor's frequency in Hz.

  @param ProcessorNumber The processor number

  @return The clock frequency in MHz

**/
UINT16
GetCpuFrequency (
  IN  UINT8  ProcessorNumber
  )
{
  return (UINT16)(OemGetCpuFreq (ProcessorNumber) / 1000 / 1000);
}

/** Gets a description of the specified cache.

  @param[in] CacheLevel       Zero-based cache level (e.g. L1 cache is 0).
  @param[in] DataCache        Cache is a data cache.
  @param[in] UnifiedCache     Cache is a unified cache.
  @param[out] CacheSocketStr  The description of the specified cache

  @return The number of Unicode characters in CacheSocketStr not including the
          terminating NUL.
**/
UINTN
GetCacheSocketStr (
  IN  UINT8    CacheLevel,
  IN  BOOLEAN  DataCache,
  IN  BOOLEAN  UnifiedCache,
  OUT CHAR16   *CacheSocketStr
  )
{
  UINTN  CacheSocketStrLen;

  if ((CacheLevel == CpuCacheL1) && !DataCache && !UnifiedCache) {
    CacheSocketStrLen = UnicodeSPrint (
                          CacheSocketStr,
                          SMBIOS_STRING_MAX_LENGTH - 1,
                          L"L%x Instruction Cache",
                          CacheLevel
                          );
  } else if ((CacheLevel == CpuCacheL1) && DataCache) {
    CacheSocketStrLen = UnicodeSPrint (
                          CacheSocketStr,
                          SMBIOS_STRING_MAX_LENGTH - 1,
                          L"L%x Data Cache",
                          CacheLevel
                          );
  } else {
    CacheSocketStrLen = UnicodeSPrint (
                          CacheSocketStr,
                          SMBIOS_STRING_MAX_LENGTH - 1,
                          L"L%x Cache",
                          CacheLevel
                          );
  }

  return CacheSocketStrLen;
}

/** Fills in the Type 7 record with the cache architecture information
    read from the CPU registers.

  @param[in]  CacheLevel       Cache level (e.g. L1, L2).
  @param[in]  DataCache        Cache is a data cache.
  @param[in]  UnifiedCache     Cache is a unified cache.
  @param[out] Type7Record      The Type 7 record to fill in.

**/
VOID
ConfigureCacheArchitectureInformation (
  IN     UINT8               CacheLevel,
  IN     BOOLEAN             DataCache,
  IN     BOOLEAN             UnifiedCache,
  OUT    SMBIOS_TABLE_TYPE7  *Type7Record
  )
{
  UINT8   Associativity;
  UINT32  CacheSize32;
  UINT16  CacheSize16;
  UINT64  CacheSize64;

  if (!DataCache && !UnifiedCache) {
    Type7Record->SystemCacheType = CacheTypeInstruction;
  } else if (DataCache) {
    Type7Record->SystemCacheType = CacheTypeData;
  } else if (UnifiedCache) {
    Type7Record->SystemCacheType = CacheTypeUnified;
  } else {
    ASSERT (FALSE);
  }

  CacheSize64 = SmbiosProcessorGetCacheSize (
                  CacheLevel,
                  DataCache,
                  UnifiedCache
                  );

  Associativity = SmbiosProcessorGetCacheAssociativity (
                    CacheLevel,
                    DataCache,
                    UnifiedCache
                    );

  CacheSize64 /= 1024; // Minimum granularity is 1K

  // Encode the cache size into the format SMBIOS wants
  if (CacheSize64 < MAX_INT16) {
    CacheSize16 = CacheSize64;
    CacheSize32 = CacheSize16;
  } else if ((CacheSize64 / 64) < MAX_INT16) {
    CacheSize16 = (1 << 15) | (CacheSize64 / 64);
    CacheSize32 = (1 << 31) | (CacheSize64 / 64);
  } else {
    if ((CacheSize64 / 1024) <= 2047) {
      CacheSize32 = CacheSize64;
    } else {
      CacheSize32 = (1 << 31) | (CacheSize64 / 64);
    }

    CacheSize16 = -1;
  }

  Type7Record->MaximumCacheSize  = CacheSize16;
  Type7Record->InstalledSize     = CacheSize16;
  Type7Record->MaximumCacheSize2 = CacheSize32;
  Type7Record->InstalledSize2    = CacheSize32;

  switch (Associativity) {
    case 2:
      Type7Record->Associativity = CacheAssociativity2Way;
      break;
    case 4:
      Type7Record->Associativity = CacheAssociativity4Way;
      break;
    case 8:
      Type7Record->Associativity = CacheAssociativity8Way;
      break;
    case 12:
      Type7Record->Associativity = CacheAssociativity12Way;
      break;
    case 16:
      Type7Record->Associativity = CacheAssociativity16Way;
      break;
    case 20:
      Type7Record->Associativity = CacheAssociativity20Way;
      break;
    case 24:
      Type7Record->Associativity = CacheAssociativity24Way;
      break;
    case 32:
      Type7Record->Associativity = CacheAssociativity32Way;
      break;
    case 48:
      Type7Record->Associativity = CacheAssociativity48Way;
      break;
    case 64:
      Type7Record->Associativity = CacheAssociativity64Way;
      break;
    default:
      Type7Record->Associativity = CacheAssociativityOther;
      break;
  }

  Type7Record->CacheConfiguration = (CacheModeUnknown << CACHE_OPERATION_MODE_SHIFT) |
                                    (1 << CACHE_ENABLED_SHIFT) |
                                    (CacheLocationUnknown << CACHE_LOCATION_SHIFT) |
                                    (0 << CACHE_SOCKETED_SHIFT) |
                                    (CacheLevel - 1);
}

/** Allocates and initializes an SMBIOS_TABLE_TYPE7 structure.

  @param[in]  CacheLevel       The cache level (L1-L7).
  @param[in]  DataCache        Cache is a data cache.
  @param[in]  UnifiedCache     Cache is a unified cache.

  @return A pointer to the Type 7 structure. Returns NULL on failure.
**/
SMBIOS_TABLE_TYPE7 *
AllocateAndInitCacheInformation (
  IN UINT8    CacheLevel,
  IN BOOLEAN  DataCache,
  IN BOOLEAN  UnifiedCache
  )
{
  SMBIOS_TABLE_TYPE7  *Type7Record;
  EFI_STRING          CacheSocketStr;
  UINTN               CacheSocketStrLen;
  UINTN               StringBufferSize;
  CHAR8               *OptionalStrStart;
  UINTN               TableSize;

  // Allocate and fetch the cache description
  StringBufferSize = sizeof (CHAR16) * SMBIOS_STRING_MAX_LENGTH;
  CacheSocketStr   = AllocateZeroPool (StringBufferSize);
  if (CacheSocketStr == NULL) {
    return NULL;
  }

  CacheSocketStrLen = GetCacheSocketStr (
                        CacheLevel,
                        DataCache,
                        UnifiedCache,
                        CacheSocketStr
                        );

  TableSize   = sizeof (SMBIOS_TABLE_TYPE7) + CacheSocketStrLen + 1 + 1;
  Type7Record = AllocateZeroPool (TableSize);
  if (Type7Record == NULL) {
    FreePool (CacheSocketStr);
    return NULL;
  }

  Type7Record->Hdr.Type   = EFI_SMBIOS_TYPE_CACHE_INFORMATION;
  Type7Record->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE7);
  Type7Record->Hdr.Handle = SMBIOS_HANDLE_PI_RESERVED;

  Type7Record->SocketDesignation = 1;

  Type7Record->SupportedSRAMType.Unknown = 1;
  Type7Record->CurrentSRAMType.Unknown   = 1;
  Type7Record->CacheSpeed                = 0;
  Type7Record->ErrorCorrectionType       = CacheErrorUnknown;

  OptionalStrStart = (CHAR8 *)(Type7Record + 1);
  UnicodeStrToAsciiStrS (CacheSocketStr, OptionalStrStart, CacheSocketStrLen + 1);
  FreePool (CacheSocketStr);

  return Type7Record;
}

/**
  Add Type 7 SMBIOS Record for Cache Information.

  @param[in]    ProcessorIndex      Processor number of specified processor.
  @param[out]   L1CacheHandle       Pointer to the handle of the L1 Cache SMBIOS record.
  @param[out]   L2CacheHandle       Pointer to the handle of the L2 Cache SMBIOS record.
  @param[out]   L3CacheHandle       Pointer to the handle of the L3 Cache SMBIOS record.

**/
VOID
AddSmbiosCacheTypeTable (
  IN UINTN               ProcessorIndex,
  OUT EFI_SMBIOS_HANDLE  *L1CacheHandle,
  OUT EFI_SMBIOS_HANDLE  *L2CacheHandle,
  OUT EFI_SMBIOS_HANDLE  *L3CacheHandle
  )
{
  EFI_STATUS          Status;
  SMBIOS_TABLE_TYPE7  *Type7Record;
  EFI_SMBIOS_HANDLE   SmbiosHandle;
  UINT8               CacheLevel;
  UINT8               MaxCacheLevel;
  BOOLEAN             DataCacheType;
  BOOLEAN             SeparateCaches;

  Status = EFI_SUCCESS;

  MaxCacheLevel = 0;

  // See if there's an L1 cache present.
  MaxCacheLevel = SmbiosProcessorGetMaxCacheLevel ();

  if (MaxCacheLevel < 1) {
    return;
  }

  for (CacheLevel = 1; CacheLevel <= MaxCacheLevel; CacheLevel++) {
    Type7Record = NULL;

    SeparateCaches = SmbiosProcessorHasSeparateCaches (CacheLevel);

    // At each level of cache, we can have a single type (unified, instruction or data),
    // or two types - separate data and instruction caches. If we have separate
    // instruction and data caches, then on the first iteration (CacheSubLevel = 0)
    // process the instruction cache.
    for (DataCacheType = 0; DataCacheType <= 1; DataCacheType++) {
      // If there's no separate data/instruction cache, skip the second iteration
      if ((DataCacheType == 1) && !SeparateCaches) {
        continue;
      }

      Type7Record = AllocateAndInitCacheInformation (
                      CacheLevel,
                      DataCacheType,
                      !SeparateCaches
                      );
      if (Type7Record == NULL) {
        continue;
      }

      ConfigureCacheArchitectureInformation (
        CacheLevel,
        DataCacheType,
        !SeparateCaches,
        Type7Record
        );

      // Allow the platform to fill in other information such as speed, SRAM type etc.
      if (!OemGetCacheInformation (
             ProcessorIndex,
             CacheLevel,
             DataCacheType,
             !SeparateCaches,
             Type7Record
             ))
      {
        continue;
      }

      SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
      // Finally, install the table
      Status = mSmbios->Add (
                          mSmbios,
                          NULL,
                          &SmbiosHandle,
                          (EFI_SMBIOS_TABLE_HEADER *)Type7Record
                          );
      if (EFI_ERROR (Status)) {
        continue;
      }

      // Config L1/L2/L3 Cache Handle
      switch (CacheLevel) {
        case CpuCacheL1:
          *L1CacheHandle = SmbiosHandle;
          break;
        case CpuCacheL2:
          *L2CacheHandle = SmbiosHandle;
          break;
        case CpuCacheL3:
          *L3CacheHandle = SmbiosHandle;
          break;
        default:
          break;
      }
    }
  }
}

/** Allocates a Type 4 Processor Information structure and sets the
    strings following the data fields.

  @param[out] Type4Record    The Type 4 structure to allocate and initialize
  @param[in]  ProcessorIndex The index of the processor
  @param[in]  Populated      Whether the specified processor is
                             populated.

  @retval EFI_SUCCESS          The Type 4 structure was successfully
                               allocated and the strings initialized.
  @retval EFI_OUT_OF_RESOURCES Could not allocate memory needed.
**/
EFI_STATUS
AllocateType4AndSetProcessorInformationStrings (
  SMBIOS_TABLE_TYPE4  **Type4Record,
  UINT8               ProcessorIndex,
  BOOLEAN             Populated
  )
{
  EFI_STATUS     Status;
  EFI_STRING_ID  ProcessorManu;
  EFI_STRING_ID  ProcessorVersion;
  EFI_STRING_ID  SerialNumber;
  EFI_STRING_ID  AssetTag;
  EFI_STRING_ID  PartNumber;
  EFI_STRING     ProcessorStr;
  EFI_STRING     ProcessorManuStr;
  EFI_STRING     ProcessorVersionStr;
  EFI_STRING     SerialNumberStr;
  EFI_STRING     AssetTagStr;
  EFI_STRING     PartNumberStr;
  CHAR8          *OptionalStrStart;
  CHAR8          *StrStart;
  UINTN          ProcessorStrLen;
  UINTN          ProcessorManuStrLen;
  UINTN          ProcessorVersionStrLen;
  UINTN          SerialNumberStrLen;
  UINTN          AssetTagStrLen;
  UINTN          PartNumberStrLen;
  UINTN          TotalSize;
  UINTN          StringBufferSize;

  Status = EFI_SUCCESS;

  ProcessorManuStr    = NULL;
  ProcessorVersionStr = NULL;
  SerialNumberStr     = NULL;
  AssetTagStr         = NULL;
  PartNumberStr       = NULL;

  ProcessorManu    = STRING_TOKEN (STR_PROCESSOR_MANUFACTURE);
  ProcessorVersion = STRING_TOKEN (STR_PROCESSOR_VERSION);
  SerialNumber     = STRING_TOKEN (STR_PROCESSOR_SERIAL_NUMBER);
  AssetTag         = STRING_TOKEN (STR_PROCESSOR_ASSET_TAG);
  PartNumber       = STRING_TOKEN (STR_PROCESSOR_PART_NUMBER);

  SET_HII_STRING_IF_PCD_NOT_EMPTY (PcdProcessorManufacturer, ProcessorManu);
  SET_HII_STRING_IF_PCD_NOT_EMPTY (PcdProcessorAssetTag, AssetTag);

  if (StrLen ((CHAR16 *)FixedPcdGetPtr (PcdProcessorSerialNumber)) > 0) {
    HiiSetString (mHiiHandle, SerialNumber, (CHAR16 *)FixedPcdGetPtr (PcdProcessorSerialNumber), NULL);
  } else {
    OemUpdateSmbiosInfo (mHiiHandle, SerialNumber, ProcessorSerialNumType04);
  }

  if (StrLen ((CHAR16 *)FixedPcdGetPtr (PcdProcessorPartNumber)) > 0) {
    HiiSetString (mHiiHandle, PartNumber, (CHAR16 *)FixedPcdGetPtr (PcdProcessorPartNumber), NULL);
  } else {
    OemUpdateSmbiosInfo (mHiiHandle, PartNumber, ProcessorPartNumType04);
  }

  if (StrLen ((CHAR16 *)FixedPcdGetPtr (PcdProcessorVersion)) > 0) {
    HiiSetString (mHiiHandle, ProcessorVersion, (CHAR16 *)FixedPcdGetPtr (PcdProcessorVersion), NULL);
  } else {
    OemUpdateSmbiosInfo (mHiiHandle, ProcessorVersion, ProcessorVersionType04);
  }

  // Processor Designation
  StringBufferSize = sizeof (CHAR16) * SMBIOS_STRING_MAX_LENGTH;
  ProcessorStr     = AllocateZeroPool (StringBufferSize);
  if (ProcessorStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ProcessorStrLen = UnicodeSPrint (
                      ProcessorStr,
                      StringBufferSize,
                      L"CPU%02d",
                      ProcessorIndex + 1
                      );

  // Processor Manufacture
  ProcessorManuStr    = HiiGetPackageString (&gEfiCallerIdGuid, ProcessorManu, NULL);
  ProcessorManuStrLen = StrLen (ProcessorManuStr);

  // Processor Version
  ProcessorVersionStr    = HiiGetPackageString (&gEfiCallerIdGuid, ProcessorVersion, NULL);
  ProcessorVersionStrLen = StrLen (ProcessorVersionStr);

  // Serial Number
  SerialNumberStr    = HiiGetPackageString (&gEfiCallerIdGuid, SerialNumber, NULL);
  SerialNumberStrLen = StrLen (SerialNumberStr);

  // Asset Tag
  AssetTagStr    = HiiGetPackageString (&gEfiCallerIdGuid, AssetTag, NULL);
  AssetTagStrLen = StrLen (AssetTagStr);

  // Part Number
  PartNumberStr    = HiiGetPackageString (&gEfiCallerIdGuid, PartNumber, NULL);
  PartNumberStrLen = StrLen (PartNumberStr);

  TotalSize = sizeof (SMBIOS_TABLE_TYPE4) +
              ProcessorStrLen        + 1 +
              ProcessorManuStrLen    + 1 +
              ProcessorVersionStrLen + 1 +
              SerialNumberStrLen     + 1 +
              AssetTagStrLen         + 1 +
              PartNumberStrLen       + 1 + 1;

  *Type4Record = AllocateZeroPool (TotalSize);
  if (*Type4Record == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  CopyMem (*Type4Record, &mSmbiosProcessorTableTemplate, sizeof (SMBIOS_TABLE_TYPE4));

  OptionalStrStart = (CHAR8 *)(*Type4Record + 1);
  UnicodeStrToAsciiStrS (
    ProcessorStr,
    OptionalStrStart,
    ProcessorStrLen + 1
    );

  StrStart = OptionalStrStart + ProcessorStrLen + 1;
  UnicodeStrToAsciiStrS (
    ProcessorManuStr,
    StrStart,
    ProcessorManuStrLen + 1
    );

  StrStart += ProcessorManuStrLen + 1;
  UnicodeStrToAsciiStrS (
    ProcessorVersionStr,
    StrStart,
    ProcessorVersionStrLen + 1
    );

  StrStart += ProcessorVersionStrLen + 1;
  UnicodeStrToAsciiStrS (
    SerialNumberStr,
    StrStart,
    SerialNumberStrLen + 1
    );

  StrStart += SerialNumberStrLen + 1;
  UnicodeStrToAsciiStrS (
    AssetTagStr,
    StrStart,
    AssetTagStrLen + 1
    );

  StrStart += AssetTagStrLen + 1;
  UnicodeStrToAsciiStrS (
    PartNumberStr,
    StrStart,
    PartNumberStrLen + 1
    );

Exit:
  FreePool (ProcessorStr);
  FreePool (ProcessorManuStr);
  FreePool (ProcessorVersionStr);
  FreePool (SerialNumberStr);
  FreePool (AssetTagStr);
  FreePool (PartNumberStr);

  return Status;
}

/**
  Add Type 4 SMBIOS Record for Processor Information.

  @param[in]    ProcessorIndex     Processor index of specified processor.

**/
EFI_STATUS
AddSmbiosProcessorTypeTable (
  IN UINTN  ProcessorIndex
  )
{
  EFI_STATUS                      Status;
  SMBIOS_TABLE_TYPE4              *Type4Record;
  EFI_SMBIOS_HANDLE               SmbiosHandle;
  EFI_SMBIOS_HANDLE               L1CacheHandle;
  EFI_SMBIOS_HANDLE               L2CacheHandle;
  EFI_SMBIOS_HANDLE               L3CacheHandle;
  UINT8                           *LegacyVoltage;
  PROCESSOR_STATUS_DATA           ProcessorStatus;
  UINT64                          *ProcessorId;
  PROCESSOR_CHARACTERISTIC_FLAGS  ProcessorCharacteristics;
  OEM_MISC_PROCESSOR_DATA         MiscProcessorData;
  BOOLEAN                         ProcessorPopulated;

  Type4Record = NULL;

  MiscProcessorData.Voltage      = 0;
  MiscProcessorData.CurrentSpeed = 0;
  MiscProcessorData.CoreCount    = 0;
  MiscProcessorData.CoresEnabled = 0;
  MiscProcessorData.ThreadCount  = 0;
  MiscProcessorData.MaxSpeed     = 0;
  L1CacheHandle                  = 0xFFFF;
  L2CacheHandle                  = 0xFFFF;
  L3CacheHandle                  = 0xFFFF;

  ProcessorPopulated = OemIsProcessorPresent (ProcessorIndex);

  Status = AllocateType4AndSetProcessorInformationStrings (
             &Type4Record,
             ProcessorIndex,
             ProcessorPopulated
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OemGetProcessorInformation (
    ProcessorIndex,
    &ProcessorStatus,
    (PROCESSOR_CHARACTERISTIC_FLAGS *)
    &Type4Record->ProcessorCharacteristics,
    &MiscProcessorData
    );

  if (ProcessorPopulated) {
    AddSmbiosCacheTypeTable (
      ProcessorIndex,
      &L1CacheHandle,
      &L2CacheHandle,
      &L3CacheHandle
      );
  }

  LegacyVoltage = (UINT8 *)&Type4Record->Voltage;

  *LegacyVoltage                 = MiscProcessorData.Voltage;
  Type4Record->CurrentSpeed      = MiscProcessorData.CurrentSpeed;
  Type4Record->MaxSpeed          = MiscProcessorData.MaxSpeed;
  Type4Record->Status            = ProcessorStatus.Data;
  Type4Record->L1CacheHandle     = L1CacheHandle;
  Type4Record->L2CacheHandle     = L2CacheHandle;
  Type4Record->L3CacheHandle     = L3CacheHandle;
  Type4Record->CoreCount         = MIN (MiscProcessorData.CoreCount, MAX_UINT8);
  Type4Record->CoreCount2        = MiscProcessorData.CoreCount;
  Type4Record->EnabledCoreCount  = MIN (MiscProcessorData.CoresEnabled, MAX_UINT8);
  Type4Record->EnabledCoreCount2 = MiscProcessorData.CoresEnabled;
  Type4Record->ThreadCount       = MIN (MiscProcessorData.ThreadCount, MAX_UINT8);
  Type4Record->ThreadCount2      = MiscProcessorData.ThreadCount;

  Type4Record->CurrentSpeed  = GetCpuFrequency (ProcessorIndex);
  Type4Record->ExternalClock =
    (UINT16)(SmbiosGetExternalClockFrequency () / 1000 / 1000);

  ProcessorId  = (UINT64 *)&Type4Record->ProcessorId;
  *ProcessorId = SmbiosGetProcessorId ();

  ProcessorCharacteristics               = SmbiosGetProcessorCharacteristics ();
  Type4Record->ProcessorCharacteristics |= *((UINT64 *)&ProcessorCharacteristics);

  Type4Record->ProcessorFamily  = SmbiosGetProcessorFamily ();
  Type4Record->ProcessorFamily2 = SmbiosGetProcessorFamily2 ();

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status       = mSmbios->Add (
                            mSmbios,
                            NULL,
                            &SmbiosHandle,
                            (EFI_SMBIOS_TABLE_HEADER *)Type4Record
                            );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Smbios Type04 Table Log Failed! %r \n",
      __func__,
      DEBUG_LINE_NUMBER,
      Status
      ));
  }

  FreePool (Type4Record);

  return Status;
}

/**
   Standard EFI driver point.

  @param  ImageHandle     Handle for the image of this driver
  @param  SystemTable     Pointer to the EFI System Table

  @retval  EFI_SUCCESS    The data was successfully stored.

**/
EFI_STATUS
EFIAPI
ProcessorSubClassEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT32      ProcessorIndex;

  //
  // Locate dependent protocols
  //
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&mSmbios);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Could not locate SMBIOS protocol.  %r\n", Status));
    return Status;
  }

  //
  // Add our default strings to the HII database. They will be modified later.
  //
  mHiiHandle = HiiAddPackages (
                 &gEfiCallerIdGuid,
                 NULL,
                 ProcessorSubClassStrings,
                 NULL,
                 NULL
                 );
  if (mHiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Add SMBIOS tables for populated sockets.
  //
  for (ProcessorIndex = 0; ProcessorIndex < OemGetMaxProcessors (); ProcessorIndex++) {
    Status = AddSmbiosProcessorTypeTable (ProcessorIndex);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Add Processor Type Table Failed!  %r.\n", Status));
      return Status;
    }
  }

  return Status;
}
