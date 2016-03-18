/** @file
This is the driver that locates the MemoryConfigurationData Variable, if it
exists, and reports the data to the DataHub.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MemorySubClass.h"

extern UINT8 MemorySubClassStrings[];

EFI_GUID  gEfiMemorySubClassDriverGuid = EFI_MEMORY_SUBCLASS_DRIVER_GUID;

EFI_STATUS
MemorySubClassEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
/*++

  Routine Description:
    This is the standard EFI driver point that detects whether there is a
    MemoryConfigurationData Variable and, if so, reports memory configuration info
    to the DataHub.

  Arguments:
    ImageHandle   - Handle for the image of this driver
    SystemTable   - Pointer to the EFI System Table

  Returns:
    EFI_SUCCESS if the data is successfully reported
    EFI_NOT_FOUND if the HOB list could not be located.

--*/
{
//  UINT8                           Index;
  UINTN                           DataSize;
  UINT8                           Dimm;
  UINTN                           StringBufferSize;
  UINT8                           NumSlots;
  UINTN                           DevLocStrLen;
  UINTN                           BankLocStrLen;
  UINTN                           ManuStrLen;
  UINTN                           SerialNumStrLen;
  UINTN                           AssertTagStrLen;
  UINTN                           PartNumStrLen;
  UINTN                           MemoryDeviceSize;
  CHAR8*                          OptionalStrStart;
  UINT16                          ArrayInstance;
  UINT64                          DimmMemorySize;
  UINT64                          TotalMemorySize;
  UINT32                          Data;
  UINT32                          MemoryCapacity;
  BOOLEAN                         MemoryDeviceSizeUnitMega;
  EFI_STATUS                      Status;
  EFI_STRING                      StringBuffer;
  EFI_STRING                      DevLocStr;
  EFI_STRING                      BankLocStr;
  EFI_STRING                      ManuStr;
  EFI_STRING                      SerialNumStr;
  EFI_STRING                      AssertTagStr;
  EFI_STRING                      PartNumStr;
  EFI_HII_HANDLE                  HiiHandle;
  EFI_SMBIOS_HANDLE               MemArraySmbiosHandle;
  EFI_SMBIOS_HANDLE               MemArrayMappedAddrSmbiosHandle;
  EFI_SMBIOS_HANDLE               MemDevSmbiosHandle;
  EFI_SMBIOS_HANDLE               MemDevMappedAddrSmbiosHandle;
  EFI_SMBIOS_HANDLE               MemModuleInfoSmbiosHandle;
  SMBIOS_TABLE_TYPE6              *Type6Record;
  SMBIOS_TABLE_TYPE16             *Type16Record;
  SMBIOS_TABLE_TYPE17             *Type17Record;
  SMBIOS_TABLE_TYPE19              *Type19Record;
  SMBIOS_TABLE_TYPE20             *Type20Record;
  EFI_SMBIOS_PROTOCOL             *Smbios;
  EFI_MEMORY_ARRAY_LINK_DATA      ArrayLink;
  EFI_MEMORY_ARRAY_LOCATION_DATA  ArrayLocationData;
  EFI_MEMORY_DEVICE_START_ADDRESS_DATA  DeviceStartAddress;


  DataSize = 0;
  Dimm = 0;


  //
  // Allocate Buffers
  //
  StringBufferSize = (sizeof (CHAR16)) * 100;
  StringBuffer = AllocateZeroPool (StringBufferSize);
  ASSERT (StringBuffer != NULL);

  //
  // Locate dependent protocols
  //
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID**)&Smbios);
  ASSERT_EFI_ERROR (Status);


  //
  // Add our default strings to the HII database. They will be modified later.
  //
  HiiHandle = HiiAddPackages (
                &gEfiMemorySubClassDriverGuid,
                NULL,
                MemorySubClassStrings,
                NULL
                );
  ASSERT (HiiHandle != NULL);

  //
  // Create physical array and associated data for all mainboard memory
  // This will translate into a Type 16 SMBIOS Record
  //
  ArrayInstance = 1;

  McD0PciCfg32 (QNC_ACCESS_PORT_MCR) = MESSAGE_READ_DW (0x3, 0x8);
  TotalMemorySize =     McD0PciCfg32 (QNC_ACCESS_PORT_MDR);

  ArrayLocationData.MemoryArrayLocation = EfiMemoryArrayLocationSystemBoard;
  ArrayLocationData.MemoryArrayUse = EfiMemoryArrayUseSystemMemory;

  ArrayLocationData.MemoryErrorCorrection = EfiMemoryErrorCorrectionNone;

  Data = 0x40000000;//(UINT32) RShiftU64(MemConfigData->RowInfo.MaxMemory, 10);

  ArrayLocationData.MaximumMemoryCapacity.Exponent = (UINT16) LowBitSet32 (Data);
  ArrayLocationData.MaximumMemoryCapacity.Value    = (UINT16) (Data >> ArrayLocationData.MaximumMemoryCapacity.Exponent);

  NumSlots = 2;// (UINT8)(MemConfigData->RowInfo.MaxRows >> 1);
  ArrayLocationData.NumberMemoryDevices = (UINT16)(NumSlots);

  //
  // Report top level physical array to Type 16 SMBIOS Record
  //
  Type16Record = AllocatePool(sizeof(SMBIOS_TABLE_TYPE16) + 1 + 1);
  ZeroMem(Type16Record, sizeof(SMBIOS_TABLE_TYPE16) + 1 + 1);

  Type16Record->Hdr.Type = EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY;
  Type16Record->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE16);
  Type16Record->Hdr.Handle = 0;

  Type16Record->Location = (UINT8)ArrayLocationData.MemoryArrayLocation;

  Type16Record->Use = (UINT8)ArrayLocationData.MemoryArrayUse;

  Type16Record->MemoryErrorCorrection = (UINT8)ArrayLocationData.MemoryErrorCorrection;

  MemoryCapacity = (UINT32) ArrayLocationData.MaximumMemoryCapacity.Value * (1 << ((UINT32) ArrayLocationData.MaximumMemoryCapacity.Exponent - 10));
  Type16Record->MaximumCapacity = MemoryCapacity;

  Type16Record->MemoryErrorInformationHandle = 0xfffe;

  Type16Record->NumberOfMemoryDevices = ArrayLocationData.NumberMemoryDevices;
  //
  // Don't change it. This handle will be referenced by type 17 records
  //
  MemArraySmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (Smbios, NULL, &MemArraySmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*) Type16Record);
  FreePool(Type16Record);
  ASSERT_EFI_ERROR (Status);

  // Do  associated data for each DIMM
  //RowConfArray = &MemConfigData->RowConfArray;

  //
  // Get total memory size for the construction of smbios record type 19
  //
  //TotalMemorySize = 0;// MSG_BUS_READ(0x0208);

  //
  // Generate Memory Array Mapped Address info
  //
  Type19Record = AllocatePool(sizeof (SMBIOS_TABLE_TYPE19));
  ZeroMem(Type19Record, sizeof(SMBIOS_TABLE_TYPE19));
  Type19Record->Hdr.Type = EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS;
  Type19Record->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE19);
  Type19Record->Hdr.Handle = 0;
  Type19Record->StartingAddress = 0;
  Type19Record->EndingAddress = (UINT32)RShiftU64(TotalMemorySize, 10) - 1;
  Type19Record->MemoryArrayHandle = MemArraySmbiosHandle;
  Type19Record->PartitionWidth = (UINT8)(NumSlots);

  //
  // Generate Memory Array Mapped Address info (TYPE 19)
  //
  MemArrayMappedAddrSmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (Smbios, NULL, &MemArrayMappedAddrSmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*) Type19Record);
  FreePool(Type19Record);
  ASSERT_EFI_ERROR (Status);


  // Use SPD data to generate Device Type info
  ZeroMem (&ArrayLink, sizeof (EFI_MEMORY_ARRAY_LINK_DATA));
  ArrayLink.MemoryDeviceLocator = STRING_TOKEN(STR_MEMORY_SUBCLASS_DEVICE_LOCATOR_0);
  ArrayLink.MemoryBankLocator = STRING_TOKEN(STR_MEMORY_SUBCLASS_DEVICE_LOCATOR_0);
  ArrayLink.MemoryAssetTag = STRING_TOKEN(STR_MEMORY_SUBCLASS_UNKNOWN);
  ArrayLink.MemoryArrayLink.ProducerName = gEfiMemorySubClassDriverGuid;
  ArrayLink.MemoryArrayLink.Instance = ArrayInstance;
  ArrayLink.MemoryArrayLink.SubInstance = EFI_SUBCLASS_INSTANCE_NON_APPLICABLE;
  ArrayLink.MemorySubArrayLink.ProducerName = gEfiMemorySubClassDriverGuid;
  ArrayLink.MemorySubArrayLink.SubInstance = EFI_SUBCLASS_INSTANCE_NON_APPLICABLE;
  ArrayLink.MemoryFormFactor = EfiMemoryFormFactorChip;
  ArrayLink.MemoryType = EfiMemoryTypeDdr2;


  StrCpy (StringBuffer, L"NO DIMM,MEMROY DOWN");
  ArrayLink.MemoryManufacturer = HiiSetString (
                                   HiiHandle,
                                   0,
                                   StringBuffer,
                                   NULL
                                   );
  ArrayLink.MemorySerialNumber = HiiSetString (
                                   HiiHandle,
                                   0,
                                   StringBuffer,
                                   NULL
                                   );

  ArrayLink.MemoryPartNumber = HiiSetString (
                                 HiiHandle,
                                 0,
                                 StringBuffer,
                                 NULL
                                 );

  //
  // Hardcode value. Need to revise for different configuration.
  //
  ArrayLink.MemoryTotalWidth = 64;
  ArrayLink.MemoryDataWidth = 64;

  DimmMemorySize = TotalMemorySize;// MSG_BUS_READ(0x0208);

  ArrayLink.MemoryDeviceSize.Exponent = (UINT16) LowBitSet64 (DimmMemorySize);
  ArrayLink.MemoryDeviceSize.Value    = (UINT16) RShiftU64(DimmMemorySize, ArrayLink.MemoryDeviceSize.Exponent);
  ArrayLink.MemoryTypeDetail.Synchronous  = 1;
  Data = 800;
  ArrayLink.MemorySpeed = *((EFI_EXP_BASE10_DATA *) &Data);



  DevLocStr = HiiGetPackageString(&gEfiMemorySubClassDriverGuid, ArrayLink.MemoryDeviceLocator, NULL);
  DevLocStrLen = StrLen(DevLocStr);
  ASSERT(DevLocStrLen <= SMBIOS_STRING_MAX_LENGTH);

  BankLocStr = HiiGetPackageString(&gEfiMemorySubClassDriverGuid, ArrayLink.MemoryBankLocator, NULL);
  BankLocStrLen = StrLen(BankLocStr);
  ASSERT(BankLocStrLen <= SMBIOS_STRING_MAX_LENGTH);

  ManuStr = HiiGetPackageString(&gEfiMemorySubClassDriverGuid, ArrayLink.MemoryManufacturer, NULL);
  ManuStrLen = StrLen(ManuStr);
  ASSERT(ManuStrLen <= SMBIOS_STRING_MAX_LENGTH);

  SerialNumStr = HiiGetPackageString(&gEfiMemorySubClassDriverGuid, ArrayLink.MemorySerialNumber, NULL);
  SerialNumStrLen = StrLen(SerialNumStr);
  ASSERT(SerialNumStrLen <= SMBIOS_STRING_MAX_LENGTH);

  AssertTagStr = HiiGetPackageString(&gEfiMemorySubClassDriverGuid, ArrayLink.MemoryAssetTag, NULL);
  AssertTagStrLen = StrLen(AssertTagStr);
  ASSERT(AssertTagStrLen <= SMBIOS_STRING_MAX_LENGTH);

  PartNumStr = HiiGetPackageString(&gEfiMemorySubClassDriverGuid, ArrayLink.MemoryPartNumber, NULL);
  PartNumStrLen = StrLen(PartNumStr);
  ASSERT(PartNumStrLen <= SMBIOS_STRING_MAX_LENGTH);

  //
  // Report DIMM level memory module information to smbios (Type 6)
  //
  DataSize = sizeof(SMBIOS_TABLE_TYPE6) + DevLocStrLen + 1 + 1;
  Type6Record = AllocatePool(DataSize);
  ZeroMem(Type6Record, DataSize);
  Type6Record->Hdr.Type = EFI_SMBIOS_TYPE_MEMORY_MODULE_INFORMATON;
  Type6Record->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE6);
  Type6Record->Hdr.Handle = 0;
  Type6Record->SocketDesignation = 1;
  if (ArrayLink.MemorySpeed.Value == 0) {
    Type6Record->CurrentSpeed = 0;
  } else {
    //
    // Memory speed is in ns unit
    //
    Type6Record->CurrentSpeed = (UINT8)(1000 / (ArrayLink.MemorySpeed.Value));
  }
  //
  // Device Size
  //
  MemoryDeviceSize = (UINTN)(ArrayLink.MemoryDeviceSize.Value) * (UINTN)(1 << ArrayLink.MemoryDeviceSize.Exponent);
  if (MemoryDeviceSize == 0) {
    *(UINT8*)&(Type6Record->InstalledSize) = 0x7F;
    *(UINT8*)&(Type6Record->EnabledSize)   = 0x7F;
  } else {
    MemoryDeviceSize = (UINTN) RShiftU64 ((UINT64) MemoryDeviceSize, 21);
    while (MemoryDeviceSize != 0) {
      (*(UINT8*)&(Type6Record->InstalledSize))++;
      (*(UINT8*)&(Type6Record->EnabledSize))++;
      MemoryDeviceSize = (UINTN) RShiftU64 ((UINT64) MemoryDeviceSize,1);
    }
  }

  if (ArrayLink.MemoryFormFactor == EfiMemoryFormFactorDimm ||
    ArrayLink.MemoryFormFactor == EfiMemoryFormFactorFbDimm) {
    *(UINT16*)&Type6Record->CurrentMemoryType |= 1<<8;
  }
  if (ArrayLink.MemoryFormFactor == EfiMemoryFormFactorSimm) {
    *(UINT16*)&Type6Record->CurrentMemoryType |= 1<<7;
  }
  if (ArrayLink.MemoryType == EfiMemoryTypeSdram) {
    *(UINT16*)&Type6Record->CurrentMemoryType |= 1<<10;
  }
  if (ArrayLink.MemoryTypeDetail.Edo == 1) {
    *(UINT16*)&Type6Record->CurrentMemoryType |= 1<<4;
  }
  if (ArrayLink.MemoryTypeDetail.FastPaged == 1) {
    *(UINT16*)&Type6Record->CurrentMemoryType |= 1<<3;
  }
  OptionalStrStart = (CHAR8 *)(Type6Record + 1);
  UnicodeStrToAsciiStr(DevLocStr, OptionalStrStart);
  MemModuleInfoSmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (Smbios, NULL, &MemModuleInfoSmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*) Type6Record);
  FreePool(Type6Record);
  ASSERT_EFI_ERROR (Status);
  //
  // Report DIMM level Device Type to smbios (Type 17)
  //
  DataSize = sizeof (SMBIOS_TABLE_TYPE17) + DevLocStrLen + 1 + BankLocStrLen + 1 + ManuStrLen + 1 + SerialNumStrLen + 1 + AssertTagStrLen + 1 + PartNumStrLen + 1 + 1;
  Type17Record = AllocatePool(DataSize);
  ZeroMem(Type17Record, DataSize);
  Type17Record->Hdr.Type = EFI_SMBIOS_TYPE_MEMORY_DEVICE;
  Type17Record->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE17);
  Type17Record->Hdr.Handle = 0;

  Type17Record->MemoryArrayHandle = MemArraySmbiosHandle;
  Type17Record->MemoryErrorInformationHandle = 0xfffe;
  Type17Record->TotalWidth = ArrayLink.MemoryTotalWidth;
  Type17Record->DataWidth = ArrayLink.MemoryDataWidth;
  //
  // Device Size
  //
  MemoryDeviceSize          = ((UINTN) ArrayLink.MemoryDeviceSize.Value) << (ArrayLink.MemoryDeviceSize.Exponent - 10);
  MemoryDeviceSizeUnitMega  = FALSE;
  //
  // kilo as unit
  //
  if (MemoryDeviceSize > 0xffff) {
    MemoryDeviceSize = MemoryDeviceSize >> 10;
    //
    // Mega as unit
    //
    MemoryDeviceSizeUnitMega = TRUE;
  }

  MemoryDeviceSize = MemoryDeviceSize & 0x7fff;
  if (MemoryDeviceSize != 0 && MemoryDeviceSizeUnitMega == FALSE) {
    MemoryDeviceSize |= 0x8000;
  }
  Type17Record->Size = (UINT16)MemoryDeviceSize;

  Type17Record->FormFactor = (UINT8)ArrayLink.MemoryFormFactor;
  Type17Record->DeviceLocator = 1;
  Type17Record->BankLocator = 2;
  Type17Record->MemoryType = (UINT8)ArrayLink.MemoryType;
  CopyMem (
    (UINT8 *) &Type17Record->TypeDetail,
    &ArrayLink.MemoryTypeDetail,
    2
  );

  Type17Record->Speed = ArrayLink.MemorySpeed.Value;
  Type17Record->Manufacturer = 3;
  Type17Record->SerialNumber = 4;
  Type17Record->AssetTag = 5;
  Type17Record->PartNumber = 6;
  //
  // temporary solution for save device label information.
  //
  Type17Record->Attributes = (UINT8)(Dimm + 1);

  OptionalStrStart = (CHAR8 *)(Type17Record + 1);
  UnicodeStrToAsciiStr(DevLocStr, OptionalStrStart);
  UnicodeStrToAsciiStr(BankLocStr, OptionalStrStart + DevLocStrLen + 1);
  UnicodeStrToAsciiStr(ManuStr, OptionalStrStart + DevLocStrLen + 1 + BankLocStrLen + 1);
  UnicodeStrToAsciiStr(SerialNumStr, OptionalStrStart + DevLocStrLen + 1 + BankLocStrLen + 1 + ManuStrLen + 1);
  UnicodeStrToAsciiStr(AssertTagStr, OptionalStrStart + DevLocStrLen + 1 + BankLocStrLen + 1 + ManuStrLen + 1 + SerialNumStrLen + 1);
  UnicodeStrToAsciiStr(PartNumStr, OptionalStrStart + DevLocStrLen + 1 + BankLocStrLen + 1 + ManuStrLen + 1 + SerialNumStrLen + 1 + AssertTagStrLen + 1);
  MemDevSmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (Smbios, NULL, &MemDevSmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*) Type17Record);
  FreePool(Type17Record);
  ASSERT_EFI_ERROR (Status);

  //
  // Generate Memory Device Mapped Address info
  //
  ZeroMem(&DeviceStartAddress, sizeof(EFI_MEMORY_DEVICE_START_ADDRESS_DATA));
  DeviceStartAddress.MemoryDeviceStartAddress = 0;
  DeviceStartAddress.MemoryDeviceEndAddress = DeviceStartAddress.MemoryDeviceStartAddress + DimmMemorySize-1;
  DeviceStartAddress.PhysicalMemoryDeviceLink.ProducerName = gEfiMemorySubClassDriverGuid;
  DeviceStartAddress.PhysicalMemoryDeviceLink.Instance = ArrayInstance;
  DeviceStartAddress.PhysicalMemoryDeviceLink.SubInstance = (UINT16)(Dimm + 1);
  DeviceStartAddress.PhysicalMemoryArrayLink.ProducerName = gEfiMemorySubClassDriverGuid;
  DeviceStartAddress.PhysicalMemoryArrayLink.Instance = ArrayInstance;
  DeviceStartAddress.PhysicalMemoryArrayLink.SubInstance = EFI_SUBCLASS_INSTANCE_NON_APPLICABLE;

  //
  // Single channel mode
  //
  DeviceStartAddress.MemoryDevicePartitionRowPosition = 0x01;
  DeviceStartAddress.MemoryDeviceInterleavePosition = 0x00;
  DeviceStartAddress.MemoryDeviceInterleaveDataDepth = 0x00;

  //
  // Generate Memory Device Mapped Address info (TYPE 20)
  //
  Type20Record = AllocatePool(sizeof (SMBIOS_TABLE_TYPE20));
  ZeroMem(Type20Record, sizeof (SMBIOS_TABLE_TYPE20));
  Type20Record->Hdr.Type = EFI_SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS;
  Type20Record->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE20);
  Type20Record->Hdr.Handle = 0;

  Type20Record->StartingAddress = (UINT32)RShiftU64 (DeviceStartAddress.MemoryDeviceStartAddress, 10);
  Type20Record->EndingAddress = (UINT32)RShiftU64 (DeviceStartAddress.MemoryDeviceEndAddress, 10);
  Type20Record->MemoryDeviceHandle = MemDevSmbiosHandle;
  Type20Record->MemoryArrayMappedAddressHandle = MemArrayMappedAddrSmbiosHandle;
  Type20Record->PartitionRowPosition = DeviceStartAddress.MemoryDevicePartitionRowPosition;
  Type20Record->InterleavePosition = DeviceStartAddress.MemoryDeviceInterleavePosition;
  Type20Record->InterleavedDataDepth = DeviceStartAddress.MemoryDeviceInterleaveDataDepth;
  MemDevMappedAddrSmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (Smbios, NULL, &MemDevMappedAddrSmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*) Type20Record);
  FreePool(Type20Record);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
