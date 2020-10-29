/** @file
*
*  Copyright (c) 2015, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2015, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include "ProcessorSubClass.h"

EFI_HII_HANDLE                  mHiiHandle;

EFI_SMBIOS_PROTOCOL             *mSmbios;

SMBIOS_TABLE_TYPE7   mSmbiosCacheTable[] = {
  //L1 Instruction Cache
  {
    {                                               //Header
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,              //Type
      sizeof (SMBIOS_TABLE_TYPE7),                    //Length
      0                                               //Handle
    },
    1,                                              //SocketDesignation
    0,                                              //CacheConfiguration
    0,                                              //MaximumCacheSize
    48,                                             //InstalledSize
    {                                               //SupportedSRAMType
      0
    },
    {                                               //CurrentSRAMType
      0
    },
    0,                                              //CacheSpeed
    CacheErrorParity,                               //ErrorCorrectionType
    CacheTypeInstruction,                           //SystemCacheType
    CacheAssociativity8Way                          //Associativity
  },

  //L1 Data Cache
  {
    {                                               //Header
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,              //Type
      sizeof (SMBIOS_TABLE_TYPE7),                    //Length
      0                                               //Handle
    },
    1,                                              //SocketDesignation
    0,                                              //CacheConfiguration
    0,                                              //MaximumCacheSize
    32,                                              //InstalledSize
    {                                               //SupportedSRAMType
      0
    },
    {                                               //CurrentSRAMType
      0
    },
    0,                                              //CacheSpeed
    CacheErrorSingleBit,                            //ErrorCorrectionType
    CacheTypeData,                                  //SystemCacheType
    CacheAssociativity8Way                          //Associativity
  },

  //L2 Cache
  {
    {                                               //Header
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,              //Type
      sizeof (SMBIOS_TABLE_TYPE7),                    //Length
      0                                               //Handle
    },
    1,                                              //SocketDesignation
    0,                                              //CacheConfiguration
    0,                                              //MaximumCacheSize
    4096,                                           //InstalledSize
    {                                               //SupportedSRAMType
      0
    },
    {                                               //CurrentSRAMType
      0
    },
    0,                                              //CacheSpeed
    CacheErrorSingleBit,                            //ErrorCorrectionType
    CacheTypeUnified,                               //SystemCacheType
    CacheAssociativity8Way                          //Associativity
  },

  //L3 Cache
  {
    {                                               //Header
      EFI_SMBIOS_TYPE_CACHE_INFORMATION,              //Type
      sizeof (SMBIOS_TABLE_TYPE7),                    //Length
      0                                               //Handle
    },
    1,                                              //SocketDesignation
    0,                                              //CacheConfiguration
    0,                                              //MaximumCacheSize
    16384,                                          //InstalledSize
    {                                               //SupportedSRAMType
      0
    },
    {                                               //CurrentSRAMType
      0
    },
    0,                                              //CacheSpeed
    CacheErrorSingleBit,                            //ErrorCorrectionType
    CacheTypeUnified,                               //SystemCacheType
    CacheAssociativity16Way                         //Associativity
  }
};

SMBIOS_TABLE_TYPE4   mSmbiosProcessorTable[] = {
  //CPU0
  {
    {                                               //Header
      EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION,          //Type
      sizeof (SMBIOS_TABLE_TYPE4),                    //Length
      0                                               //Handle
    },
    1,                                              //Socket
    CentralProcessor,                               //ProcessorType
    ProcessorFamilyIndicatorFamily2,                //ProcessorFamily
    2,                                              //ProcessorManufacture
    {                                               //ProcessorId
      {                                               //Signature
        0
      },
      {                                               //FeatureFlags
        0
      }
    },
    3,                                              //ProcessorVersion
    {                                               //Voltage
      0
    },
    0,                                              //ExternalClock
    0,                                              //MaxSpeed
    0,                                              //CurrentSpeed
    0,                                              //Status
    ProcessorUpgradeUnknown,                        //ProcessorUpgrade
    0xFFFF,                                         //L1CacheHandle
    0xFFFF,                                         //L2CacheHandle
    0xFFFF,                                         //L3CacheHandle
    4,                                              //SerialNumber
    5,                                              //AssetTag
    6,                                              //PartNumber
    0,                                              //CoreCount
    0,                                              //EnabledCoreCount
    0,                                              //ThreadCount
    0,                                              //ProcessorCharacteristics
    ProcessorFamilyARM,                             //ProcessorFamily2
    0,                                              //CoreCount2
    0,                                              //EnabledCoreCount2
    0                                               //ThreadCount2
  },

  //CPU1
  {
    {                                               //Header
      EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION,          //Type
      sizeof (SMBIOS_TABLE_TYPE4),                    //Length
      0                                               //Handle
    },
    1,                                              //Socket
    CentralProcessor,                               //ProcessorType
    ProcessorFamilyIndicatorFamily2,                //ProcessorFamily
    2,                                              //ProcessorManufacture
    {                                               //ProcessorId
      {                                               //Signature
        0
      },
      {                                               //FeatureFlags
        0
      }
    },
    3,                                              //ProcessorVersion
    {                                               //Voltage
      0
    },
    0,                                              //ExternalClock
    0,                                              //MaxSpeed
    0,                                              //CurrentSpeed
    0,                                              //Status
    ProcessorUpgradeUnknown,                        //ProcessorUpgrade
    0xFFFF,                                         //L1CacheHandle
    0xFFFF,                                         //L2CacheHandle
    0xFFFF,                                         //L3CacheHandle
    4,                                              //SerialNumber
    5,                                              //AssetTag
    6,                                              //PartNumber
    0,                                              //CoreCount
    0,                                              //EnabledCoreCount
    0,                                              //ThreadCount
    0,                                              //ProcessorCharacteristics
    ProcessorFamilyARMv8,                           //ProcessorFamily2
    0,                                              //CoreCount2
    0,                                              //EnabledCoreCount2
    0                                               //ThreadCount2
  }
};


UINT16
GetCpuFrequency (
  IN  UINT8 ProcessorNumber
)
{
  return (UINT16)(PlatformGetCpuFreq (ProcessorNumber)/1000/1000);
}

UINTN
GetCacheSocketStr (
  IN  UINT8     CacheLevel,
  OUT CHAR16    *CacheSocketStr
  )
{
  UINTN CacheSocketStrLen;

  if (CacheLevel == CPU_CACHE_L1_Instruction)
  {
    CacheSocketStrLen = UnicodeSPrint (CacheSocketStr, SMBIOS_STRING_MAX_LENGTH - 1, L"L%x Instruction Cache", CacheLevel + 1);
  }
  else if (CacheLevel == CPU_CACHE_L1_Data)
  {
    CacheSocketStrLen = UnicodeSPrint (CacheSocketStr, SMBIOS_STRING_MAX_LENGTH - 1, L"L%x Data Cache", CacheLevel);
  }
  else
  {
    CacheSocketStrLen = UnicodeSPrint (CacheSocketStr, SMBIOS_STRING_MAX_LENGTH - 1, L"L%x Cache", CacheLevel);
  }

  return CacheSocketStrLen;
}

/**
  Add Type 7 SMBIOS Record for Cache Information.

  @param[in]    ProcessorNumber     Processor number of specified processor.
  @param[out]   L1CacheHandle       Pointer to the handle of the L1 Cache SMBIOS record.
  @param[out]   L2CacheHandle       Pointer to the handle of the L2 Cache SMBIOS record.
  @param[out]   L3CacheHandle       Pointer to the handle of the L3 Cache SMBIOS record.

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
  UINTN                       TableSize;
  UINT8                       CacheLevel;
  CHAR8                       *OptionalStrStart;
  EFI_STRING                  CacheSocketStr;
  UINTN                       CacheSocketStrLen;
  UINTN                       StringBufferSize;

  Status = EFI_SUCCESS;

  //
  // Get Cache information
  //
  for (CacheLevel = 0; CacheLevel < MAX_CACHE_LEVEL; CacheLevel++)
  {
    Type7Record = NULL;

    if (mSmbiosCacheTable[CacheLevel].InstalledSize == 0)
    {
      continue;
    }

    //
    // Update Cache information
    //
    if (mSmbiosCacheTable[CacheLevel].MaximumCacheSize == 0)
    {
      OemGetCacheInformation (CacheLevel, &mSmbiosCacheTable[CacheLevel]);
    }

    StringBufferSize = sizeof (CHAR16) * SMBIOS_STRING_MAX_LENGTH;
    CacheSocketStr = AllocateZeroPool (StringBufferSize);
    if (CacheSocketStr == NULL)
    {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    CacheSocketStrLen = GetCacheSocketStr (CacheLevel, CacheSocketStr);

    TableSize = sizeof (SMBIOS_TABLE_TYPE7) + CacheSocketStrLen + 1 + 1;
    Type7Record = AllocateZeroPool (TableSize);
    if (Type7Record == NULL)
    {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    (VOID)CopyMem (Type7Record, &mSmbiosCacheTable[CacheLevel], sizeof (SMBIOS_TABLE_TYPE7));

    OptionalStrStart = (CHAR8 *) (Type7Record + 1);
    UnicodeStrToAsciiStrS (CacheSocketStr, OptionalStrStart, sizeof (OptionalStrStart));

    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    Status = mSmbios->Add (mSmbios, NULL, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *)Type7Record);
    if (EFI_ERROR (Status))
    {
      goto Exit;
    }

    // Config L1/L2/L3 Cache Handle
    switch (CacheLevel)
    {
      case CPU_CACHE_L1_Instruction:
      case CPU_CACHE_L1_Data:
        *L1CacheHandle = SmbiosHandle;
        break;
      case CPU_CACHE_L2:
        *L2CacheHandle = SmbiosHandle;
        break;
      case CPU_CACHE_L3:
        *L3CacheHandle = SmbiosHandle;
        break;
      default :
          break;
      }
Exit:
      if (Type7Record != NULL)
      {
        FreePool (Type7Record);
      }
      if (CacheSocketStr != NULL)
      {
        FreePool (CacheSocketStr);
        CacheSocketStr = NULL;
      }
  }

  return Status;
}

/**
  Add Type 4 SMBIOS Record for Processor Information.

  @param[in]    ProcessorNumber     Processor number of specified processor.

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

  CHAR8                       *OptionalStrStart;
  UINT8                       *LegacyVoltage;
  EFI_STRING_ID               ProcessorManu;
  EFI_STRING_ID               ProcessorVersion;
  EFI_STRING_ID               SerialNumber;
  EFI_STRING_ID               AssetTag;
  EFI_STRING_ID               PartNumber;
  EFI_STRING                  ProcessorSocketStr;
  EFI_STRING                  ProcessorManuStr;
  EFI_STRING                  ProcessorVersionStr;
  EFI_STRING                  SerialNumberStr;
  EFI_STRING                  AssetTagStr;
  EFI_STRING                  PartNumberStr;
  UINTN                       ProcessorSocketStrLen;
  UINTN                       ProcessorManuStrLen;
  UINTN                       ProcessorVersionStrLen;
  UINTN                       SerialNumberStrLen;
  UINTN                       AssetTagStrLen;
  UINTN                       PartNumberStrLen;
  UINTN                       StringBufferSize;
  UINTN                       TotalSize;

  PROCESSOR_STATUS_DATA       ProcessorStatus = {{0}};
  PROCESSOR_CHARACTERISTICS_DATA  ProcessorCharacteristics = {{0}};
  MISC_PROCESSOR_DATA         MiscProcessorData;

  ARM_SMC_ARGS                Args;
  BOOLEAN                     Arm64SocIdSupported = FALSE;
  int                         Jep106Code;
  int                         SocRevision;
  int                         SmcCallStatus;
  UINT64                      *ProcessorId;

  Type4Record         = NULL;
  ProcessorManuStr    = NULL;
  ProcessorVersionStr = NULL;
  SerialNumberStr     = NULL;
  AssetTagStr         = NULL;
  PartNumberStr       = NULL;


  MiscProcessorData.Voltage             = 0;
  MiscProcessorData.CurrentSpeed        = 0;
  MiscProcessorData.CoreCount           = 0;
  MiscProcessorData.CoresEnabled        = 0;
  MiscProcessorData.ThreadCount         = 0;
  L1CacheHandle       = 0xFFFF;
  L2CacheHandle       = 0xFFFF;
  L3CacheHandle       = 0xFFFF;

  ProcessorManu       = STRING_TOKEN (STR_PROCESSOR_UNKNOWN);
  ProcessorVersion    = STRING_TOKEN (STR_PROCESSOR_UNKNOWN);
  SerialNumber        = STRING_TOKEN (STR_PROCESSOR_UNKNOWN);
  AssetTag            = STRING_TOKEN (STR_PROCESSOR_UNKNOWN);
  PartNumber          = STRING_TOKEN (STR_PROCESSOR_UNKNOWN);

  BOOLEAN Populated = OemGetProcessorInformation (ProcessorNumber,
                                                  &ProcessorStatus,
                                                  &ProcessorCharacteristics,
                                                  &MiscProcessorData);
  if (Populated)
  {
    Status = AddSmbiosCacheTypeTable (ProcessorNumber, &L1CacheHandle,
                                      &L2CacheHandle, &L3CacheHandle);

    ProcessorManu       = STRING_TOKEN (STR_PROCESSOR_MANUFACTURE);
    ProcessorVersion    = STRING_TOKEN (STR_PROCESSOR_VERSION);
    SerialNumber        = STRING_TOKEN (STR_PROCESSOR_SERIAL_NUMBER);
    AssetTag            = STRING_TOKEN (STR_PROCESSOR_ASSET_TAG);
    PartNumber          = STRING_TOKEN (STR_PROCESSOR_PART_NUMBER);

    ProcessorManuStr = (CHAR16 *) PcdGetPtr (PcdProcessorManufacturer);

    if (StrLen (ProcessorManuStr) > 0)
    {
      HiiSetString (mHiiHandle, ProcessorManu, ProcessorManuStr, NULL);
    }

    ProcessorVersionStr = (CHAR16 *) PcdGetPtr (PcdProcessorVersion);

    if (StrLen (ProcessorVersionStr) > 0)
    {
      HiiSetString (mHiiHandle, ProcessorVersion, ProcessorVersionStr, NULL);
    }

    SerialNumberStr = (CHAR16 *) PcdGetPtr (PcdProcessorSerialNumber);

    if (StrLen (SerialNumberStr) > 0)
    {
      HiiSetString (mHiiHandle, SerialNumber, SerialNumberStr, NULL);
    }

    AssetTagStr = (CHAR16 *) PcdGetPtr (PcdProcessorAssetTag);

    if (StrLen (AssetTagStr) > 0)
    {
      HiiSetString (mHiiHandle, AssetTag, AssetTagStr, NULL);
    }

    PartNumberStr = (CHAR16 *) PcdGetPtr (PcdProcessorPartNumber);

    if (StrLen (PartNumberStr) > 0)
    {
      HiiSetString (mHiiHandle, PartNumber, PartNumberStr, NULL);
    }
  }

  // Processor Socket Designation
  StringBufferSize = sizeof (CHAR16) * SMBIOS_STRING_MAX_LENGTH;
  ProcessorSocketStr = AllocateZeroPool (StringBufferSize);
  if (ProcessorSocketStr == NULL)
  {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  ProcessorSocketStrLen = UnicodeSPrint (ProcessorSocketStr, StringBufferSize, L"CPU%02d", ProcessorNumber + 1);

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
  Type4Record = AllocateZeroPool (TotalSize);
  if (Type4Record == NULL)
  {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
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
  Type4Record->ProcessorCharacteristics   = ProcessorCharacteristics.Data;

  Type4Record->ExternalClock              = (UINT16)(ArmReadCntFrq () / 1000 / 1000);

  ProcessorId = (UINT64 *)&(Type4Record->ProcessorId);

  Args.Arg0 = ARM_SMC_ID_ARCH_VERSION;
  ArmCallSmc (&Args);
  SmcCallStatus = (int)Args.Arg0;

  if (SmcCallStatus < 0 || (SmcCallStatus >> 16) >= 1) {
    Args.Arg0 = ARM_SMC_ID_ARCH_FEATURES;
    Args.Arg1 = ARM_SMC_ID_ARCH_SOC_ID;
    ArmCallSmc (&Args);

    if (Args.Arg0 >= 0) {
      Args.Arg0 = ARM_SMC_ID_ARCH_SOC_ID;
      Args.Arg1 = 0;
      ArmCallSmc (&Args);
      SmcCallStatus = (int)Args.Arg0;

      if (SmcCallStatus >= 0) {
        Arm64SocIdSupported = TRUE;
        ProcessorCharacteristics.Bits.Arm64SocId = 1;
        Jep106Code = (int)Args.Arg0;
      } else {
        ProcessorCharacteristics.Bits.Arm64SocId = 0;
      }
      Args.Arg0 = ARM_SMC_ID_ARCH_SOC_ID;
      Args.Arg1 = 1;
      ArmCallSmc (&Args);
      SmcCallStatus = (int)Args.Arg0;

      if (SmcCallStatus >= 0) {
        SocRevision = (int)Args.Arg0;
      }
    }
  }

  if (Arm64SocIdSupported) {
    *ProcessorId = ((UINT64)Jep106Code << 32) | SocRevision;
  } else {
    *ProcessorId = ArmReadMidr ();
  }

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

  OptionalStrStart = (CHAR8 *) (Type4Record + 1);
  UnicodeStrToAsciiStrS (ProcessorSocketStr, OptionalStrStart, ProcessorSocketStrLen + 1);
  UnicodeStrToAsciiStrS (ProcessorManuStr, OptionalStrStart + ProcessorSocketStrLen + 1, ProcessorManuStrLen + 1);
  UnicodeStrToAsciiStrS (ProcessorVersionStr, OptionalStrStart + ProcessorSocketStrLen + 1 + ProcessorManuStrLen + 1, ProcessorVersionStrLen + 1);
  UnicodeStrToAsciiStrS (SerialNumberStr, OptionalStrStart + ProcessorSocketStrLen + 1 + ProcessorManuStrLen + 1 + ProcessorVersionStrLen + 1, SerialNumberStrLen + 1);
  UnicodeStrToAsciiStrS (AssetTagStr, OptionalStrStart + ProcessorSocketStrLen + 1 + ProcessorManuStrLen + 1 + ProcessorVersionStrLen + 1 + SerialNumberStrLen + 1, AssetTagStrLen + 1);
  UnicodeStrToAsciiStrS (PartNumberStr, OptionalStrStart + ProcessorSocketStrLen + 1 + ProcessorManuStrLen + 1 + ProcessorVersionStrLen + 1 + SerialNumberStrLen + 1 + AssetTagStrLen + 1, PartNumberStrLen + 1);

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = mSmbios->Add (mSmbios, NULL, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *)Type4Record);
  if (EFI_ERROR (Status))
  {
    DEBUG ((DEBUG_ERROR, "[%a]:[%dL] Smbios Type04 Table Log Failed! %r \n", __FUNCTION__, __LINE__, Status));
  }
  FreePool (Type4Record);

Exit:
  if (ProcessorSocketStr != NULL)
  {
    FreePool (ProcessorSocketStr);
  }
  if (ProcessorManuStr != NULL)
  {
    FreePool (ProcessorManuStr);
  }
  if (ProcessorVersionStr != NULL)
  {
    FreePool (ProcessorVersionStr);
  }
  if (SerialNumberStr != NULL)
  {
    FreePool (SerialNumberStr);
  }
  if (AssetTagStr != NULL)
  {
    FreePool (AssetTagStr);
  }
  if (PartNumberStr != NULL)
  {
    FreePool (PartNumberStr);
  }

  return Status;
}

/**
  Standard EFI driver point.  This driver locates the ProcessorConfigurationData Variable,
  if it exists, add the related SMBIOS tables by PI SMBIOS protocol.

  @param  ImageHandle     Handle for the image of this driver
  @param  SystemTable     Pointer to the EFI System Table

  @retval  EFI_SUCCESS    The data was successfully stored.

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
  for (SocketIndex = 0; SocketIndex < OemGetProcessorMaxSockets(); SocketIndex++) {
    Status = AddSmbiosProcessorTypeTable (SocketIndex);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Add Processor Type Table Failed!  %r.\n", Status));
      return Status;
    }
  }

  return Status;
}
