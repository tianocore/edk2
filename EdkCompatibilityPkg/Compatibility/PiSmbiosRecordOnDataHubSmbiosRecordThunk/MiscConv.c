/** @file
  Routines that support Misc SubClass data records translation.
  
Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Thunk.h"

/**
  Field Filling Function for Misc SubClass record type 0 -- Bios Information.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType0 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                 Status;
  EFI_MISC_BIOS_VENDOR_DATA  *BiosInfo;

  Status    = EFI_SUCCESS;
  BiosInfo  = NULL;

  BiosInfo  = (EFI_MISC_BIOS_VENDOR_DATA *) RecordData;

  //
  // Bios Vendor
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE0, Vendor),
    &(BiosInfo->BiosVendor),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // Bios Version
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE0, BiosVersion),
    &(BiosInfo->BiosVersion),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // Bios Release Date
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE0, BiosReleaseDate),
    &(BiosInfo->BiosReleaseDate),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // Bios Starting Address Segment
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE0, BiosSegment),
    &BiosInfo->BiosStartingAddress,
    2
    );

  //
  // Bios Physical device size
  //
  SmbiosFldBase2ToByteWith64K (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE0, BiosSize),
    &BiosInfo->BiosPhysicalDeviceSize,
    sizeof (EFI_EXP_BASE2_DATA)
    );
  (*(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE0, BiosSize)))--;

  //
  // Bios Characteristics
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE0, BiosCharacteristics),
    &BiosInfo->BiosCharacteristics1,
    4
    );
  
  //
  // Bios Characteristics higher four bytes
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE0, BiosCharacteristics) + 4,
    &BiosInfo->BiosCharacteristics2,
    4
    ); 

  //
  // Bios Characteristics Extension1/2
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE0, BIOSCharacteristicsExtensionBytes),
    (UINT8 *) &BiosInfo->BiosCharacteristics1 + 4,
    2
    );

  //
  // System BIOS Major Release
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE0, SystemBiosMajorRelease),
    (UINT8 *) &BiosInfo->BiosMajorRelease,
    1
    );

  //
  // System BIOS Minor Release
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE0, SystemBiosMinorRelease),
    (UINT8 *) &BiosInfo->BiosMinorRelease,
    1
    );

  //
  // Embedded Controller Firmware Major Release
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE0, EmbeddedControllerFirmwareMajorRelease),
    (UINT8 *) &BiosInfo->BiosEmbeddedFirmwareMajorRelease,
    1
    );

  //
  // Embedded Controller Firmware Minor Release
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE0, EmbeddedControllerFirmwareMinorRelease),
    (UINT8 *) &BiosInfo->BiosEmbeddedFirmwareMinorRelease,
    1
    );

  return Status;
}

/**
  Field Filling Function for Misc SubClass record type 1 -- System Information.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType1 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                         Status;
  EFI_MISC_SYSTEM_MANUFACTURER_DATA  *SystemInfo;

  Status      = EFI_SUCCESS;
  SystemInfo  = NULL;

  SystemInfo  = (EFI_MISC_SYSTEM_MANUFACTURER_DATA *) RecordData;

  //
  // System Manufacturer
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE1, Manufacturer),
    &(SystemInfo->SystemManufacturer),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // System Product Name
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE1, ProductName),
    &(SystemInfo->SystemProductName),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // System Version
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE1, Version),
    &(SystemInfo->SystemVersion),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // System Serial Number
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE1, SerialNumber),
    &(SystemInfo->SystemSerialNumber),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // Uuid
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE1, Uuid),
    &SystemInfo->SystemUuid,
    16
    );

  //
  // Wakeup Type
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE1, WakeUpType),
    &SystemInfo->SystemWakeupType,
    1
    );

  //
  // System SKU Number
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE1, SKUNumber),
    &(SystemInfo->SystemSKUNumber),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // System Family
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE1, Family),
    &(SystemInfo->SystemFamily),
    2 // 64 * sizeof(CHAR16)
    );

  return Status;
}

/**
  Field Filling Function for record type 2 -- Base Board Manufacture.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType2 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                             Status;
  EFI_MISC_BASE_BOARD_MANUFACTURER_DATA  *Bbm;
  Status  = EFI_SUCCESS;
  Bbm     = (EFI_MISC_BASE_BOARD_MANUFACTURER_DATA *) RecordData;

  //
  // Manufacturer
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE2, Manufacturer),
    &(Bbm->BaseBoardManufacturer),
    2
    );

  //
  // Product
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE2, ProductName),
    &(Bbm->BaseBoardProductName),
    2
    );

  //
  // Version
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE2, Version),
    &(Bbm->BaseBoardVersion),
    2
    );

  //
  // Serial Number
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE2, SerialNumber),
    &(Bbm->BaseBoardSerialNumber),
    2
    );

  //
  // Asset Tag
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE2, AssetTag),
    &(Bbm->BaseBoardAssetTag),
    2
    );

  //
  // Location in Chassis
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE2, LocationInChassis),
    &(Bbm->BaseBoardChassisLocation),
    2
    );

  //
  // Feature Flags
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE2, FeatureFlag),
    &Bbm->BaseBoardFeatureFlags,
    1
    );

  //
  // Board Type
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE2, BoardType)) = (UINT8) Bbm->BaseBoardType;

  //
  // Chassis Handle
  //
  SmbiosFldInterLink (
    StructureNode,
    (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE2, ChassisHandle),
    3,  // SMBIOS type 3 - System Enclosure or Chassis
    &Bbm->BaseBoardChassisLink,
    &gEfiMiscSubClassGuid
    );

  //
  // Number of Contained Object Handles
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE2, NumberOfContainedObjectHandles)) = (UINT8) Bbm->BaseBoardNumberLinks;

  return Status;
}

/**
  Field Filling Function for Misc SubClass record type 3 -
  - System Enclosure or Chassis.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType3 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                         Status;
  EFI_MISC_CHASSIS_MANUFACTURER_DATA *Ec;
  EFI_MISC_ELEMENTS                  *Element;
  UINT16                             Index;
  UINT8                              ContainedElementType;
  Status  = EFI_SUCCESS;
  Ec      = (EFI_MISC_CHASSIS_MANUFACTURER_DATA *) RecordData;

  //
  // Chassis Type
  //
  *(UINT8*)((UINT8 *) (StructureNode->Structure) + 
            OFFSET_OF (SMBIOS_TABLE_TYPE3, Type)) 
            = (UINT8) (Ec->ChassisType.ChassisType | Ec->ChassisType.ChassisLockPresent << 7);


  //
  // Chassis Bootup State
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE3, BootupState),
    &Ec->ChassisBootupState,
    1
    );

  //
  // Chassis Power Supply State
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE3, PowerSupplyState),
    &Ec->ChassisPowerSupplyState,
    1
    );

  //
  // Chassis Thermal State
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE3, ThermalState),
    &Ec->ChassisThermalState,
    1
    );

  //
  // Chassis Security State
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE3, SecurityStatus),
    &Ec->ChassisSecurityState,
    1
    );

  //
  // Chassis Oem Defined
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE3, OemDefined),
    &Ec->ChassisOemDefined,
    4
    );

  //
  // Chassis Height
  //
  *(UINT8*)((UINT8*)(StructureNode->Structure) + 
            OFFSET_OF (SMBIOS_TABLE_TYPE3, Height)) 
            = (UINT8)Ec->ChassisHeight;

  //
  // Chassis Number Power Cords
  //
  *(UINT8*)((UINT8*)(StructureNode->Structure) + 
            OFFSET_OF (SMBIOS_TABLE_TYPE3, NumberofPowerCords)) 
            = (UINT8)Ec->ChassisNumberPowerCords;
  
  //
  // Chassis Element Count
  //
  *(UINT8*)((UINT8*)(StructureNode->Structure) + 
            OFFSET_OF (SMBIOS_TABLE_TYPE3, ContainedElementCount)) 
            = (UINT8)Ec->ChassisElementCount;

  if(Ec->ChassisElementCount > 0) {
    //
    // Element Record Length
    // Current solution covers first 3 bytes; user can extend to meet its requirements.
    //
    *(UINT8*)((UINT8*)(StructureNode->Structure) + 
              OFFSET_OF (SMBIOS_TABLE_TYPE3, ContainedElementRecordLength)) 
              = (UINT8)sizeof(CONTAINED_ELEMENT);

    //
    // Update the structure's length and StructureSize
    //
    StructureNode->Structure->Length = (UINT8)(StructureNode->Structure->Length + 
                                               Ec->ChassisElementCount * sizeof(CONTAINED_ELEMENT));
    Status = SmbiosEnlargeStructureBuffer (
               StructureNode,
               StructureNode->Structure->Length,
               StructureNode->StructureSize,
               StructureNode->StructureSize + Ec->ChassisElementCount * sizeof(CONTAINED_ELEMENT)
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    
    //
    // Contained Elements
    //
    for (Index=0, Element = &Ec->ChassisElements; 
         Index < Ec->ChassisElementCount; 
         Index += 1, Element ++) {

      //
      // ContainedElementType
      //
      ContainedElementType = (UINT8)((Element->ChassisElementType.RecordType == 1)
                                      ? (UINT8)(Element->ChassisElementType.RecordType << 7 | Element->ChassisElementType.Type)
                                      : (UINT8)(Element->ChassisBaseBoard));
      *(UINT8*)((UINT8*)(StructureNode->Structure) + 
                OFFSET_OF (SMBIOS_TABLE_TYPE3, ContainedElements) + 
                Index * sizeof(CONTAINED_ELEMENT) + 
                OFFSET_OF(CONTAINED_ELEMENT,ContainedElementType)) 
                = ContainedElementType;

      //
      // ContainedElementMinimum
      //
      *(UINT8*)((UINT8*)(StructureNode->Structure) + 
                OFFSET_OF (SMBIOS_TABLE_TYPE3, ContainedElements) + 
                Index * sizeof(CONTAINED_ELEMENT) + 
                OFFSET_OF(CONTAINED_ELEMENT,ContainedElementMinimum))
                = (UINT8)Element->ChassisElementMinimum;

      //
      // ContainedElementMaximum
      //
      *(UINT8*)((UINT8*)(StructureNode->Structure) + 
                OFFSET_OF (SMBIOS_TABLE_TYPE3, ContainedElements) + 
                Index * sizeof(CONTAINED_ELEMENT) + 
                OFFSET_OF(CONTAINED_ELEMENT,ContainedElementMaximum)) 
                = (UINT8)Element->ChassisElementMaximum;
    }
  }

  //
  // Move the filling of following four String fields after Contained Elements 
  // because they would break SMBIOS table.
  // Chassis Manufacturer
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE3, Manufacturer),
    &(Ec->ChassisManufacturer),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // Chassis Version
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE3, Version),
    &(Ec->ChassisVersion),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // Chassis Serial Number
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE3, SerialNumber),
    &(Ec->ChassisSerialNumber),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // Chassis Asset Tag
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE3, AssetTag),
    &(Ec->ChassisAssetTag),
    2 // 64 * sizeof(CHAR16)
    );
  
  return Status;
}

/**
  Field Filling Function for Misc SubClass record type 8 -- Port Connector.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType8 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                                       Status;
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *Picd;

  Status  = EFI_SUCCESS;
  Picd    = (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR_DATA *) RecordData;

  //
  // Internal Connector Designator
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE8, InternalReferenceDesignator),
    &(Picd->PortInternalConnectorDesignator),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // Internal Connector Type
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE8, InternalConnectorType)) = (UINT8) Picd->PortInternalConnectorType;

  //
  // External Connector Designator
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE8, ExternalReferenceDesignator),
    &(Picd->PortExternalConnectorDesignator),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // Internal Connector Type
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE8, ExternalConnectorType)) = (UINT8) Picd->PortExternalConnectorType;

  //
  // Internal Connector Type
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE8, PortType)) = (UINT8) Picd->PortType;

  return Status;
}

/**
  Field Filling Function for Misc SubClass record type 9 -- System slot.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType9 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                             Status;
  EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA  *Slot;

  Status  = EFI_SUCCESS;
  Slot    = (EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA *) RecordData;

  //
  // Slot Designation
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE9, SlotDesignation),
    &(Slot->SlotDesignation),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // Slot Type
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE9, SlotType),
    &Slot->SlotType,
    1
    );

  //
  // Slot Data Bus Width
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE9, SlotDataBusWidth),
    &Slot->SlotDataBusWidth,
    1
    );

  //
  // Slot Usage
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE9, CurrentUsage),
    &Slot->SlotUsage,
    1
    );

  //
  // Slot Length
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE9, SlotLength),
    &Slot->SlotLength,
    1
    );

  //
  // Slot Id
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE9, SlotID),
    &Slot->SlotId,
    2
    );

  //
  // Slot Characteristics
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE9, SlotCharacteristics1),
    &Slot->SlotCharacteristics,
    2
    );

  return Status;
}

/**
  Field Filling Function for Misc SubClass record type 10 - Onboard Device.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType10 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                   Status;
  EFI_MISC_ONBOARD_DEVICE_DATA *OnboardDevice;
  UINTN                        NumberOfDevices;
  UINTN                        Index;
  UINT8                        StatusAndType;

  Status          = EFI_SUCCESS;
  OnboardDevice   = (EFI_MISC_ONBOARD_DEVICE_DATA *) RecordData;

  NumberOfDevices = (StructureNode->Structure->Length - 4) / 2;
  for (Index = 0; Index < NumberOfDevices; Index += 1) {
    //
    // OnBoard Device Description
    //
    SmbiosFldString (
      StructureNode,
      (UINT32) (OFFSET_OF (SMBIOS_TABLE_TYPE10, Device) + 1 + (2 * Index)),
      &(OnboardDevice->OnBoardDeviceDescription),
      2 // 64 * sizeof(CHAR16)
      );

    //
    // Status & Type: Bit 7    Devicen Status, Bits 6:0 Type of Device
    //
    StatusAndType = (UINT8) OnboardDevice->OnBoardDeviceStatus.DeviceType;
    if (OnboardDevice->OnBoardDeviceStatus.DeviceEnabled != 0) {
      StatusAndType |= 0x80;
    } else {
      StatusAndType &= 0x7F;
    }

    * (UINT8 *) ((UINT8 *) (StructureNode->Structure) + (OFFSET_OF (SMBIOS_TABLE_TYPE10, Device) + (2 * Index))) = StatusAndType;
  }

  return Status;
}

/**
  Field Filling Function for Misc SubClass record type 11 - OEM Strings.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType11 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_OEM_STRING_DATA *OemString;

  OemString = (EFI_MISC_OEM_STRING_DATA *)RecordData;
  //
  // OEM String data
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE11, StringCount),
    &(OemString->OemStringRef[0]),
    2
    );
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 12 - System Options.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType12 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                         Status;
  EFI_MISC_SYSTEM_OPTION_STRING_DATA *Sos;
  UINTN                              NumberOfInstallableLanguages;
  UINTN                              Index;

  Status  = EFI_SUCCESS;
  Sos     = (EFI_MISC_SYSTEM_OPTION_STRING_DATA *) RecordData;

  //
  // As MiscDataHub spec defines,
  // NumberOfInstallableLanguages should retrieve from Type 13.
  //
  NumberOfInstallableLanguages = (StructureNode->Structure->Length - 4);
  for (Index = 0; Index < NumberOfInstallableLanguages; Index += 1) {
    //
    // OnBoard Device Description
    //
    SmbiosFldString (
      StructureNode,
      (UINT32) (OFFSET_OF (SMBIOS_TABLE_TYPE12, StringCount) + (Index)),
      &(Sos->SystemOptionStringRef[Index]),
      2
      );
  }

  return Status;
}

/**
  Field Filling Function for Misc SubClass record type 13 - BIOS Language.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType13 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA  *InstallableLanguage;

  InstallableLanguage = (EFI_MISC_NUMBER_OF_INSTALLABLE_LANGUAGES_DATA *) RecordData;

  //
  // Number Of Installable Languages
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE13, InstallableLanguages)) = (UINT8) (InstallableLanguage->NumberOfInstallableLanguages);

  //
  // Language Flags
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE13, Flags),
    &InstallableLanguage->LanguageFlags,
    1
    );

  //
  // Current Language Number
  // It's the index of multiple languages. Languages are filled by SmbiosFldMiscType14.
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE13, CurrentLanguages),
    &InstallableLanguage->CurrentLanguageNumber,
    1
    );

  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 14 - System Language String
  Current solution assumes that EFI_MISC_SYSTEM_LANGUAGE_STRINGs are logged with
  their LanguageId having ascending orders.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType14 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  UINT16                           CurrentLanguageNumber;
  EFI_MISC_SYSTEM_LANGUAGE_STRING  *LanguageString;
  
  LanguageString = (EFI_MISC_SYSTEM_LANGUAGE_STRING *) RecordData;

  //
  // Backup CurrentLanguage
  //
  CopyMem (
    &CurrentLanguageNumber,
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE13, CurrentLanguages),
    1
    );

  //
  // Clear the field so that SmbiosFldString can be reused
  //
  *(UINT8 *)((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE13, CurrentLanguages)) = 0;
  
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE13, CurrentLanguages),
    &(LanguageString->SystemLanguageString),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // Restore CurrentLanguage
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE13, CurrentLanguages),
    &CurrentLanguageNumber,
    1
    );

  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 15 -- System Event Log.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType15 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                      Status;
  EFI_MISC_SYSTEM_EVENT_LOG_DATA  *SystemEventLog;

  Status          = EFI_SUCCESS;
  SystemEventLog  = NULL;

  SystemEventLog  = (EFI_MISC_SYSTEM_EVENT_LOG_DATA *) RecordData;

  //
  // Log Area Length
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE15, LogAreaLength),
    &(SystemEventLog->LogAreaLength),
    2
    );

  //
  // Log Header Start Offset
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE15, LogHeaderStartOffset),
    &(SystemEventLog->LogHeaderStartOffset),
    2
    );

  //
  // Log Data Start Offset
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE15, LogDataStartOffset),
    &(SystemEventLog->LogDataStartOffset),
    2
    );

  //
  // Access Method
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE15, AccessMethod),
    &(SystemEventLog->AccessMethod),
    1
    );

  //
  // Log Status
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE15, LogStatus),
    &(SystemEventLog->LogStatus),
    1
    );

  //
  // Log Change Token
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE15, LogChangeToken),
    &(SystemEventLog->LogChangeToken),
    4
    );

  //
  // Access Method Address
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE15, AccessMethodAddress),
    &(SystemEventLog->AccessMethodAddress),
    4
    );

  //
  // Log Header Format
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE15, LogHeaderFormat),
    &(SystemEventLog->LogHeaderFormat),
    1
    );

  //
  // Number of Supported Log Type Descriptors
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE15, NumberOfSupportedLogTypeDescriptors),
    &(SystemEventLog->NumberOfSupportedLogType),
    1
    );

  //
  // Length of each Log Type Descriptor
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE15, LengthOfLogTypeDescriptor),
    &(SystemEventLog->LengthOfLogDescriptor),
    1
    );

  return Status;
}

/**
  Field Filling Function for Misc SubClass record type 21 - Pointing Device.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType21 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_POINTING_DEVICE_TYPE_DATA *PointingDeviceData;

  PointingDeviceData = (EFI_MISC_POINTING_DEVICE_TYPE_DATA *) RecordData;

  //
  // Pointing Device Type
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE21, Type)) = (UINT8) (PointingDeviceData->PointingDeviceType);

  //
  // Pointing Device Interface
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE21, Interface)) = (UINT8) (PointingDeviceData->PointingDeviceInterface);

  //
  // Number Pointing Device Buttons
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE21, NumberOfButtons)) = (UINT8) (PointingDeviceData->NumberPointingDeviceButtons);

  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 22 - Portable Battery.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType22 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_PORTABLE_BATTERY *PortableBattery;
  STRING_REF                Chemistry;
  PortableBattery = (EFI_MISC_PORTABLE_BATTERY *)RecordData;
  
  //
  // Location
  // 
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE22, Location),
    &(PortableBattery->Location),
    2 // 64 * sizeof(CHAR16)
    );
  
  //
  // Manufacturer
  // 
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE22, Manufacturer),
    &(PortableBattery->Manufacturer),
    2 
    );  

  //
  // ManufactureDate
  // 
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE22, ManufactureDate),
    &(PortableBattery->ManufactureDate),
    2 
    );    

  //
  // SerialNumber
  // 
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE22, SerialNumber),
    &(PortableBattery->SerialNumber),
    2 
    );

  //
  // DeviceName
  // 
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE22, DeviceName),
    &(PortableBattery->DeviceName),
    2 
  );   
  
  //
  // DeviceChemistry
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE22, DeviceChemistry),
    &PortableBattery->DeviceChemistry,
    1
    );

  //
  // DesignCapacity
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE22, DeviceCapacity),
    &PortableBattery->DesignCapacity,
    2
    );
    
  //
  // DesignVoltage
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE22, DesignVoltage),
    &PortableBattery->DesignVoltage,
    2
    );    
 
  //
  // SBDSVersionNumber
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE22, SBDSVersionNumber),
    &(PortableBattery->SBDSVersionNumber),
    2 // 64 * sizeof(CHAR16)
    );

  //
  // MaximumError
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE22, MaximumErrorInBatteryData),
    &PortableBattery->MaximumError,
    1
    ); 
    
  //
  // SBDSSerialNumber
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE22, SBDSSerialNumber),
    &PortableBattery->SBDSSerialNumber,
    2
    );      

  //
  // SBDSManufactureDate
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE22, SBDSManufactureDate),
    &PortableBattery->SBDSManufactureDate,
    2
    );

  //
  // Avoid alignment issue on IPF
  //
  CopyMem (
    &Chemistry, 
    &PortableBattery->SBDSDeviceChemistry,
    2
    );

  //
  // SBDSDeviceChemistry
  //    
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE22, SBDSDeviceChemistry),
    &Chemistry,
    2 // 64 * sizeof(CHAR16)
    ); 
    
  //
  // DesignCapacityMultiplier
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE22, DesignCapacityMultiplier),
    &PortableBattery->DesignCapacityMultiplier,
    1
    );             
 
  //
  // OEMSpecific
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE22, OEMSpecific),
    &PortableBattery->OEMSpecific,
    4
    );
    
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 23 - System Reset.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType23 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_RESET_CAPABILITIES_DATA  *SystemResetData;

  SystemResetData = (EFI_MISC_RESET_CAPABILITIES_DATA *) RecordData;

  //
  // Reset Capabilities
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE23, Capabilities),
    &(SystemResetData->ResetCapabilities),
    1
    );

  //
  // Reset Count
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE23, ResetCount),
    &(SystemResetData->ResetCount),
    2
    );

  //
  // Reset Limit
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE23, ResetLimit),
    &(SystemResetData->ResetLimit),
    2
    );

  //
  // Reset Timer Interval
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE23, TimerInterval),
    &(SystemResetData->ResetTimerInterval),
    2
    );

  //
  // Reset Timeout
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE23, Timeout),
    &(SystemResetData->ResetTimeout),
    2
    );

  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 24 - Hardware Security.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType24 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_HARDWARE_SECURITY_SETTINGS_DATA *HardwareSecurity;
  
  HardwareSecurity = (EFI_MISC_HARDWARE_SECURITY_SETTINGS_DATA *)RecordData;
  
  //
  // Hardware Security Settings
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE24, HardwareSecuritySettings),
    &HardwareSecurity->HardwareSecuritySettings,
    1
    );
  
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 25 - System Power Controls.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType25 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_SCHEDULED_POWER_ON_MONTH *PowerOnMonth;
  
  PowerOnMonth = (EFI_MISC_SCHEDULED_POWER_ON_MONTH *)RecordData;
  
  //
  // ScheduledPoweronMonth
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE25, NextScheduledPowerOnMonth),
    &PowerOnMonth->ScheduledPoweronMonth,
    1
    );
    
  //
  // ScheduledPoweronDayOfMonth
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE25, NextScheduledPowerOnDayOfMonth),
    &PowerOnMonth->ScheduledPoweronDayOfMonth,
    1
    );    

  //
  // ScheduledPoweronHour
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE25, NextScheduledPowerOnHour),
    &PowerOnMonth->ScheduledPoweronHour,
    1
    );   

  //
  // ScheduledPoweronMinute
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE25, NextScheduledPowerOnMinute),
    &PowerOnMonth->ScheduledPoweronMinute,
    1
    );   

  //
  // ScheduledPoweronSecond
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE25, NextScheduledPowerOnSecond),
    &PowerOnMonth->ScheduledPoweronSecond,
    1
    );
  
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 26 - Voltage Probe.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType26 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_VOLTAGE_PROBE_DESCRIPTION *VoltageProbe;
  
  VoltageProbe = (EFI_MISC_VOLTAGE_PROBE_DESCRIPTION *)RecordData;
  
  //
  // VoltageProbe Description
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE26, Description),
    &(VoltageProbe->VoltageProbeDescription),
    2 // 64 * sizeof(CHAR16)
    );  
    
  //
  // VoltageProbeLocation
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE26, LocationAndStatus),
    &VoltageProbe->VoltageProbeLocation,
    1
    );    
    
  //
  // VoltageProbeMaximumValue
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE26, MaximumValue),
    &VoltageProbe->VoltageProbeMaximumValue,
    2
    );    
    
  //
  // VoltageProbeMinimumValue
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE26, MinimumValue),
    &VoltageProbe->VoltageProbeMinimumValue,
    2
    );   

  //
  // VoltageProbeResolution
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE26, Resolution),
    &VoltageProbe->VoltageProbeResolution,
    2
    );    

  //
  // VoltageProbeTolerance
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE26, Tolerance),
    &VoltageProbe->VoltageProbeTolerance,
    2
    );   

  //
  // VoltageProbeAccuracy
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE26, Accuracy),
    &VoltageProbe->VoltageProbeAccuracy,
    2
    );  
    
  //
  // VoltageProbeNominalValue
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE26, NominalValue),
    &VoltageProbe->VoltageProbeNominalValue,
    2
    );   

  //
  // VoltageProbeOemDefined
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE26, OEMDefined),
    &VoltageProbe->VoltageProbeOemDefined,
    4
    );
  
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 27 - Cooling Device.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType27 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_COOLING_DEVICE_TEMP_LINK *CoolingDevice;
  
  CoolingDevice = (EFI_MISC_COOLING_DEVICE_TEMP_LINK *)RecordData;
  
  //
  // Device Type
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE27, DeviceTypeAndStatus),
    &CoolingDevice->CoolingDeviceType,
    1
    );
  
  //
  // Temperature Probe
  //
  SmbiosFldInterLink (
    StructureNode,
    (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE27, TemperatureProbeHandle),
    28,  // SMBIOS type 28 - Temperature Probe
    &CoolingDevice->CoolingDeviceTemperatureLink,
    &gEfiMiscSubClassGuid
    );
  
  //
  // CoolingDeviceUnitGroup
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE27, CoolingUnitGroup),
    &CoolingDevice->CoolingDeviceUnitGroup,
    1
    );

  //
  // CoolingDeviceUnitGroup
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE27, OEMDefined),
    &CoolingDevice->CoolingDeviceOemDefined,
    4
    );

  //
  // CoolingDeviceNominalSpeed
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE27, NominalSpeed),
    &CoolingDevice->CoolingDeviceNominalSpeed,
    2
    );
  
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 28 -- Temperature Probe.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType28 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_TEMPERATURE_PROBE_DESCRIPTION *TemperatureProbe;
  
  TemperatureProbe = (EFI_MISC_TEMPERATURE_PROBE_DESCRIPTION *)RecordData;
  
  //
  // TemperatureProbeDescription
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE28, Description),
    &(TemperatureProbe->TemperatureProbeDescription),
    2
    );
    
  //
  // TemperatureProbeLocation
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE28, LocationAndStatus),
    &TemperatureProbe->TemperatureProbeLocation,
    1
    ); 
    
  //
  // TemperatureProbeMaximumValue
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE28, MaximumValue),
    &TemperatureProbe->TemperatureProbeMaximumValue,
    2
    );        

  //
  // TemperatureProbeMinimumValue
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE28, MinimumValue),
    &TemperatureProbe->TemperatureProbeMinimumValue,
    2
    );  

  //
  // TemperatureProbeResolution
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE28, Resolution),
    &TemperatureProbe->TemperatureProbeResolution,
    2
    );  
  

  //
  // TemperatureProbeTolerance
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE28, Tolerance),
    &TemperatureProbe->TemperatureProbeTolerance,
    2
    );  

  //
  // TemperatureProbeAccuracy
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE28, Accuracy),
    &TemperatureProbe->TemperatureProbeAccuracy,
    2
    );

  //
  // TemperatureProbeNominalValue
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE28, NominalValue),
    &TemperatureProbe->TemperatureProbeNominalValue,
    2
    );

  //
  // TemperatureProbeOemDefined
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE28, OEMDefined),
    &TemperatureProbe->TemperatureProbeOemDefined,
    4
    );
  
  return EFI_SUCCESS;
}  

/**
  Field Filling Function for Misc SubClass record type 29 -- Electrical Current Probe.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType29 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_ELECTRICAL_CURRENT_PROBE_DESCRIPTION *ElectricalProbe;
  
  ElectricalProbe = (EFI_MISC_ELECTRICAL_CURRENT_PROBE_DESCRIPTION *)RecordData;
  
  //
  // ElectricalCurrentProbeDescription
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE29, Description),
    &(ElectricalProbe->ElectricalCurrentProbeDescription),
    2 // 64 * sizeof(CHAR16)
    );
    
  //
  // ElectricalCurrentProbeLocation
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE29, LocationAndStatus),
    &ElectricalProbe->ElectricalCurrentProbeLocation,
    1
    );
    
  //
  // ElectricalCurrentProbeMaximumValue
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE29, MaximumValue),
    &ElectricalProbe->ElectricalCurrentProbeMaximumValue,
    2
    );   

  //
  // ElectricalCurrentProbeMinimumValue
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE29, MinimumValue),
    &ElectricalProbe->ElectricalCurrentProbeMinimumValue,
    2
    );   

  //
  // ElectricalCurrentProbeResolution
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE29, Resolution),
    &ElectricalProbe->ElectricalCurrentProbeResolution,
    2
    );     

  //
  // ElectricalCurrentProbeTolerance
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE29, Tolerance),
    &ElectricalProbe->ElectricalCurrentProbeTolerance,
    2
    );     

  //
  // ElectricalCurrentProbeAccuracy
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE29, Accuracy),
    &ElectricalProbe->ElectricalCurrentProbeAccuracy,
    2
    );   
  //
  // ElectricalCurrentProbeNominalValue
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE29, NominalValue),
    &ElectricalProbe->ElectricalCurrentProbeNominalValue,
    2
    );   

  //
  // ElectricalCurrentProbeOemDefined
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE29, OEMDefined),
    &ElectricalProbe->ElectricalCurrentProbeOemDefined,
    4
    );
  
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 30 -- Out-of-Band Remote Access.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType30 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_REMOTE_ACCESS_MANUFACTURER_DESCRIPTION *RemoteData;
  
  RemoteData = (EFI_MISC_REMOTE_ACCESS_MANUFACTURER_DESCRIPTION *)RecordData;
  
  //
  // ManufacturerNameDescription
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE30, ManufacturerName),
    &(RemoteData->RemoteAccessManufacturerNameDescription),
    2 // 64 * sizeof(CHAR16)
    );  
    
  //
  // RemoteAccessConnections
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE30, Connections),
    &RemoteData->RemoteAccessConnections,
    1
    );
  
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 32 -- System Boot Information.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType32 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                             Status;
  EFI_MISC_BOOT_INFORMATION_STATUS_DATA  *BootInfo;

  Status    = EFI_SUCCESS;
  BootInfo  = (EFI_MISC_BOOT_INFORMATION_STATUS_DATA *) RecordData;

  //
  // Set reserved bytes
  //
  ZeroMem ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE32, Reserved), 6);

  //
  // Set BootInformation Status
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE32, BootStatus),
    &BootInfo->BootInformationStatus,
    1
    );

  //
  // Set Additional Data
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE32, BootStatus) + 1,
    &BootInfo->BootInformationData,
    9
    );

  return Status;
}

/**
  Field Filling Function for Misc SubClass record type 34 -- Management Device.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType34 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_MANAGEMENT_DEVICE_DESCRIPTION *ManagementDevice;
  
  ManagementDevice = (EFI_MISC_MANAGEMENT_DEVICE_DESCRIPTION *)RecordData;
  
  //
  // ManagementDeviceDescription
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE34, Description),
    &(ManagementDevice->ManagementDeviceDescription),
    2 // 64 * sizeof(CHAR16)
    );  
    
  //
  // ManagementDeviceType
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE34, Type),
    &ManagementDevice->ManagementDeviceType,
    1
    );    

  //
  // ManagementDeviceAddress
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE34, Address),
    &ManagementDevice->ManagementDeviceAddress,
    4
    );    

  //
  // ManagementDeviceAddressType
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE34, AddressType),
    &ManagementDevice->ManagementDeviceAddressType,
    1
    );  
  
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 35 -- Management Device Component.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType35 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_MANAGEMENT_DEVICE_COMPONENT_DESCRIPTION  *ManagementDeviceComponent;
  EFI_INTER_LINK_DATA                               ManagementDeviceLink;
  EFI_INTER_LINK_DATA                               ManagementDeviceComponentLink;
  EFI_INTER_LINK_DATA                               ManagementDeviceThresholdLink;
  
  ManagementDeviceComponent = (EFI_MISC_MANAGEMENT_DEVICE_COMPONENT_DESCRIPTION *)RecordData;
  CopyMem (
    &ManagementDeviceLink,
    &ManagementDeviceComponent->ManagementDeviceLink,
    sizeof (EFI_INTER_LINK_DATA)
    );
  CopyMem (
    &ManagementDeviceComponentLink,
    &ManagementDeviceComponent->ManagementDeviceComponentLink,
    sizeof (EFI_INTER_LINK_DATA)
    );
  CopyMem (&ManagementDeviceThresholdLink,
    &ManagementDeviceComponent->ManagementDeviceThresholdLink,
    sizeof (EFI_INTER_LINK_DATA)
    );

  //
  // ManagementDeviceComponentDescription
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE35, Description),
    &ManagementDeviceComponent->ManagementDeviceComponentDescription,
    2       // 64 * sizeof(CHAR16)
    );

  //
  // ManagementDeviceLink
  //
  SmbiosFldInterLink (
    StructureNode,
    (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE35, ManagementDeviceHandle),
    34,     // SMBIOS type 34 - Management Device
    &ManagementDeviceLink,
    &gEfiMiscSubClassGuid
    );

  //
  // ManagementDeviceComponentLink
  //
  SmbiosFldInterLink (
    StructureNode,
    (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE35, ComponentHandle),
    ManagementDeviceComponent->ComponentType,   // SMBIOS type, according to SMBIOS spec, it can be Type 26, 27, 28, 29
    &ManagementDeviceComponentLink,
    &gEfiMiscSubClassGuid
    );

  //
  // ManagementDeviceThresholdLink
  //
  SmbiosFldInterLink (
    StructureNode,
    (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE35, ThresholdHandle),
    36,     // SMBIOS type 36 - Management Device Threshold Data
    &ManagementDeviceThresholdLink,
    &gEfiMiscSubClassGuid
    );

  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 36 -- Management Device Threshold.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType36 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_MANAGEMENT_DEVICE_THRESHOLD *DeviceThreshold;
  
  DeviceThreshold = (EFI_MISC_MANAGEMENT_DEVICE_THRESHOLD *)RecordData;

  //
  // LowerThresNonCritical
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE36, LowerThresholdNonCritical),
    &DeviceThreshold->LowerThresNonCritical,
    2
    );    

  //
  // UpperThresNonCritical
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE36, UpperThresholdNonCritical),
    &DeviceThreshold->UpperThresNonCritical,
    2
    );
    
  //
  // LowerThresCritical
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE36, LowerThresholdCritical),
    &DeviceThreshold->LowerThresCritical,
    2
    );          

  //
  // UpperThresCritical
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE36, UpperThresholdCritical),
    &DeviceThreshold->UpperThresCritical,
    2
    );   

  //
  // LowerThresNonRecover
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE36, LowerThresholdNonRecoverable),
    &DeviceThreshold->LowerThresNonRecover,
    2
    );

  //
  // UpperThresNonRecover
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE36, UpperThresholdNonRecoverable),
    &DeviceThreshold->UpperThresNonRecover,
    2
    );          
  
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 38 -- IPMI device info.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType38 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_IPMI_INTERFACE_TYPE_DATA  *IpmiInfo;

  IpmiInfo  = (EFI_MISC_IPMI_INTERFACE_TYPE_DATA *) RecordData;

  //
  // Interface Type
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE38, InterfaceType)) = (UINT8) (IpmiInfo->IpmiInterfaceType);

  //
  // IPMI specification revision
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE38, IPMISpecificationRevision)) = 
  (UINT8) ((IpmiInfo->IpmiSpecificationRevision.IpmiSpecLeastSignificantDigit) + \
           (IpmiInfo->IpmiSpecificationRevision.IpmiSpecMostSignificantDigit << 4));

  //
  // I2C slave address
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE38, I2CSlaveAddress)) = (UINT8) (IpmiInfo->IpmiI2CSlaveAddress);

  //
  // NV storage device address
  //
  *(UINT8 *) ((UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE38, NVStorageDeviceAddress)) = (UINT8) (IpmiInfo->IpmiNvDeviceAddress);

  //
  // Base address
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE38, BaseAddress),
    &IpmiInfo->IpmiBaseAddress,
    8
    );

  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 39 -- Power supply.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType39 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_MISC_SYSTEM_POWER_SUPPLY *PowerSupply;
  
  PowerSupply = (EFI_MISC_SYSTEM_POWER_SUPPLY *)RecordData;
  
  //
  // PowerUnitGroup
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE39, PowerUnitGroup),
    &PowerSupply->PowerUnitGroup,
    1
    );  
    
  //
  // PowerSupplyLocation
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE39, Location),
    &(PowerSupply->PowerSupplyLocation),
    2 // 64 * sizeof(CHAR16)
    );    

  //
  // PowerSupplyDeviceName
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE39, DeviceName),
    &(PowerSupply->PowerSupplyDeviceName),
    2 // 64 * sizeof(CHAR16)
    );   

  //
  // PowerSupplyManufacturer
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE39, Manufacturer),
    &(PowerSupply->PowerSupplyManufacturer),
    2 // 64 * sizeof(CHAR16)
    ); 

  //
  // PowerSupplySerialNumber
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE39, SerialNumber),
    &(PowerSupply->PowerSupplySerialNumber),
    2 // 64 * sizeof(CHAR16)
    );  
    
  //
  // PowerSupplyAssetTagNumber
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE39, AssetTagNumber),
    &(PowerSupply->PowerSupplyAssetTagNumber),
    2 // 64 * sizeof(CHAR16)
    );      

  //
  // PowerSupplyModelPartNumber
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE39, ModelPartNumber),
    &(PowerSupply->PowerSupplyModelPartNumber),
    2 // 64 * sizeof(CHAR16)
    ); 
    
  //
  // PowerSupplyRevisionLevel
  //
  SmbiosFldString (
    StructureNode,
    OFFSET_OF (SMBIOS_TABLE_TYPE39, RevisionLevel),
    &(PowerSupply->PowerSupplyRevisionLevel),
    2 // 64 * sizeof(CHAR16)
    );      

  //
  // PowerSupplyMaxPowerCapacity
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE39, MaxPowerCapacity),
    &PowerSupply->PowerSupplyMaxPowerCapacity,
    2
    );

  //
  // PowerSupplyCharacteristics
  //
  CopyMem (
    (UINT8 *) (StructureNode->Structure) + OFFSET_OF (SMBIOS_TABLE_TYPE39, PowerSupplyCharacteristics),
    &PowerSupply->PowerSupplyCharacteristics,
    2
    );
    
  //
  // PowerSupplyInputVoltageProbeLink
  //
  SmbiosFldInterLink (
    StructureNode,
    (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE39, InputVoltageProbeHandle),
    26,  // SMBIOS type 26 - Voltage Probe
    &PowerSupply->PowerSupplyInputVoltageProbeLink,
    &gEfiMiscSubClassGuid
    );    

  //
  // PowerSupplyCoolingDeviceLink
  //
  SmbiosFldInterLink (
    StructureNode,
    (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE39, CoolingDeviceHandle),
    27,  // SMBIOS type 27 - Cooling Device
    &PowerSupply->PowerSupplyCoolingDeviceLink,
    &gEfiMiscSubClassGuid
    );   

  //
  // PowerSupplyInputCurrentProbeLink
  //
  SmbiosFldInterLink (
    StructureNode,
    (UINT16) OFFSET_OF (SMBIOS_TABLE_TYPE39, InputCurrentProbeHandle),
    29,  // SMBIOS type 29 - Electrical Current Probe
    &PowerSupply->PowerSupplyInputCurrentProbeLink,
    &gEfiMiscSubClassGuid
    );  
  
  return EFI_SUCCESS;
}

/**
  Field Filling Function for Misc SubClass record type 0x80-0xFF -- OEM.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscTypeOEM (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  EFI_STATUS                            Status;
  UINT8                                 *NewRecordData;
  UINT32                                IncrementDataSize;
  UINT16                                Handle;
  INT8                                  Result;
  UINT32                                StructureSize;
  UINT8                                 CountOfString;
  
  Status             = EFI_SUCCESS;
  NewRecordData      = NULL;

  //
  // Check if OEM structure has included 2 trailing 0s in data record, if not, 
  // we append them at the end to ensure OEM structure is always correct with 2 trailing 0s.
  //
  Result = SmbiosCheckTrailingZero (RecordData, RecordDataSize);
  
  if (Result != 0) {
    DEBUG ((EFI_D_ERROR, "OEM SMBIOS type %x is not valid!!\n", ((SMBIOS_STRUCTURE *) RecordData) -> Type));
    if (Result == -1) {
      //
      // No 2 trailing 0s exist
      //
      DEBUG ((EFI_D_ERROR, "OEM SMBIOS type has NO 2 trailing 0s!!\n"));
      IncrementDataSize = 2;
    } else {
      //
      // Only 1 trailing 0 exist at the end
      //
      DEBUG ((EFI_D_ERROR, "OEM SMBIOS type has only 1 trailing 0!!\n"));
      IncrementDataSize = 1;
    }
    NewRecordData = AllocateZeroPool (RecordDataSize + IncrementDataSize);
    ASSERT (NewRecordData != NULL);
    CopyMem (NewRecordData, RecordData, RecordDataSize);
    RecordData = NewRecordData;
    RecordDataSize += IncrementDataSize;
  }
  
  Status = GetSmbiosStructureSize (StructureNode->Structure, &StructureSize, &CountOfString);
  ASSERT_EFI_ERROR (Status);
  
  if (StructureSize < RecordDataSize) {
    //
    // Create new SMBIOS table entry
    //
    SmbiosUpdateStructureBuffer (
      StructureNode,
      RecordData
      );
  } else {
    //
    // Copy the entire data (including the Smbios structure header),
    // but preserve the handle that is already allocated.
    //
    Handle = StructureNode->Structure->Handle;
    CopyMem (
      StructureNode->Structure,
      RecordData,
      RecordDataSize
      );
    StructureNode->Structure->Handle = Handle;
    StructureNode->StructureSize = RecordDataSize;
  }
  
  if (NewRecordData != NULL) {
    FreePool (NewRecordData);
  }

  return Status;
}

/**
  Field Filling Function for Misc SubClass record type 127 - End-of-Table.
  
  @param StructureNode    Pointer to SMBIOS_STRUCTURE_NODE which is current processed.
  @param Offset           Offset of SMBIOS record which RecordData will be filled.
  @param RecordData       RecordData buffer will be filled.
  @param RecordDataSize   The size of RecordData buffer.
  
  @retval EFI_SUCCESS   Success fill RecordData into SMBIOS's record buffer.
**/
EFI_STATUS
SmbiosFldMiscType127 (
  IN OUT  SMBIOS_STRUCTURE_NODE     *StructureNode,
  IN      UINT32                    Offset,
  IN      VOID                      *RecordData,
  IN      UINT32                    RecordDataSize
  )
{
  return EFI_SUCCESS;
}
