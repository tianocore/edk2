/** @file
*  ProcessorSubClass.c
*
*  Copyright (c) 2020, NUVIA Inc. All rights reserved.
*  Copyright (c) 2015, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2015, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include "ProcessorSubClass.h"

#define CACHE_SOCKETED_SHIFT       3
#define CACHE_LOCATION_SHIFT       5
#define CACHE_ENABLED_SHIFT        7
#define CACHE_OPERATION_MODE_SHIFT 8

typedef enum {
  CacheModeWriteThrough = 0,
  CacheModeWriteBack,
  CacheModeVariesWithAddress,
  CacheModeUnknown
} CacheOperationMode;

typedef enum {
  CacheLocationInternal = 0,
  CacheLocationExternal,
  CacheLocationReserved,
  CacheLocationUnknown
} CacheLocation;

EFI_HII_HANDLE       mHiiHandle;

EFI_SMBIOS_PROTOCOL  *mSmbios;

SMBIOS_TABLE_TYPE4 mSmbiosProcessorTable[] = {
  {
    {                                          //Header
      EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION,   //Type
      sizeof (SMBIOS_TABLE_TYPE4),             //Length
      0                                        //Handle
    },
    1,                                         //Socket
    CentralProcessor,                          //ProcessorType
    ProcessorFamilyIndicatorFamily2,           //ProcessorFamily
    2,                                         //ProcessorManufacture
    {                                          //ProcessorId
      {                                        //Signature
        0
      },
      {                                        //FeatureFlags
        0
      }
    },
    3,                                         //ProcessorVersion
    {                                          //Voltage
      0
    },
    0,                                         //ExternalClock
    0,                                         //MaxSpeed
    0,                                         //CurrentSpeed
    0,                                         //Status
    ProcessorUpgradeUnknown,                   //ProcessorUpgrade
    0xFFFF,                                    //L1CacheHandle
    0xFFFF,                                    //L2CacheHandle
    0xFFFF,                                    //L3CacheHandle
    4,                                         //SerialNumber
    5,                                         //AssetTag
    6,                                         //PartNumber
    0,                                         //CoreCount
    0,                                         //EnabledCoreCount
    0,                                         //ThreadCount
    0,                                         //ProcessorCharacteristics
    ProcessorFamilyARM,                        //ProcessorFamily2
    0,                                         //CoreCount2
    0,                                         //EnabledCoreCount2
    0                                          //ThreadCount2
  },

  //CPU1
  {
    {                                          //Header
      EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION,   //Type
      sizeof (SMBIOS_TABLE_TYPE4),             //Length
      0                                        //Handle
    },
    1,                                         //Socket
    CentralProcessor,                          //ProcessorType
    ProcessorFamilyIndicatorFamily2,           //ProcessorFamily
    2,                                         //ProcessorManufacture
    {                                          //ProcessorId
      {                                        //Signature
        0
      },
      {                                        //FeatureFlags
        0
      }
    },
    3,                                         //ProcessorVersion
    {                                          //Voltage
      0
    },
    0,                                         //ExternalClock
    0,                                         //MaxSpeed
    0,                                         //CurrentSpeed
    0,                                         //Status
    ProcessorUpgradeUnknown,                   //ProcessorUpgrade
    0xFFFF,                                    //L1CacheHandle
    0xFFFF,                                    //L2CacheHandle
    0xFFFF,                                    //L3CacheHandle
    4,                                         //SerialNumber
    5,                                         //AssetTag
    6,                                         //PartNumber
    0,                                         //CoreCount
    0,                                         //EnabledCoreCount
    0,                                         //ThreadCount
    0,                                         //ProcessorCharacteristics
    ProcessorFamilyARMv8,                      //ProcessorFamily2
    0,                                         //CoreCount2
    0,                                         //EnabledCoreCount2
    0                                          //ThreadCount2
  }
};

/** Fetches the specified processor's frequency in Hz
 *
 * @param ProcessorNumber The processor number
 *
 * @return The clock frequency in MHz
 *
**/
STATIC
UINT16
GetCpuFrequency (
  IN  UINT8 ProcessorNumber
  )
{
  return (UINT16)(PlatformGetCpuFreq (ProcessorNumber)/1000/1000);
}

/** Gets a description of the specified cache
 *
 * @param[in] CacheLevel      Zero-based cache level (e.g. L1 cache is 0)
 * @param[in] CacheSubLevel   Where the cache level has separate data and
 *                            instruction caches, 0 is instruction and 1 is data
 * @param[out] CacheSocketStr The description of the specified cache
 *
 * @return The number of Unicode characters in CacheSocketStr not including the
 *         terminating NUL
**/
STATIC
UINTN
GetCacheSocketStr (
  IN  UINT8     CacheLevel,
  IN  UINT8     CacheSubLevel,
  OUT CHAR16    *CacheSocketStr
  )
{
  UINTN CacheSocketStrLen;

  if (CacheLevel == CPU_CACHE_L1 && CacheSubLevel == 0) {
    CacheSocketStrLen = UnicodeSPrint (CacheSocketStr, SMBIOS_STRING_MAX_LENGTH - 1,
                                       L"L%x Instruction Cache", CacheLevel + 1);
  } else if (CacheLevel == CPU_CACHE_L1 && CacheSubLevel == 1) {
    CacheSocketStrLen = UnicodeSPrint (CacheSocketStr, SMBIOS_STRING_MAX_LENGTH - 1,
                                       L"L%x Data Cache", CacheLevel + 1);
  } else {
    CacheSocketStrLen = UnicodeSPrint (CacheSocketStr, SMBIOS_STRING_MAX_LENGTH - 1,
                                       L"L%x Cache", CacheLevel + 1);
  }

  return CacheSocketStrLen;
}

/** Fills in the Type 7 record with the cache architecture information
 *  read from the CPU registers.
 *
 * @param CacheLevel     Cache level (e.g. L1)
 * @param CacheSubLevel  Type of cache (e.g. instruction)
 * @param CcidxSupported Whether CCIDX is supported
 * @param CacheType      The type of cache supported at this cache level
 * @param Type7Record    The Type 7 record to fill in
 *
**/
STATIC
VOID
SetCacheArchitectureInformation (
  UINT8 CacheLevel,
  UINT8 CacheSubLevel,
  BOOLEAN CcidxSupported,
  CLIDR_CACHE_TYPE CacheType,
  SMBIOS_TABLE_TYPE7 *Type7Record
  )
{
  CSSELR_DATA  Csselr;
  CCSIDR_DATA  Ccsidr;
  UINT8        Associativity;
  UINT32       CacheSize32;
  UINT16       CacheSize16;
  UINT64       CacheSize64;

  Csselr.Data = 0;
  Csselr.Bits.Level = CacheLevel;

  if (CacheSubLevel == 0) {
    if (CacheType == ClidrCacheTypeInstructionOnly ||
        CacheType == ClidrCacheTypeSeparate) {
      Csselr.Bits.InD = CsselrCacheTypeInstruction;
      Type7Record->SystemCacheType = CacheTypeInstruction;
    } else {
      Csselr.Bits.InD = CsselrCacheTypeDataOrUnified;
      if (CacheType == ClidrCacheTypeDataOnly) {
        Type7Record->SystemCacheType = CacheTypeData;
      } else {
        Type7Record->SystemCacheType = CacheTypeUnified;
      }
    }
  } else {
    Type7Record->SystemCacheType = CacheTypeData;
    Csselr.Bits.InD = CsselrCacheTypeDataOrUnified;
  }

  // Read the CCSIDR register to get the cache architecture
  Ccsidr.Data = ReadCCSIDR (Csselr.Data);

  if (CcidxSupported) {
    CacheSize64 = (UINT64)(1 << (Ccsidr.BitsCcidx.LineSize + 4)) *
                                (Ccsidr.BitsCcidx.Associativity + 1) *
                                (Ccsidr.BitsCcidx.NumSets + 1);
    Associativity = Ccsidr.BitsCcidx.Associativity;
  } else {
    CacheSize64 = (1 << (Ccsidr.BitsNonCcidx.LineSize + 4)) *
                        (Ccsidr.BitsNonCcidx.Associativity + 1) *
                        (Ccsidr.BitsNonCcidx.NumSets + 1);
    Associativity = Ccsidr.BitsNonCcidx.Associativity;
  }

  CacheSize64 /= 1024; // Minimum granularity is 1K

  // Encode the cache size into the format SMBIOS wants
  if (CacheSize64 < MAX_INT16) {
    CacheSize16 = CacheSize64;
    CacheSize32 = CacheSize16;
  } else if ((CacheSize64 / 64) < MAX_INT16) {
    CacheSize16 = (1 << 15) | (CacheSize64 / 64);
    CacheSize32 = CacheSize16;
  } else {
    if ((CacheSize64 / 1024) <= 2047) {
      CacheSize32 = CacheSize64;
    } else {
      CacheSize32 = (1 << 31) | (CacheSize64 / 64);
    }

    CacheSize16 = -1;
  }

  Type7Record->Associativity = Associativity + 1;
  Type7Record->MaximumCacheSize = CacheSize16;
  Type7Record->InstalledSize = CacheSize16;
  Type7Record->MaximumCacheSize2 = CacheSize32;
  Type7Record->InstalledSize2 = CacheSize32;

  switch (Associativity + 1) {
    case 2:
      Type7Record->Associativity = CacheAssociativity2Way;
      break;
    case 4:
      Type7Record->Associativity = CacheAssociativity4Way;
      break;
    case 8:
      Type7Record->Associativity = CacheAssociativity8Way;
      break;
    case 16:
      Type7Record->Associativity = CacheAssociativity16Way;
      break;
    case 12:
      Type7Record->Associativity = CacheAssociativity12Way;
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
    case 20:
      Type7Record->Associativity = CacheAssociativity20Way;
      break;
    default:
      Type7Record->Associativity = CacheAssociativityOther;
      break;
  }

  Type7Record->CacheConfiguration = (CacheModeUnknown << CACHE_OPERATION_MODE_SHIFT) |
                                    (1 << CACHE_ENABLED_SHIFT) |
                                    (CacheLocationUnknown << CACHE_LOCATION_SHIFT) |
                                    (0 << CACHE_SOCKETED_SHIFT) |
                                    CacheLevel;
}


STATIC
SMBIOS_TABLE_TYPE7*
AllocateAndInitCacheInformation (
  UINT8 CacheLevel,
  UINT8 CacheSubLevel
  )
{
  SMBIOS_TABLE_TYPE7          *Type7Record;
  EFI_STRING                  CacheSocketStr;
  UINTN                       CacheSocketStrLen;
  UINTN                       StringBufferSize;
  CHAR8                       *OptionalStrStart;
  UINTN                       TableSize;
  EFI_STATUS                  Status;

  // Allocate and fetch the cache description
  StringBufferSize = sizeof (CHAR16) * SMBIOS_STRING_MAX_LENGTH;
  CacheSocketStr = AllocateZeroPool (StringBufferSize);
  if (CacheSocketStr == NULL) {
    return NULL;
  }

  CacheSocketStrLen = GetCacheSocketStr (CacheLevel, CacheSubLevel, CacheSocketStr);

  TableSize = sizeof (SMBIOS_TABLE_TYPE7) + CacheSocketStrLen + 1 + 1;
  Type7Record = AllocateZeroPool (TableSize);
  if (Type7Record == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    FreePool(CacheSocketStr);
    return NULL;
  }

  Type7Record->Hdr.Type = EFI_SMBIOS_TYPE_CACHE_INFORMATION;
  Type7Record->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE7);
  Type7Record->Hdr.Handle = SMBIOS_HANDLE_PI_RESERVED;

  Type7Record->SocketDesignation = 1;

  Type7Record->SupportedSRAMType.Unknown = 1;
  Type7Record->CurrentSRAMType.Unknown = 1;
  Type7Record->CacheSpeed = 0;
  Type7Record->ErrorCorrectionType = CacheErrorUnknown;

  OptionalStrStart = (CHAR8 *)(Type7Record + 1);
  UnicodeStrToAsciiStrS (CacheSocketStr, OptionalStrStart, CacheSocketStrLen + 1);
  FreePool (CacheSocketStr);
}

/**
 * Add Type 7 SMBIOS Record for Cache Information.
 *
 * @param[in]    ProcessorNumber     Processor number of specified processor.
 * @param[out]   L1CacheHandle       Pointer to the handle of the L1 Cache SMBIOS record.
 * @param[out]   L2CacheHandle       Pointer to the handle of the L2 Cache SMBIOS record.
 * @param[out]   L3CacheHandle       Pointer to the handle of the L3 Cache SMBIOS record.
 *
**/
EFI_STATUS
AddSmbiosCacheTypeTable (
  IN UINTN                  ProcessorNumber,
  OUT EFI_SMBIOS_HANDLE     *L1CacheHandle,
  OUT EFI_SMBIOS_HANDLE     *L2CacheHandle,
  OUT EFI_SMBIOS_HANDLE     *L3CacheHandle
  )
{
  EFI_STATUS                  Status;
  SMBIOS_TABLE_TYPE7          *Type7Record;
  EFI_SMBIOS_HANDLE           SmbiosHandle;
  UINT8                       CacheLevel;
  UINT8                       CacheSubLevel;
  CLIDR_DATA                  Clidr;
  BOOLEAN                     CcidxSupported;
  UINT8                       MaxCacheLevel;

  Status = EFI_SUCCESS;

  MaxCacheLevel = 0;

  // Read the CLIDR register to find out what caches are present.
  Clidr.Data = ReadCLIDR ();

  // Get the cache type for the L1 cache. If it's 0, there are no caches.
  if (CLIDR_GET_CACHE_TYPE (Clidr.Data, 0) == ClidrCacheTypeNone) {
    return EFI_SUCCESS;
  }

  for (CacheLevel = 1; CacheLevel < MAX_ARM_CACHE_LEVEL; CacheLevel++) {
    if (CLIDR_GET_CACHE_TYPE (Clidr.Data, CacheLevel) == ClidrCacheTypeNone) {
      MaxCacheLevel = CacheLevel;
      break;
    }
  }

  CcidxSupported = ArmIsCcidxImplemented ();

  for (CacheLevel = 0; CacheLevel < MaxCacheLevel; CacheLevel++) {
    Type7Record = NULL;

    CLIDR_CACHE_TYPE CacheType = CLIDR_GET_CACHE_TYPE (Clidr.Data, CacheLevel);

    // At each level of cache, we can have a single type (unified, instruction or data),
    // or two types - separate data and instruction caches. If we have separate
    // instruction and data caches, then on the first iteration (CacheSubLevel = 0)
    // process the instruction cache.
    for (CacheSubLevel = 0; CacheSubLevel <= 1; CacheSubLevel++) {
      // If there's no separate data/instruction cache, skip the second iteration
      if (CacheSubLevel > 0 && CacheType != ClidrCacheTypeSeparate) {
        continue;
      }

      Type7Record = AllocateAndInitCacheInformation (CacheLevel, CacheSubLevel);
      if (Type7Record == NULL) {
        continue;
      }

      SetCacheArchitectureInformation(CacheLevel, CacheSubLevel, CcidxSupported,
                                       CacheType, Type7Record);

      // Allow the platform to fill in other information such as speed, SRAM type etc.
      if (!OemGetCacheInformation (CacheLevel, Type7Record)) {
        continue;
      }

      SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
      // Finally, install the table
      Status = mSmbios->Add (mSmbios, NULL, &SmbiosHandle,
                             (EFI_SMBIOS_TABLE_HEADER *)Type7Record);
      if (EFI_ERROR (Status)) {
        continue;
      }

      // Config L1/L2/L3 Cache Handle
      switch (CacheLevel) {
        case CPU_CACHE_L1:
          *L1CacheHandle = SmbiosHandle;
          break;
        case CPU_CACHE_L2:
          *L2CacheHandle = SmbiosHandle;
          break;
        case CPU_CACHE_L3:
          *L3CacheHandle = SmbiosHandle;
          break;
        default:
            break;
      }
    }
  }

  return Status;
}


/** Fills in the Type 4 CPU processor ID field
 *
 * @param Type4Record The SMBIOS Type 4 record to fill in
 *
**/
STATIC
VOID
SetProcessorIdField (
  SMBIOS_TABLE_TYPE4 *Type4Record
)
{
  ARM_SMC_ARGS Args;
  INT32 SmcCallStatus;
  INT32 Jep106Code;
  INT32 SocRevision;
  BOOLEAN Arm64SocIdSupported = FALSE;
  UINT64 *ProcessorId;

  Args.Arg0 = SMCCC_VERSION;
  ArmCallSmc (&Args);
  SmcCallStatus = (INT32)Args.Arg0;

  if (SmcCallStatus < 0 || (SmcCallStatus >> 16) >= 1) {
    Args.Arg0 = SMCCC_ARCH_FEATURES;
    Args.Arg1 = SMCCC_ARCH_SOC_ID;
    ArmCallSmc (&Args);

    if (Args.Arg0 >= 0) {
      PROCESSOR_CHARACTERISTIC_FLAGS *ProcessorCharacteristicFlags =
        (PROCESSOR_CHARACTERISTIC_FLAGS*)&Type4Record->ProcessorCharacteristics;
      Args.Arg0 = SMCCC_ARCH_SOC_ID;
      Args.Arg1 = 0;
      ArmCallSmc (&Args);
      SmcCallStatus = (int)Args.Arg0;

      if (SmcCallStatus >= 0) {
        Arm64SocIdSupported = TRUE;
        ProcessorCharacteristicFlags->ProcessorArm64SocId = 1;
        Jep106Code = (int)Args.Arg0;
      } else {
        ProcessorCharacteristicFlags->ProcessorArm64SocId = 0;
      }
      Args.Arg0 = SMCCC_ARCH_SOC_ID;
      Args.Arg1 = 1;
      ArmCallSmc (&Args);
      SmcCallStatus = (int)Args.Arg0;

      if (SmcCallStatus >= 0) {
        SocRevision = (int)Args.Arg0;
      }
    }
  }

  ProcessorId = (UINT64 *)&Type4Record->ProcessorId;

  if (Arm64SocIdSupported) {
    *ProcessorId = ((UINT64)Jep106Code << 32) | SocRevision;
  } else {
    *ProcessorId = ArmReadMidr ();
  }
}


STATIC
UINTN
SetProcessorInformationStrings (
  SMBIOS_TABLE_TYPE4 *Type4Record,
  UINT8 ProcessorNumber,
  BOOLEAN Populated
  )
{
  EFI_STATUS      Status;
  EFI_STRING_ID   ProcessorManu;
  EFI_STRING_ID   ProcessorVersion;
  EFI_STRING_ID   SerialNumber;
  EFI_STRING_ID   AssetTag;
  EFI_STRING_ID   PartNumber;
  EFI_STRING      ProcessorSocketStr;
  EFI_STRING      ProcessorManuStr;
  EFI_STRING      ProcessorVersionStr;
  EFI_STRING      SerialNumberStr;
  EFI_STRING      AssetTagStr;
  EFI_STRING      PartNumberStr;
  CHAR8           *OptionalStrStart;
  CHAR8           *StrStart;
  UINTN           ProcessorSocketStrLen;
  UINTN           ProcessorManuStrLen;
  UINTN           ProcessorVersionStrLen;
  UINTN           SerialNumberStrLen;
  UINTN           AssetTagStrLen;
  UINTN           PartNumberStrLen;
  UINTN           TotalSize;
  UINTN           StringBufferSize;

  ProcessorManuStr    = NULL;
  ProcessorVersionStr = NULL;
  SerialNumberStr     = NULL;
  AssetTagStr         = NULL;
  PartNumberStr       = NULL;

  ProcessorManu       = STRING_TOKEN (STR_PROCESSOR_UNKNOWN);
  ProcessorVersion    = STRING_TOKEN (STR_PROCESSOR_UNKNOWN);
  SerialNumber        = STRING_TOKEN (STR_PROCESSOR_UNKNOWN);
  AssetTag            = STRING_TOKEN (STR_PROCESSOR_UNKNOWN);
  PartNumber          = STRING_TOKEN (STR_PROCESSOR_UNKNOWN);

  if (Populated) {
    ProcessorManu       = STRING_TOKEN (STR_PROCESSOR_MANUFACTURE);
    ProcessorVersion    = STRING_TOKEN (STR_PROCESSOR_VERSION);
    SerialNumber        = STRING_TOKEN (STR_PROCESSOR_SERIAL_NUMBER);
    AssetTag            = STRING_TOKEN (STR_PROCESSOR_ASSET_TAG);
    PartNumber          = STRING_TOKEN (STR_PROCESSOR_PART_NUMBER);

    SET_HII_STRING_IF_PCD_NOT_EMPTY(PcdProcessorManufacturer, ProcessorManu);
    SET_HII_STRING_IF_PCD_NOT_EMPTY(PcdProcessorVersion, ProcessorVersion);
    SET_HII_STRING_IF_PCD_NOT_EMPTY(PcdProcessorSerialNumber, SerialNumber);
    SET_HII_STRING_IF_PCD_NOT_EMPTY(PcdProcessorAssetTag, AssetTag);
    SET_HII_STRING_IF_PCD_NOT_EMPTY(PcdProcessorPartNumber, PartNumber);
  }

  // Processor Socket Designation
  StringBufferSize = sizeof (CHAR16) * SMBIOS_STRING_MAX_LENGTH;
  ProcessorSocketStr = AllocateZeroPool (StringBufferSize);
  if (ProcessorSocketStr == NULL) {
    return 0;
  }

  ProcessorSocketStrLen = UnicodeSPrint (ProcessorSocketStr, StringBufferSize,
                                         L"CPU%02d", ProcessorNumber + 1);

  // Processor Manufacture
  ProcessorManuStr = HiiGetPackageString (&gEfiCallerIdGuid, ProcessorManu, NULL);
  ProcessorManuStrLen = StrLen (ProcessorManuStr);

  // Processor Version
  ProcessorVersionStr = HiiGetPackageString (&gEfiCallerIdGuid, ProcessorVersion, NULL);
  ProcessorVersionStrLen = StrLen (ProcessorVersionStr);

  // Serial Number
  SerialNumberStr = HiiGetPackageString (&gEfiCallerIdGuid, SerialNumber, NULL);
  SerialNumberStrLen = StrLen (SerialNumberStr);

  // Asset Tag
  AssetTagStr = HiiGetPackageString (&gEfiCallerIdGuid, AssetTag, NULL);
  AssetTagStrLen = StrLen (AssetTagStr);

  // Part Number
  PartNumberStr = HiiGetPackageString (&gEfiCallerIdGuid, PartNumber, NULL);
  PartNumberStrLen = StrLen (PartNumberStr);

  TotalSize = sizeof (SMBIOS_TABLE_TYPE4) + ProcessorSocketStrLen + 1 +
                                            ProcessorManuStrLen + 1 +
                                            ProcessorVersionStrLen + 1 +
                                            SerialNumberStrLen + 1 +
                                            AssetTagStrLen + 1 +
                                            PartNumberStrLen + 1 + 1;

  OptionalStrStart = (CHAR8 *)(Type4Record + 1);
  UnicodeStrToAsciiStrS (ProcessorSocketStr, OptionalStrStart,
                         ProcessorSocketStrLen + 1);
  StrStart = OptionalStrStart + ProcessorSocketStrLen + 1;
  UnicodeStrToAsciiStrS (ProcessorManuStr, StrStart, ProcessorManuStrLen + 1);
  StrStart += ProcessorManuStrLen + 1;
  UnicodeStrToAsciiStrS (ProcessorVersionStr, StrStart, ProcessorVersionStrLen + 1);
  StrStart += ProcessorVersionStrLen + 1;
  UnicodeStrToAsciiStrS (SerialNumberStr, StrStart, SerialNumberStrLen + 1);
  StrStart += SerialNumberStrLen + 1;
  UnicodeStrToAsciiStrS (AssetTagStr, StrStart, AssetTagStrLen + 1);
  StrStart += AssetTagStrLen + 1;
  UnicodeStrToAsciiStrS (PartNumberStr, StrStart, PartNumberStrLen + 1);

  FreePool (ProcessorSocketStr);
  FreePool (ProcessorManuStr);
  FreePool (ProcessorVersionStr);
  FreePool (SerialNumberStr);
  FreePool (AssetTagStr);
  FreePool (PartNumberStr);

  return TotalSize;
}


#define SET_HII_STRING_IF_NOT_EMPTY(pcd, id) \
  id##Str = (CHAR16 *)PcdGetPtr (pcd); \
  if (StrLen(id##Str) > 0) { \
    HiiSetString (mHiiHandle, id, id##Str, NULL); \
  }


/**
 * Add Type 4 SMBIOS Record for Processor Information.
 *
 * @param[in]    ProcessorNumber     Processor number of specified processor.
 *
**/
EFI_STATUS
AddSmbiosProcessorTypeTable (
  IN UINTN                  ProcessorNumber
  )
{
  EFI_STATUS                  Status;
  SMBIOS_TABLE_TYPE4          *Type4Record;
  EFI_SMBIOS_HANDLE           SmbiosHandle;
  EFI_SMBIOS_HANDLE           L1CacheHandle;
  EFI_SMBIOS_HANDLE           L2CacheHandle;
  EFI_SMBIOS_HANDLE           L3CacheHandle;
  UINT8                       *LegacyVoltage;
  UINTN                       TotalSize;
  PROCESSOR_STATUS_DATA       ProcessorStatus = {{0}};
  MISC_PROCESSOR_DATA         MiscProcessorData;

  Type4Record         = NULL;

  MiscProcessorData.Voltage             = 0;
  MiscProcessorData.CurrentSpeed        = 0;
  MiscProcessorData.CoreCount           = 0;
  MiscProcessorData.CoresEnabled        = 0;
  MiscProcessorData.ThreadCount         = 0;
  L1CacheHandle       = 0xFFFF;
  L2CacheHandle       = 0xFFFF;
  L3CacheHandle       = 0xFFFF;

  BOOLEAN Populated = OemGetProcessorInformation (ProcessorNumber,
                                                  &ProcessorStatus,
                                                  (PROCESSOR_CHARACTERISTIC_FLAGS*)
                                                  &mSmbiosProcessorTable[ProcessorNumber].ProcessorCharacteristics,
                                                  &MiscProcessorData);
  if (Populated) {
    Status = AddSmbiosCacheTypeTable (ProcessorNumber, &L1CacheHandle,
                                      &L2CacheHandle, &L3CacheHandle);
  }

  TotalSize = SetProcessorInformationStrings (Populated);

  Type4Record = AllocateZeroPool (TotalSize);
  if (Type4Record == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  (VOID)CopyMem (Type4Record, &mSmbiosProcessorTable[ProcessorNumber], sizeof (SMBIOS_TABLE_TYPE4));

  LegacyVoltage = (UINT8*)&Type4Record->Voltage;

  *LegacyVoltage                          = MiscProcessorData.Voltage;
  Type4Record->CurrentSpeed               = MiscProcessorData.CurrentSpeed;
  Type4Record->MaxSpeed                   = MiscProcessorData.MaxSpeed;
  Type4Record->Status                     = ProcessorStatus.Data;
  Type4Record->L1CacheHandle              = L1CacheHandle;
  Type4Record->L2CacheHandle              = L2CacheHandle;
  Type4Record->L3CacheHandle              = L3CacheHandle;
  Type4Record->CoreCount                  = MiscProcessorData.CoreCount;
  Type4Record->CoreCount2                 = MiscProcessorData.CoreCount;
  Type4Record->EnabledCoreCount           = MiscProcessorData.CoresEnabled;
  Type4Record->EnabledCoreCount2          = MiscProcessorData.CoresEnabled;
  Type4Record->ThreadCount                = MiscProcessorData.ThreadCount;
  Type4Record->ThreadCount2               = MiscProcessorData.ThreadCount;

  Type4Record->ExternalClock              = (UINT16)(ArmReadCntFrq () / 1000 / 1000);

  SetProcessorIdField (Type4Record);

  UINTN MainIdRegister = ArmReadMidr ();
  if (((MainIdRegister >> 16) & 0xF) < 8) {
    Type4Record->ProcessorFamily2 = ProcessorFamilyARM;
  } else {
    if (sizeof (VOID*) == 4) {
      Type4Record->ProcessorFamily2 = ProcessorFamilyARMv7;
    } else {
      Type4Record->ProcessorFamily2 = ProcessorFamilyARMv8;
    }
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = mSmbios->Add (mSmbios, NULL, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *)Type4Record);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a]:[%dL] Smbios Type04 Table Log Failed! %r \n",
            __FUNCTION__, __LINE__, Status));
  }
  FreePool (Type4Record);

  return Status;
}

/**
 * Standard EFI driver point.  This driver locates the ProcessorConfigurationData Variable,
 * if it exists, add the related SMBIOS tables by PI SMBIOS protocol.
 *
 * @param  ImageHandle     Handle for the image of this driver
 * @param  SystemTable     Pointer to the EFI System Table
 *
 * @retval  EFI_SUCCESS    The data was successfully stored.
 *
**/
EFI_STATUS
EFIAPI
ProcessorSubClassEntryPoint(
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                      Status;
  UINT32                          SocketIndex;

  //
  // Locate dependent protocols
  //
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID**)&mSmbios);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Could not locate SMBIOS protocol.  %r\n", Status));
    return Status;
  }

  //
  // Add our default strings to the HII database. They will be modified later.
  //
  mHiiHandle = HiiAddPackages (&gEfiCallerIdGuid,
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
  for (SocketIndex = 0; SocketIndex < OemGetProcessorMaxSockets(); SocketIndex++) {
    Status = AddSmbiosProcessorTypeTable (SocketIndex);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Add Processor Type Table Failed!  %r.\n", Status));
      return Status;
    }
  }

  return Status;
}
