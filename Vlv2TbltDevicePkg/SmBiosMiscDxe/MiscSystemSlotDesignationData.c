/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

    MiscSystemSlotDesignationData.c

Abstract:

  Static data of System Slot Designation.
  System Slot Designation is Misc. subclass type 7 and SMBIOS type 9.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"


//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx16Slot1) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT_PCIEX16_1),    // SlotDesignation
  EfiSlotTypePciExpress,        // SlotType
  EfiSlotDataBusWidth16xOrx16,  // SlotDataBusWidth
  EfiSlotUsageAvailable,        // SlotUsage
  EfiSlotLengthShort,           // SlotLength
  0x06,                         // SlotId
  {                             // SlotCharacteristics
    0,                          // CharacteristicsUnknown  :1;
    0,                          // Provides50Volts         :1;
    1,                          // Provides33Volts         :1;
    0,                          // SharedSlot              :1;
    0,                          // PcCard16Supported       :1;
    0,                          // CardBusSupported        :1;
    0,                          // ZoomVideoSupported      :1;
    0,                          // ModemRingResumeSupported:1;
    0,                          // PmeSignalSupported      :1;
    0,                          // HotPlugDevicesSupported :1;
    0,                          // SmbusSignalSupported    :1;
    0                           // Reserved                :21;
  },
  0                             // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx16Slot2) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT_PCIEX16_2),    // SlotDesignation
  EfiSlotTypePciExpress,        // SlotType
  EfiSlotDataBusWidth16xOrx16,  // SlotDataBusWidth
  EfiSlotUsageAvailable,        // SlotUsage
  EfiSlotLengthShort,           // SlotLength
  0x04,                         // SlotId
  {                             // SlotCharacteristics
    0,                          // CharacteristicsUnknown  :1;
    0,                          // Provides50Volts         :1;
    1,                          // Provides33Volts         :1;
    0,                          // SharedSlot              :1;
    0,                          // PcCard16Supported       :1;
    0,                          // CardBusSupported        :1;
    0,                          // ZoomVideoSupported      :1;
    0,                          // ModemRingResumeSupported:1;
    0,                          // PmeSignalSupported      :1;
    0,                          // HotPlugDevicesSupported :1;
    0,                          // SmbusSignalSupported    :1;
    0                           // Reserved                :21;
  },
  0                             // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx4) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT_PCIEX4),    // SlotDesignation
  EfiSlotTypePciExpress,        // SlotType
  EfiSlotDataBusWidth4xOrx4,  // SlotDataBusWidth
  EfiSlotUsageAvailable,        // SlotUsage
  EfiSlotLengthShort,           // SlotLength
  0x03,                         // SlotId
  {                             // SlotCharacteristics
    0,                          // CharacteristicsUnknown  :1;
    0,                          // Provides50Volts         :1;
    1,                          // Provides33Volts         :1;
    0,                          // SharedSlot              :1;
    0,                          // PcCard16Supported       :1;
    0,                          // CardBusSupported        :1;
    0,                          // ZoomVideoSupported      :1;
    0,                          // ModemRingResumeSupported:1;
    0,                          // PmeSignalSupported      :1;
    0,                          // HotPlugDevicesSupported :1;
    0,                          // SmbusSignalSupported    :1;
    0                           // Reserved                :21;
  },
  0                             // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx1Slot1) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT_PCIEX1_1),    // SlotDesignation
  EfiSlotTypePciExpress,        // SlotType
  EfiSlotDataBusWidth1xOrx1,    // SlotDataBusWidth
  EfiSlotUsageAvailable,        // SlotUsage
  EfiSlotLengthShort,           // SlotLength
  0x02,                         // SlotId
  {                             // SlotCharacteristics
    0,                          // CharacteristicsUnknown  :1;
    0,                          // Provides50Volts         :1;
    1,                          // Provides33Volts         :1;
    0,                          // SharedSlot              :1;
    0,                          // PcCard16Supported       :1;
    0,                          // CardBusSupported        :1;
    0,                          // ZoomVideoSupported      :1;
    0,                          // ModemRingResumeSupported:1;
    1,                          // PmeSignalSupported      :1;
    0,                          // HotPlugDevicesSupported :1;
    1,                          // SmbusSignalSupported    :1;
    0                           // Reserved                :21;
  },
  0                             // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx1Slot2) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT_PCIEX1_2),    // SlotDesignation
  EfiSlotTypePciExpress,        // SlotType
  EfiSlotDataBusWidth1xOrx1,    // SlotDataBusWidth
  EfiSlotUsageAvailable,        // SlotUsage
  EfiSlotLengthShort,           // SlotLength
  0x15,                         // SlotId
  {                             // SlotCharacteristics
    0,                          // CharacteristicsUnknown  :1;
    0,                          // Provides50Volts         :1;
    1,                          // Provides33Volts         :1;
    0,                          // SharedSlot              :1;
    0,                          // PcCard16Supported       :1;
    0,                          // CardBusSupported        :1;
    0,                          // ZoomVideoSupported      :1;
    0,                          // ModemRingResumeSupported:1;
    1,                          // PmeSignalSupported      :1;
    0,                          // HotPlugDevicesSupported :1;
    1,                          // SmbusSignalSupported    :1;
    0                           // Reserved                :21;
  },
  0                             // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCIEx1Slot3) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT_PCIEX1_3),    // SlotDesignation
  EfiSlotTypePciExpress,        // SlotType
  EfiSlotDataBusWidth1xOrx1,    // SlotDataBusWidth
  EfiSlotUsageAvailable,        // SlotUsage
  EfiSlotLengthShort,           // SlotLength
  0x16,                         // SlotId
  {                             // SlotCharacteristics
    0,                          // CharacteristicsUnknown  :1;
    0,                          // Provides50Volts         :1;
    1,                          // Provides33Volts         :1;
    0,                          // SharedSlot              :1;
    0,                          // PcCard16Supported       :1;
    0,                          // CardBusSupported        :1;
    0,                          // ZoomVideoSupported      :1;
    0,                          // ModemRingResumeSupported:1;
    1,                          // PmeSignalSupported      :1;
    0,                          // HotPlugDevicesSupported :1;
    1,                          // SmbusSignalSupported    :1;
    0                           // Reserved                :21;
  },
  0                             // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCI1) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT_PCI1),    // SlotDesignation
  EfiSlotTypePci,               // SlotType
  EfiSlotDataBusWidth32Bit,     // SlotDataBusWidth
  EfiSlotUsageAvailable,        // SlotUsage
  EfiSlotLengthLong ,           // SlotLength
  0x07,                         // SlotId
  {                             // SlotCharacteristics
    0,                          // CharacteristicsUnknown  :1;
    0,                          // Provides50Volts         :1;
    1,                          // Provides33Volts         :1;
    0,                          // SharedSlot              :1;
    0,                          // PcCard16Supported       :1;
    0,                          // CardBusSupported        :1;
    0,                          // ZoomVideoSupported      :1;
    0,                          // ModemRingResumeSupported:1;
    1,                          // PmeSignalSupported      :1;
    0,                          // HotPlugDevicesSupported :1;
    1,                          // SmbusSignalSupported    :1;
    0                           // Reserved                :21;
  },
  0                             // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCI2) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT_PCI2),    // SlotDesignation
  EfiSlotTypePci,               // SlotType
  EfiSlotDataBusWidth32Bit,     // SlotDataBusWidth
  EfiSlotUsageAvailable,        // SlotUsage
  EfiSlotLengthLong ,           // SlotLength
  0x18,                         // SlotId
  {                             // SlotCharacteristics
    0,                          // CharacteristicsUnknown  :1;
    0,                          // Provides50Volts         :1;
    1,                          // Provides33Volts         :1;
    0,                          // SharedSlot              :1;
    0,                          // PcCard16Supported       :1;
    0,                          // CardBusSupported        :1;
    0,                          // ZoomVideoSupported      :1;
    0,                          // ModemRingResumeSupported:1;
    1,                          // PmeSignalSupported      :1;
    0,                          // HotPlugDevicesSupported :1;
    1,                          // SmbusSignalSupported    :1;
    0                           // Reserved                :21;
  },
  0                             // SlotDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotPCI3) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT_PCI3),    // SlotDesignation
  EfiSlotTypePci,               // SlotType
  EfiSlotDataBusWidth32Bit,     // SlotDataBusWidth
  EfiSlotUsageAvailable,        // SlotUsage
  EfiSlotLengthLong ,           // SlotLength
  0x17,                         // SlotId
  {                             // SlotCharacteristics
    0,                          // CharacteristicsUnknown  :1;
    0,                          // Provides50Volts         :1;
    1,                          // Provides33Volts         :1;
    0,                          // SharedSlot              :1;
    0,                          // PcCard16Supported       :1;
    0,                          // CardBusSupported        :1;
    0,                          // ZoomVideoSupported      :1;
    0,                          // ModemRingResumeSupported:1;
    1,                          // PmeSignalSupported      :1;
    0,                          // HotPlugDevicesSupported :1;
    1,                          // SmbusSignalSupported    :1;
    0                           // Reserved                :21;
  },
  0                             // SlotDevicePath
};

