/*++

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

    MiscSystemSlotDesignationData.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubClassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_SLOT_DESIGNATION_DATA, MiscSystemSlotDesignation) = {
  STRING_TOKEN(STR_MISC_SYSTEM_SLOT_DESIGNATION),   // SlotDesignation
  EfiSlotTypeOther,         // SlotType
  EfiSlotDataBusWidthOther, // SlotDataBusWidth
  EfiSlotUsageOther,        // SlotUsage
  EfiSlotLengthOther,       // SlotLength
  0,                        // SlotId
  {                         // SlotCharacteristics
    0,                      // CharacteristicsUnknown  :1;
    0,                      // Provides50Volts         :1;
    0,                      // Provides33Volts         :1;
    0,                      // SharedSlot              :1;
    0,                      // PcCard16Supported       :1;
    0,                      // CardBusSupported        :1;
    0,                      // ZoomVideoSupported      :1;
    0,                      // ModemRingResumeSupported:1;
    0,                      // PmeSignalSupported      :1;
    0,                      // HotPlugDevicesSupported :1;
    0,                      // SmbusSignalSupported    :1;
    0                       // Reserved                :21;
  },
  {0, 0, {0, 0}}            // SlotDevicePath
};

/* eof - MiscSystemSlotsData.c */
