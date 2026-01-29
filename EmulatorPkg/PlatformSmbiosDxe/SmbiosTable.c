/** @file
  Static SMBIOS Table for platform

  Note SMBIOS 2.7.1 Required structures:
    BIOS Information (Type 0)
    System Information (Type 1)
    System Enclosure (Type 3)
    Processor Information (Type 4) - CPU Driver
    Cache Information (Type 7) - For cache that is external to processor
    System Slots (Type 9) - If system has slots
    Physical Memory Array (Type 16)
    Memory Device (Type 17) - For each socketed system-memory Device
    Memory Array Mapped Address (Type 19) - One per contiguous block per Physical Memroy Array
    System Boot Information (Type 32)

  Copyright (c) 2012, Apple Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>
#include <Library/SmbiosLib.h>

SMBIOS_TABLE_TYPE0  gSmbiosType0Template = {
  { EFI_SMBIOS_TYPE_BIOS_INFORMATION, sizeof (SMBIOS_TABLE_TYPE0), 0 },
  1,                    // Vendor String
  2,                    // BiosVersion String
  0xE000,               // BiosSegment
  3,                    // BiosReleaseDate String
  0x7F,                 // BiosSize
  {       // BiosCharacteristics
    0,    //  Reserved                          :2;  ///< Bits 0-1.
    0,    //  Unknown                           :1;
    0,    //  BiosCharacteristicsNotSupported   :1;
    0,    //  IsaIsSupported                    :1;
    0,    //  McaIsSupported                    :1;
    0,    //  EisaIsSupported                   :1;
    1,    //  PciIsSupported                    :1;
    0,    //  PcmciaIsSupported                 :1;
    0,    //  PlugAndPlayIsSupported            :1;
    0,    //  ApmIsSupported                    :1;
    1,    //  BiosIsUpgradable                  :1;
    1,    //  BiosShadowingAllowed              :1;
    0,    //  VlVesaIsSupported                 :1;
    0,    //  EscdSupportIsAvailable            :1;
    0,    //  BootFromCdIsSupported             :1;
    1,    //  SelectableBootIsSupported         :1;
    0,    //  RomBiosIsSocketed                 :1;
    0,    //  BootFromPcmciaIsSupported         :1;
    0,    //  EDDSpecificationIsSupported       :1;
    0,    //  JapaneseNecFloppyIsSupported      :1;
    0,    //  JapaneseToshibaFloppyIsSupported  :1;
    0,    //  Floppy525_360IsSupported          :1;
    0,    //  Floppy525_12IsSupported           :1;
    0,    //  Floppy35_720IsSupported           :1;
    0,    //  Floppy35_288IsSupported           :1;
    0,    //  PrintScreenIsSupported            :1;
    0,    //  Keyboard8042IsSupported           :1;
    0,    //  SerialIsSupported                 :1;
    0,    //  PrinterIsSupported                :1;
    0,    //  CgaMonoIsSupported                :1;
    0,    //  NecPc98                           :1;
    0     //  ReservedForVendor                 :32; ///< Bits 32-63. Bits 32-47 reserved for BIOS vendor
    ///< and bits 48-63 reserved for System Vendor.
  },
  {       // BIOSCharacteristicsExtensionBytes[]
    0x81, //  AcpiIsSupported                   :1;
          //  UsbLegacyIsSupported              :1;
          //  AgpIsSupported                    :1;
          //  I2OBootIsSupported                :1;
          //  Ls120BootIsSupported              :1;
          //  AtapiZipDriveBootIsSupported      :1;
          //  Boot1394IsSupported               :1;
          //  SmartBatteryIsSupported           :1;
    //  BIOSCharacteristicsExtensionBytes[1]
    0x0a, //  BiosBootSpecIsSupported              :1;
          //  FunctionKeyNetworkBootIsSupported    :1;
          //  TargetContentDistributionEnabled     :1;
          //  UefiSpecificationSupported           :1;
          //  VirtualMachineSupported              :1;
          //  ExtensionByte2Reserved               :3;
  },
  0x00,                    // SystemBiosMajorRelease
  0x01,                    // SystemBiosMinorRelease
  0xFF,                    // EmbeddedControllerFirmwareMajorRelease
  0xFF,                    // EmbeddedControllerFirmwareMinorRelease
};
CHAR8               *gSmbiosType0Strings[] = {
  "http://www.tianocore.org/edk2/", // Vendor String
  __TIME__,                         // BiosVersion String
  __DATE__,                         // BiosReleaseDate String
  NULL
};

SMBIOS_TABLE_TYPE1  gSmbiosType1Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_INFORMATION, sizeof (SMBIOS_TABLE_TYPE1), 0    },
  1,    // Manufacturer String
  2,    // ProductName String
  3,    // Version String
  4,    // SerialNumber String
  { 0x25EF0280,                         0xEC82,                      0x42B0, { 0x8F, 0xB6, 0x10, 0xAD, 0xCC, 0xC6, 0x7C, 0x02}
  },
  SystemWakeupTypePowerSwitch,
  5,    // SKUNumber String
  6,    // Family String
};
CHAR8               *gSmbiosType1Strings[] = {
  "http://www.tianocore.org/edk2/",
  "EmulatorPkg",
  "1.0",
  "System Serial#",
  "System SKU#",
  "edk2",
  NULL
};

SMBIOS_TABLE_TYPE2  gSmbiosType2Template = {
  { EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION, sizeof (SMBIOS_TABLE_TYPE2), 0 },
  1,    // Manufacturer String
  2,    // ProductName String
  3,    // Version String
  4,    // SerialNumber String
  5,    // AssetTag String
  {     // FeatureFlag
    1,  //  Motherboard           :1;
    0,  //  RequiresDaughterCard  :1;
    0,  //  Removable             :1;
    0,  //  Replaceable           :1;
    0,  //  HotSwappable          :1;
    0,  //  Reserved              :3;
  },
  6,                        // LocationInChassis String
  0,                        // ChassisHandle;
  BaseBoardTypeMotherBoard, // BoardType;
  0,                        // NumberOfContainedObjectHandles;
  { 0 }                     // ContainedObjectHandles[1];
};
CHAR8               *gSmbiosType2Strings[] = {
  "http://www.tianocore.org/edk2/",
  "EmulatorPkg",
  "1.0",
  "Base Board Serial#",
  "Base Board Asset Tag#",
  "Part Component",
  NULL
};

SMBIOS_TABLE_TYPE3  gSmbiosType3Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE, sizeof (SMBIOS_TABLE_TYPE3), 0 },
  1,                                                                    // Manufacturer String
  MiscChassisTypeLapTop,                                                // Type;
  2,                                                                    // Version String
  3,                                                                    // SerialNumber String
  4,                                                                    // AssetTag String
  ChassisStateSafe,                                                     // BootupState;
  ChassisStateSafe,                                                     // PowerSupplyState;
  ChassisStateSafe,                                                     // ThermalState;
  ChassisSecurityStatusNone,                                            // SecurityStatus;
  { 0,                                0,                           0, 0}, // OemDefined[4];
  0,                                                                    // Height;
  0,                                                                    // NumberofPowerCords;
  0,                                                                    // ContainedElementCount;
  0,                                                                    // ContainedElementRecordLength;
  {
    { 0 }
  },            // ContainedElements[1];
};
CHAR8               *gSmbiosType3Strings[] = {
  "http://www.tianocore.org/edk2/",
  "EmulatorPkg",
  "Chassis Board Serial#",
  "Chassis Board Asset Tag#",
  NULL
};

SMBIOS_TABLE_TYPE8  gSmbiosType8Template1 = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  0,                            // InternalReferenceDesignator String
  PortConnectorTypeNone,        // InternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  1,                            // ExternalReferenceDesignator String
  PortConnectorTypeNone,        // ExternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeOther,                // PortType;                       ///< The enumeration value from MISC_PORT_TYPE.
};
CHAR8               *gSmbiosType8Strings1[] = {
  "Mini DisplayPort",
  NULL
};

SMBIOS_TABLE_TYPE8  gSmbiosType8Template2 = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  0,                            // InternalReferenceDesignator String
  PortConnectorTypeNone,        // InternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  1,                            // ExternalReferenceDesignator String
  PortConnectorTypeNone,        // ExternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeFireWire,             // PortType;                       ///< The enumeration value from MISC_PORT_TYPE.
};
CHAR8               *gSmbiosType8Strings2[] = {
  "FireWire 800",
  NULL
};

SMBIOS_TABLE_TYPE8  gSmbiosType8Template3 = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  0,                            // InternalReferenceDesignator String
  PortConnectorTypeNone,        // InternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  1,                            // ExternalReferenceDesignator String
  PortConnectorTypeRJ45,        // ExternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeNetworkPort,          // PortType;                       ///< The enumeration value from MISC_PORT_TYPE.
};
CHAR8               *gSmbiosType8Strings3[] = {
  "Ethernet",
  NULL
};

SMBIOS_TABLE_TYPE8  gSmbiosType8Template4 = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  0,                            // InternalReferenceDesignator String
  PortConnectorTypeNone,        // InternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  1,                            // ExternalReferenceDesignator String
  PortConnectorTypeUsb,         // ExternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeUsb,                  // PortType;                       ///< The enumeration value from MISC_PORT_TYPE.
};
CHAR8               *gSmbiosType8Strings4[] = {
  "USB0",
  NULL
};

SMBIOS_TABLE_TYPE8  gSmbiosType8Template5 = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  0,                            // InternalReferenceDesignator String
  PortConnectorTypeNone,        // InternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  1,                            // ExternalReferenceDesignator String
  PortConnectorTypeUsb,         // ExternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeUsb,                  // PortType;                       ///< The enumeration value from MISC_PORT_TYPE.
};
CHAR8               *gSmbiosType8Strings5[] = {
  "USB1",
  NULL
};

SMBIOS_TABLE_TYPE8  gSmbiosType8Template6 = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  0,                            // InternalReferenceDesignator String
  PortConnectorTypeNone,        // InternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  1,                            // ExternalReferenceDesignator String
  PortConnectorTypeUsb,         // ExternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeUsb,                  // PortType;                       ///< The enumeration value from MISC_PORT_TYPE.
};
CHAR8               *gSmbiosType8Strings6[] = {
  "USB2",
  NULL
};

SMBIOS_TABLE_TYPE8  gSmbiosType8Template7 = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  0,                            // InternalReferenceDesignator String
  PortConnectorTypeNone,        // InternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  1,                            // ExternalReferenceDesignator String
  PortConnectorTypeUsb,         // ExternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeUsb,                  // PortType;                       ///< The enumeration value from MISC_PORT_TYPE.
};
CHAR8               *gSmbiosType8Strings7[] = {
  "USB3",
  NULL
};

SMBIOS_TABLE_TYPE8  gSmbiosType8Template8 = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  0,                                  // InternalReferenceDesignator String
  PortConnectorTypeNone,              // InternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  1,                                  // ExternalReferenceDesignator String
  PortConnectorTypeHeadPhoneMiniJack, // ExternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeAudioPort,                  // PortType;                       ///< The enumeration value from MISC_PORT_TYPE.
};
CHAR8               *gSmbiosType8Strings8[] = {
  "Audio Line In",
  NULL
};

SMBIOS_TABLE_TYPE8  gSmbiosType8Template9 = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  0,                                  // InternalReferenceDesignator String
  PortConnectorTypeNone,              // InternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  1,                                  // ExternalReferenceDesignator String
  PortConnectorTypeHeadPhoneMiniJack, // ExternalConnectorType;          ///< The enumeration value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeAudioPort,                  // PortType;                       ///< The enumeration value from MISC_PORT_TYPE.
};
CHAR8               *gSmbiosType8Strings9[] = {
  "Audio Line Out",
  NULL
};

SMBIOS_TABLE_TYPE9  gSmbiosType9Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_SLOTS, sizeof (SMBIOS_TABLE_TYPE9), 0 },
  1,                     // SlotDesignation String
  SlotTypeOther,         // SlotType;                 ///< The enumeration value from MISC_SLOT_TYPE.
  SlotDataBusWidthOther, // SlotDataBusWidth;         ///< The enumeration value from MISC_SLOT_DATA_BUS_WIDTH.
  SlotUsageAvailable,    // CurrentUsage;             ///< The enumeration value from MISC_SLOT_USAGE.
  SlotLengthOther,       // SlotLength;               ///< The enumeration value from MISC_SLOT_LENGTH.
  0,                     // SlotID;
  {    // SlotCharacteristics1;
    1, // CharacteristicsUnknown  :1;
    0, // Provides50Volts         :1;
    0, // Provides33Volts         :1;
    0, // SharedSlot              :1;
    0, // PcCard16Supported       :1;
    0, // CardBusSupported        :1;
    0, // ZoomVideoSupported      :1;
    0, // ModemRingResumeSupported:1;
  },
  {     // SlotCharacteristics2;
    0,  // PmeSignalSupported      :1;
    0,  // HotPlugDevicesSupported :1;
    0,  // SmbusSignalSupported    :1;
    0,  // Reserved                :5;  ///< Set to 0.
  },
  0,    // SegmentGroupNum;
  0,    // BusNum;
  0,    // DevFuncNum;
};
CHAR8               *gSmbiosType9Strings[] = {
  "SD Card",
  NULL
};

SMBIOS_TABLE_TYPE11  gSmbiosType11Template = {
  { EFI_SMBIOS_TYPE_OEM_STRINGS, sizeof (SMBIOS_TABLE_TYPE11), 0 },
  1 // StringCount
};
CHAR8                *gSmbiosType11Strings[] = {
  "https://svn.code.sf.net/p/edk2/code/trunk/edk2/EmulatorPkg/",
  NULL
};

SMBIOS_TABLE_TYPE12  gSmbiosType12Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS, sizeof (SMBIOS_TABLE_TYPE12), 0 },
  1 // StringCount
};
CHAR8                *gSmbiosType12Strings[] = {
  "https://svn.code.sf.net/p/edk2/code/trunk/edk2/EmulatorPkg/EmulatorPkg.dsc",
  NULL
};

SMBIOS_TABLE_TYPE16  gSmbiosType16Template = {
  { EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY, sizeof (SMBIOS_TABLE_TYPE16), 0 },
  MemoryArrayLocationSystemBoard, // Location;                       ///< The enumeration value from MEMORY_ARRAY_LOCATION.
  MemoryArrayUseSystemMemory,     // Use;                            ///< The enumeration value from MEMORY_ARRAY_USE.
  MemoryErrorCorrectionUnknown,   // MemoryErrorCorrection;          ///< The enumeration value from MEMORY_ERROR_CORRECTION.
  0x80000000,                     // MaximumCapacity;
  0xFFFE,                         // MemoryErrorInformationHandle;
  1,                              // NumberOfMemoryDevices;
  0x3fffffffffffffffULL,          // ExtendedMaximumCapacity;
};

SMBIOS_TABLE_TYPE17  gSmbiosType17Template = {
  { EFI_SMBIOS_TYPE_MEMORY_DEVICE, sizeof (SMBIOS_TABLE_TYPE17), 0 },
  0,                       // MemoryArrayHandle;
  0xFFFE,                  // MemoryErrorInformationHandle;
  0xFFFF,                  // TotalWidth;
  0xFFFF,                  // DataWidth;
  0xFFFF,                  // Size;
  MemoryFormFactorUnknown, // FormFactor;                     ///< The enumeration value from MEMORY_FORM_FACTOR.
  0xff,                    // DeviceSet;
  1,                       // DeviceLocator String
  2,                       // BankLocator String
  MemoryTypeDram,          // MemoryType;                     ///< The enumeration value from MEMORY_DEVICE_TYPE.
  {     // TypeDetail;
    0,  // Reserved        :1;
    0,  // Other           :1;
    1,  // Unknown         :1;
    0,  // FastPaged       :1;
    0,  // StaticColumn    :1;
    0,  // PseudoStatic    :1;
    0,  // Rambus          :1;
    0,  // Synchronous     :1;
    0,  // Cmos            :1;
    0,  // Edo             :1;
    0,  // WindowDram      :1;
    0,  // CacheDram       :1;
    0,  // Nonvolatile     :1;
    0,  // Registered      :1;
    0,  // Unbuffered      :1;
    0,  // Reserved1       :1;
  },
  0,          // Speed;
  3,          // Manufacturer String
  0,          // SerialNumber String
  0,          // AssetTag String
  0,          // PartNumber String
  0,          // Attributes;
  0,          // ExtendedSize;
  0,          // ConfiguredMemoryClockSpeed;
};
CHAR8                *gSmbiosType17Strings[] = {
  "OS Virtual Memory",
  "malloc",
  "OSV",
  NULL
};

SMBIOS_TABLE_TYPE23  gSmbiosType23Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, sizeof (SMBIOS_TABLE_TYPE23), 0 },
  0,  // Capabilities;
  0,  // ResetCount;
  0,  // ResetLimit;
  0,  // TimerInterval;
  0   // Timeout;
};

SMBIOS_TABLE_TYPE32  gSmbiosType32Template = {
  { EFI_SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION, sizeof (SMBIOS_TABLE_TYPE32), 0 },
  { 0,                                       0,                            0, 0, 0, 0}, // Reserved[6];
  BootInformationStatusNoError                                                  // BootStatus
};

SMBIOS_TEMPLATE_ENTRY  gSmbiosTemplate[] = {
  { (SMBIOS_STRUCTURE *)&gSmbiosType0Template,  gSmbiosType0Strings  },
  { (SMBIOS_STRUCTURE *)&gSmbiosType1Template,  gSmbiosType1Strings  },
  { (SMBIOS_STRUCTURE *)&gSmbiosType2Template,  gSmbiosType2Strings  },
  { (SMBIOS_STRUCTURE *)&gSmbiosType3Template,  gSmbiosType3Strings  },
  { (SMBIOS_STRUCTURE *)&gSmbiosType8Template1, gSmbiosType8Strings1 },
  { (SMBIOS_STRUCTURE *)&gSmbiosType8Template2, gSmbiosType8Strings2 },
  { (SMBIOS_STRUCTURE *)&gSmbiosType8Template3, gSmbiosType8Strings3 },
  { (SMBIOS_STRUCTURE *)&gSmbiosType8Template4, gSmbiosType8Strings4 },
  { (SMBIOS_STRUCTURE *)&gSmbiosType8Template5, gSmbiosType8Strings5 },
  { (SMBIOS_STRUCTURE *)&gSmbiosType8Template6, gSmbiosType8Strings6 },
  { (SMBIOS_STRUCTURE *)&gSmbiosType8Template7, gSmbiosType8Strings7 },
  { (SMBIOS_STRUCTURE *)&gSmbiosType8Template8, gSmbiosType8Strings8 },
  { (SMBIOS_STRUCTURE *)&gSmbiosType8Template9, gSmbiosType8Strings9 },
  { (SMBIOS_STRUCTURE *)&gSmbiosType9Template,  gSmbiosType9Strings  },
  { (SMBIOS_STRUCTURE *)&gSmbiosType11Template, gSmbiosType11Strings },
  { (SMBIOS_STRUCTURE *)&gSmbiosType12Template, gSmbiosType12Strings },
  { (SMBIOS_STRUCTURE *)&gSmbiosType16Template, NULL                 },
  { (SMBIOS_STRUCTURE *)&gSmbiosType17Template, gSmbiosType17Strings },
  { (SMBIOS_STRUCTURE *)&gSmbiosType23Template, NULL                 },
  { (SMBIOS_STRUCTURE *)&gSmbiosType32Template, NULL                 },
  { NULL,                                       NULL                 }
};
