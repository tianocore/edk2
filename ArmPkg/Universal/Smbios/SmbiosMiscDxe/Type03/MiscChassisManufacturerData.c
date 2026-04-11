/** @file
  This file provides Smbios Type3 Data

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosMisc.h"

//
// Static (possibly build generated) Chassis Manufacturer data.
//
SMBIOS_MISC_TABLE_DATA (SMBIOS_TABLE_TYPE3, MiscChassisManufacturer) = {
  {                                                       // Hdr
    EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE,                     // Type,
    0,                                                    // Length,
    0                                                     // Handle
  },
  1,                                                      // Manufacturer
  MiscChassisTypeMainServerChassis,                       // Type
  2,                                                      // Version
  3,                                                      // SerialNumber
  4,                                                      // AssetTag
  ChassisStateSafe,                                       // BootupState
  ChassisStateSafe,                                       // PowerSupplyState
  ChassisStateSafe,                                       // ThermalState
  ChassisSecurityStatusNone,                              // SecurityState
  {
    0,                                                    // OemDefined[0]
    0,                                                    // OemDefined[1]
    0,                                                    // OemDefined[2]
    0                                                     // OemDefined[3]
  },
  2,                                                      // Height
  1,                                                      // NumberofPowerCords
  0,                                                      // ContainedElementCount
  0,                                                      // ContainedElementRecordLength
  {                                                       // ContainedElements[0]
    {
      0,                                                    // ContainedElementType
      0,                                                    // ContainedElementMinimum
      0                                                     // ContainedElementMaximum
    }
  }
};
