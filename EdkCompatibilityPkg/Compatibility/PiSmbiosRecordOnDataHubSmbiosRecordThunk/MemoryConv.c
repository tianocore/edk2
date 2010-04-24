/** @file
  Routines that support Memory SubClass data records translation.
  
Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Thunk.h"

/**
  Field Filling Function for Memory SubClass record type 2 -- Physical Memory
  Array.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType2 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                            Status;
  EFI_MEMORY_ARRAY_LOCATION_DATA        *PhyMemArray;
  FRAMEWORK_MEMORY_ARRAY_LOCATION_DATA  *FrameworkPhyMemArray;
  UINT32                                MemoryCapacity;
  UINT16                                NumberMemoryDevices;
  UINT16                                Test16;

  Status      = EFI_SUCCESS;
  PhyMemArray = (EFI_MEMORY_ARRAY_LOCATION_DATA *) RecordData;
  FrameworkPhyMemArray = (FRAMEWORK_MEMORY_ARRAY_LOCATION_DATA *) RecordData;

  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE16, Location))  = (UINT8) (PhyMemArray->MemoryArrayLocation);
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE16, Use))  = (UINT8) (PhyMemArray->MemoryArrayUse);
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE16, MemoryErrorCorrection))  = (UINT8) (PhyMemArray->MemoryErrorCorrection);

  if (!FeaturePcdGet(PcdFrameworkCompatibilitySupport)) {
    MemoryCapacity = (UINT32) (((UINTN) PhyMemArray->MaximumMemoryCapacity.Value) << PhyMemArray->MaximumMemoryCapacity.Exponent);
    NumberMemoryDevices = PhyMemArray->NumberMemoryDevices;
  } else {
    //
    // Support EDk/Framework defined Data strucutre.
    //
    MemoryCapacity      = FrameworkPhyMemArray->MaximumMemoryCapacity;
    NumberMemoryDevices = FrameworkPhyMemArray->NumberMemoryDevices;
  }

  CopyMem (
    (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE16, MaximumCapacity),
    &MemoryCapacity,
    4
    );

  Test16 = 0xfffe;
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE16, MemoryErrorInformationHandle),
    &Test16,
    2
    );
  
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE16, NumberOfMemoryDevices),
    &NumberMemoryDevices,
    2
    );

  return Status;
}

/**
  Field Filling Function for Memory SubClass record type 3 -
  - Memory Device: SMBIOS Type 17
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType3 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MEMORY_ARRAY_LINK_DATA       *MemDevice;
  FRAMEWORK_MEMORY_ARRAY_LINK_DATA *FrameworkMemDevice;
  UINT64                           MemoryDeviceSize;
  BOOLEAN                          MemoryDeviceSizeUnitMega;
  UINT16                           MemoryDeviceSpeed;
  UINT16                           MemoryDeviceExponent;
  UINT16                           Test16;

  MemDevice = (EFI_MEMORY_ARRAY_LINK_DATA *) RecordData;
  FrameworkMemDevice = (FRAMEWORK_MEMORY_ARRAY_LINK_DATA *) RecordData;
  MemoryDeviceSpeed  = 0;
  MemoryDeviceExponent = 0;

  //
  // Memory Device Locator
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE17, DeviceLocator),
    &(MemDevice->MemoryDeviceLocator),
    sizeof (STRING_REF)
    );

  //
  // Memory Bank Locator
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE17, BankLocator),
    &(MemDevice->MemoryBankLocator),
    sizeof (STRING_REF)
    );

  //
  // Memory Manufacturer
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE17, Manufacturer),
    &(MemDevice->MemoryManufacturer),
    sizeof (STRING_REF)
    );

  //
  // Memory Serial Number
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE17, SerialNumber),
    &(MemDevice->MemorySerialNumber),
    sizeof (STRING_REF)
    );

  //
  // Memory Asset Tag
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE17, AssetTag),
    &(MemDevice->MemoryAssetTag),
    sizeof (STRING_REF)
    );

  //
  // Memory Part Number
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE17, PartNumber),
    &(MemDevice->MemoryPartNumber),
    sizeof (STRING_REF)
    );

  //
  // Memory Array Link
  //
  SmbiosFldInterLink (
    StructureNode,
    (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE17, MemoryArrayHandle),
    16, // SMBIOS type 16
    &MemDevice->MemoryArrayLink,
    &gEfiMemorySubClassGuid
    );

  //
  // Set Memory Error Information Handle to Not supported
  //
  Test16 = 0xfffe;
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE17, MemoryErrorInformationHandle),
    &Test16,
    sizeof (EFI_SMBIOS_HANDLE)
    ); 
     
  //
  // Total width
  //
  CopyMem (
    (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, TotalWidth),
    &MemDevice->MemoryTotalWidth,
    2
    );

  //
  // Data Width
  //
  CopyMem (
    (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, DataWidth),
    &MemDevice->MemoryDataWidth,
    2
    );

  //
  // Device Size
  //
  if (!FeaturePcdGet(PcdFrameworkCompatibilitySupport)) {
    //
    // Form Factor
    //
    CopyMem (
      (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, FormFactor),
      &MemDevice->MemoryFormFactor,
      1
      );
  
    //
    // Device Set
    //
    CopyMem (
      (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, DeviceSet),
      &MemDevice->MemoryDeviceSet,
      1
      );
  
    //
    // Type
    //
    CopyMem (
      (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, MemoryType),
      &MemDevice->MemoryType,
      1
      );
  
    //
    // Type Detail
    //
    CopyMem (
      (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, TypeDetail),
      &MemDevice->MemoryTypeDetail,
      2
      );
  
    //
    // Speed
    //
    MemoryDeviceSpeed    = MemDevice->MemorySpeed.Value;
    MemoryDeviceExponent = MemDevice->MemorySpeed.Exponent;
    while (MemoryDeviceExponent-- > 0) {
      MemoryDeviceSpeed = (UINT16) (MemoryDeviceSpeed * 10);
    }

    CopyMem (
      (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, Speed),
      &MemoryDeviceSpeed,
      2
      );

    MemoryDeviceSize = (UINT64) (((UINTN) MemDevice->MemoryDeviceSize.Value) << MemDevice->MemoryDeviceSize.Exponent);
  } else {
    //
    // Form Factor
    //
    CopyMem (
      (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, FormFactor),
      &FrameworkMemDevice->MemoryFormFactor,
      1
      );
  
    //
    // Device Set
    //
    CopyMem (
      (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, DeviceSet),
      &FrameworkMemDevice->MemoryDeviceSet,
      1
      );
  
    //
    // Type
    //
    CopyMem (
      (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, MemoryType),
      &FrameworkMemDevice->MemoryType,
      1
      );
  
    //
    // Type Detail
    //
    CopyMem (
      (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, TypeDetail),
      &FrameworkMemDevice->MemoryTypeDetail,
      2
      );
  
    //
    // Speed
    //
    CopyMem (
      (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, Speed),
      &FrameworkMemDevice->MemorySpeed,
      2
      );

    MemoryDeviceSize = FrameworkMemDevice->MemoryDeviceSize;
  }

  MemoryDeviceSizeUnitMega  = FALSE;
  MemoryDeviceSize          = RShiftU64 (MemoryDeviceSize, 10);
  //
  // kilo as unit
  //
  if (MemoryDeviceSize > 0xffff) {
    MemoryDeviceSize = RShiftU64 (MemoryDeviceSize, 10);
    //
    // Mega as unit
    //
    MemoryDeviceSizeUnitMega = TRUE;
  }

  MemoryDeviceSize = MemoryDeviceSize & 0x7fff;
  if (MemoryDeviceSize != 0 && !MemoryDeviceSizeUnitMega) {
    MemoryDeviceSize |= 0x8000;
  }

  CopyMem (
    (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE17, Size),
    &MemoryDeviceSize,
    2
    );

  return EFI_SUCCESS;
}

/**
  Field Filling Function for Memory SubClass record type 3 -
  - Memory Device: SMBIOS Type 6
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldSMBIOSType6 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MEMORY_ARRAY_LINK_DATA       *MemDevice;
  UINT64                           MemoryDeviceSize;
  UINT8                            MemSize;
  UINT16                           MemType;
  UINT8                            MemSpeed;
  FRAMEWORK_MEMORY_ARRAY_LINK_DATA *FrameworkMemDevice;
  UINT16                           MemoryDeviceSpeed;
  UINT16                           MemoryDeviceExponent;

  MemDevice            = (EFI_MEMORY_ARRAY_LINK_DATA *) RecordData;
  FrameworkMemDevice   = (FRAMEWORK_MEMORY_ARRAY_LINK_DATA *) RecordData;
  MemoryDeviceExponent = 0;

  //
  // Memory Device Locator
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE6, SocketDesignation),
    &(MemDevice->MemoryDeviceLocator),
    2
    );
  
  if (!FeaturePcdGet(PcdFrameworkCompatibilitySupport)) {
    MemoryDeviceSpeed    = MemDevice->MemorySpeed.Value;
    MemoryDeviceExponent = MemDevice->MemorySpeed.Exponent;
    while (MemoryDeviceExponent-- > 0) {
      MemoryDeviceSpeed = (UINT16) (MemoryDeviceSpeed * 10);
    }
    MemoryDeviceSize = (UINT64) (((UINTN) MemDevice->MemoryDeviceSize.Value) << MemDevice->MemoryDeviceSize.Exponent);
  } else {
    //
    // Support EDk/Framework defined Data strucutre.
    //
    MemoryDeviceSpeed = FrameworkMemDevice->MemorySpeed;
    MemoryDeviceSize  = FrameworkMemDevice->MemoryDeviceSize;
  }
  
  if (MemoryDeviceSpeed == 0) {
    MemSpeed = 0;
  } else {
    //
    // Memory speed is in ns unit
    //
    MemSpeed = (UINT8)(1000 / MemoryDeviceSpeed);
  }

  CopyMem (
   (UINT8*)StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE6, CurrentSpeed),
   &MemSpeed,
   1
  );


  //
  // Device Size
  //
  MemSize = 0;
  if (MemoryDeviceSize == 0) {
    MemSize = 0x7F;
  } else {
    MemoryDeviceSize = RShiftU64 (MemoryDeviceSize, 21);
    while (MemoryDeviceSize != 0) {
      MemSize++;
      MemoryDeviceSize = RShiftU64(MemoryDeviceSize,1);
    }
  }

  CopyMem (
    (UINT8*)StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE6, InstalledSize),
    &MemSize,
    1
    );

  CopyMem (
    (UINT8*)StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE6, EnabledSize),
    &MemSize,
    1
    );

  //
  // According SMBIOS SPEC Type 6 definition
  //
  MemType = 0;
  if (!FeaturePcdGet(PcdFrameworkCompatibilitySupport)) {
    if (MemDevice->MemoryFormFactor == EfiMemoryFormFactorDimm ||
        MemDevice->MemoryFormFactor == EfiMemoryFormFactorFbDimm) {
      MemType |= 1<<8;
    }
    
    if (MemDevice->MemoryFormFactor == EfiMemoryFormFactorSimm) {
      MemType |= 1<<7;
    }
    
    if (MemDevice->MemoryType == EfiMemoryTypeSdram) {
      MemType |= 1<<10;
    }
    
    if (MemDevice->MemoryTypeDetail.Edo == 1) {
      MemType |= 1<<4;
    }
    
    if (MemDevice->MemoryTypeDetail.FastPaged == 1) {
      MemType |= 1<<3;
    }
  } else {
    //
    // Support EDk/Framework defined Data strucutre.
    //
    if (FrameworkMemDevice->MemoryFormFactor == EfiMemoryFormFactorDimm ||
        FrameworkMemDevice->MemoryFormFactor == EfiMemoryFormFactorFbDimm) {
      MemType |= 1<<8;
    }
    
    if (FrameworkMemDevice->MemoryFormFactor == EfiMemoryFormFactorSimm) {
      MemType |= 1<<7;
    }
    
    if (FrameworkMemDevice->MemoryType == EfiMemoryTypeSdram) {
      MemType |= 1<<10;
    }
    
    if (FrameworkMemDevice->MemoryTypeDetail.Edo == 1) {
      MemType |= 1<<4;
    }
    
    if (FrameworkMemDevice->MemoryTypeDetail.FastPaged == 1) {
      MemType |= 1<<3;
    }
  }
  //
  // Form Factor
  //
  CopyMem (
    (UINT8*)StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE6, CurrentMemoryType),
    &MemType,
    2
    );


  return EFI_SUCCESS;
}

/**
  Field Filling Function for Memory SubClass record type 4
  -- Memory Array Mapped Address: SMBIOS Type 19
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType4 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MEMORY_ARRAY_START_ADDRESS_DATA  *Masa;
  EFI_PHYSICAL_ADDRESS                 TempData;

  Masa = (EFI_MEMORY_ARRAY_START_ADDRESS_DATA *) RecordData;

  //
  // Starting Address
  //
  TempData = RShiftU64 (Masa->MemoryArrayStartAddress, 10);
  CopyMem (
    (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE19, StartingAddress),
    &TempData,
    4
    );

  //
  // Ending Address
  //
  TempData = RShiftU64 (Masa->MemoryArrayEndAddress, 10);
  CopyMem (
    (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE19, EndingAddress),
    &TempData,
    4
    );

  //
  // Partition Width
  //
  CopyMem (
    (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE19, PartitionWidth),
    &Masa->MemoryArrayPartitionWidth,
    1
    );

  //
  // Physical Memory Array Link
  //
  return SmbiosFldInterLink (
          StructureNode,
          (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE19, MemoryArrayHandle),
          16, // SMBIOS type 16
          &Masa->PhysicalMemoryArrayLink,
          &gEfiMemorySubClassGuid
          );

}

/**
  Field Filling Function for Memory SubClass record type 5
  -- Memory Device Mapped Address: SMBIOS Type 20
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType5 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MEMORY_DEVICE_START_ADDRESS_DATA *Mdsa;
  EFI_PHYSICAL_ADDRESS                 TempData;

  Mdsa = (EFI_MEMORY_DEVICE_START_ADDRESS_DATA *) RecordData;

  //
  // Starting Address
  //
  TempData = RShiftU64 (Mdsa->MemoryDeviceStartAddress, 10);
  CopyMem (
    (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE20, StartingAddress),
    &TempData,
    4
    );

  //
  // Ending Address
  //
  TempData = RShiftU64 (Mdsa->MemoryDeviceEndAddress, 10);
  CopyMem (
    (UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE20, EndingAddress),
    &TempData,
    4
    );

  //
  // Memory Device Link
  //
  SmbiosFldInterLink (
    StructureNode,
    (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE20, MemoryDeviceHandle),
    17, // SMBIOS type 17
    &Mdsa->PhysicalMemoryDeviceLink,
    &gEfiMemorySubClassGuid
    );

  //
  // Memory Array Mapped Address Link
  //
  SmbiosFldInterLink (
    StructureNode,
    (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE20, MemoryArrayMappedAddressHandle),
    19, // SMBIOS type 19
    &Mdsa->PhysicalMemoryArrayLink,
    &gEfiMemorySubClassGuid
    );

  //
  // Memory Device Partition Row Position
  //
  *(UINT8 *) ((UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE20, PartitionRowPosition)) = (UINT8) Mdsa->MemoryDevicePartitionRowPosition;

  //
  // Memory Device Interleave Position
  //
  *(UINT8 *) ((UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE20, InterleavePosition)) = (UINT8) Mdsa->MemoryDeviceInterleavePosition;

  //
  // Memory Device Interleave Data Depth
  //
  *(UINT8 *) ((UINT8 *) StructureNode->Structure + OFFSET_OF (SMBIOS_TABLE_TYPE20, InterleavedDataDepth)) = (UINT8) Mdsa->MemoryDeviceInterleaveDataDepth;

  return EFI_SUCCESS;
}

/**
  Field Filling Function for Memory SubClass record type 6
  -- Memory Channel Type: SMBIOS Type 37
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType6 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MEMORY_CHANNEL_TYPE_DATA  *McTa;
  EFI_STATUS                    Status;

  McTa  = (EFI_MEMORY_CHANNEL_TYPE_DATA *) RecordData;

  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE37, ChannelType))  = (UINT8) (McTa->MemoryChannelType);

  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE37, MaximumChannelLoad))  = (UINT8) (McTa->MemoryChannelMaximumLoad);

  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE37, MemoryDeviceCount))  = (UINT8) (McTa->MemoryChannelDeviceCount);

  //
  // Update the length field
  // Multiple device loads are filled through SmbiosFldMemoryType7
  //
  StructureNode->Structure->Length = (UINT8)(StructureNode->Structure->Length + 
                                       sizeof(MEMORY_DEVICE) * McTa->MemoryChannelDeviceCount);
  Status = SmbiosEnlargeStructureBuffer(
             StructureNode, 
             StructureNode->Structure->Length,
             StructureNode->StructureSize,
             StructureNode->StructureSize + sizeof(MEMORY_DEVICE) * McTa->MemoryChannelDeviceCount
             );
  return Status;
}

/**
  Field Filling Function for Memory SubClass record type 7
  -- Memory Channel Device: SMBIOS Type 37
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType7 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MEMORY_CHANNEL_DEVICE_DATA  *Mcdd;
  UINTN                           DeviceLoadOffset;
  UINTN                           DeviceLoadHandleOffset;

  Mcdd = (EFI_MEMORY_CHANNEL_DEVICE_DATA *) RecordData;

  if (Mcdd->DeviceId < 1) {
    return EFI_INVALID_PARAMETER;
  }

  DeviceLoadOffset        = OFFSET_OF (SMBIOS_TABLE_TYPE37, MemoryDevice[0]) + 3 * (Mcdd->DeviceId - 1);
  DeviceLoadHandleOffset  = OFFSET_OF (SMBIOS_TABLE_TYPE37, MemoryDevice[1]) + 3 * (Mcdd->DeviceId - 1);

  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + DeviceLoadOffset) = (UINT8) (Mcdd->MemoryChannelDeviceLoad);

  //
  // Memory Device Handle Link
  //
  return SmbiosFldInterLink (
          StructureNode,
          (UINT16) DeviceLoadHandleOffset,
          17, // Smbios type 17 -- Physical Memory Device
          &Mcdd->DeviceLink,
          &gEfiMemorySubClassGuid
          );

}

/**
  Field Filling Function for Memory SubClass record type 8
  -- Memory Controller information: SMBIOS Type 5
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType8 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MEMORY_CONTROLLER_INFORMATION_DATA *MemoryController;
  UINT32                                 NewMinimalSize;
  UINT16                                 Count;
  EFI_INTER_LINK_DATA                    *Link; 
  EFI_STATUS                             Status;
  
  NewMinimalSize   = 0;

  //
  // There is an update from EFI_MEMORY_CONTROLLER_INFORMATION to
  // EFI_MEMORY_CONTROLLER_INFORMATION_DATA. Multiple MemoryModuleConfig
  // handles are filled.
  //
  MemoryController = (EFI_MEMORY_CONTROLLER_INFORMATION_DATA *)RecordData;
  
  //
  // ErrorDetectingMethod
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE5, ErrDetectMethod),
    &MemoryController->ErrorDetectingMethod,
    1
    );  

  //
  // ErrorCorrectingCapability
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE5, ErrCorrectCapability),
    &MemoryController->ErrorCorrectingCapability,
    1
    );
    
  //
  // MemorySupportedInterleave
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE5, SupportInterleave),
    &MemoryController->MemorySupportedInterleave,
    1
    );
    
  //
  // MemoryCurrentInterleave
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE5, CurrentInterleave),
    &MemoryController->MemoryCurrentInterleave,
    1
    );  
    
  //
  // MaxMemoryModuleSize
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE5, MaxMemoryModuleSize),
    &MemoryController->MaxMemoryModuleSize,
    1
    );
    
  //
  // MemorySpeedType
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE5, SupportSpeed),
    &MemoryController->MemorySpeedType,
    2
    );                             

  //
  // MemorySupportedType
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE5, SupportMemoryType),
    &MemoryController->MemorySupportedType,
    2
    );   

  //
  // MemoryModuleVoltage
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE5, MemoryModuleVoltage),
    &MemoryController->MemoryModuleVoltage,
    1
    ); 

  //
  // NumberofMemorySlot
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE5, AssociatedMemorySlotNum),
    &MemoryController->NumberofMemorySlot,
    1
    ); 
  
  if (MemoryController->NumberofMemorySlot == 0) {
    //
    // EnabledCorrectingCapability
    //
    CopyMem (
      (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE5, MemoryModuleConfigHandles),
      &MemoryController->EnabledCorrectingCapability,
      1
      );     
  } else {
    //
    // Memory module configuration handles exist
    // we should enlarge smbios entry buffer from minimal size
    //
    NewMinimalSize = (MemoryController->NumberofMemorySlot) * sizeof(UINT16) + StructureNode->StructureSize;
    StructureNode->Structure->Length = (UINT8) (NewMinimalSize - 2);
    Status = SmbiosEnlargeStructureBuffer (StructureNode, StructureNode->Structure->Length, StructureNode->StructureSize, NewMinimalSize);
    ASSERT_EFI_ERROR (Status);
    
    //
    // MemoryModuleConfigHandles
    //
    for (Count = 0, Link = MemoryController->MemoryModuleConfig; 
         Count < MemoryController->NumberofMemorySlot; 
         Count++, Link++) {
      SmbiosFldInterLink (
        StructureNode,
        (UINT16) (OFFSET_OF (SMBIOS_TABLE_TYPE5, MemoryModuleConfigHandles) + Count * sizeof(UINT16)),
        6, // SMBIOS type 6
        Link,
        &gEfiMemorySubClassGuid
        );
    }
         
    //
    // EnabledCorrectingCapability
    //  
    CopyMem (
      (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE5, MemoryModuleConfigHandles) + (MemoryController->NumberofMemorySlot) * sizeof(UINT16),
      &MemoryController->EnabledCorrectingCapability,
      1
      );
  }
  
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Memory SubClass record type 
  -- Memory 32 Bit Error Information: SMBIOS Type 18
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType9 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MEMORY_32BIT_ERROR_INFORMATION *MemoryInfo;
  
  MemoryInfo = (EFI_MEMORY_32BIT_ERROR_INFORMATION *)RecordData;
  
  //
  // MemoryErrorType
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE18, ErrorType),
    &MemoryInfo->MemoryErrorType,
    1
    );
    
  //
  // MemoryErrorGranularity
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE18, ErrorGranularity),
    &MemoryInfo->MemoryErrorGranularity,
    1
    );    

  //
  // MemoryErrorOperation
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE18, ErrorOperation),
    &MemoryInfo->MemoryErrorOperation,
    1
    );    

  //
  // VendorSyndrome
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE18, VendorSyndrome),
    &MemoryInfo->VendorSyndrome,
    4
    );    

  //
  // MemoryArrayErrorAddress
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE18, MemoryArrayErrorAddress),
    &MemoryInfo->MemoryArrayErrorAddress,
    4
    ); 

  //
  // DeviceErrorAddress
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE18, DeviceErrorAddress),
    &MemoryInfo->DeviceErrorAddress,
    4
    ); 

  //
  // DeviceErrorResolution
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE18, ErrorResolution),
    &MemoryInfo->DeviceErrorResolution,
    4
    ); 
    
  return EFI_SUCCESS;    
}

/**
  Field Filling Function for Memory SubClass record type 
  -- Memory 64 Bit Error Information: SMBIOS Type 33
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMemoryType10 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MEMORY_64BIT_ERROR_INFORMATION *MemoryInfo;
  
  MemoryInfo = (EFI_MEMORY_64BIT_ERROR_INFORMATION *)RecordData;
  
  //
  // MemoryErrorType
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE33, ErrorType),
    &MemoryInfo->MemoryErrorType,
    1
    );
    
  //
  // MemoryErrorGranularity
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE33, ErrorGranularity),
    &MemoryInfo->MemoryErrorGranularity,
    1
    );    

  //
  // MemoryErrorOperation
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE33, ErrorOperation),
    &MemoryInfo->MemoryErrorOperation,
    1
    );    

  //
  // VendorSyndrome
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE33, VendorSyndrome),
    &MemoryInfo->VendorSyndrome,
    4
    );    

  //
  // MemoryArrayErrorAddress
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE33, MemoryArrayErrorAddress),
    &MemoryInfo->MemoryArrayErrorAddress,
    8
    ); 

  //
  // DeviceErrorAddress
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE33, DeviceErrorAddress),
    &MemoryInfo->DeviceErrorAddress,
    8
    ); 

  //
  // DeviceErrorResolution
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE33, ErrorResolution),
    &MemoryInfo->DeviceErrorResolution,
    4
    ); 
    
  return EFI_SUCCESS;    
}
