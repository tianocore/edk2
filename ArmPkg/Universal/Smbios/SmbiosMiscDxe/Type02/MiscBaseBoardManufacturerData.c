/** @file

  This file provide OEM to define Smbios Type2 Data

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
SMBIOS_MISC_TABLE_DATA(SMBIOS_TABLE_TYPE2, MiscBaseBoardManufacturer) = {
  {                                                       // Hdr
    EFI_SMBIOS_TYPE_BASEBOARD_INFORMATION,                // Type,
    0,                                                    // Length,
    0                                                     // Handle
  },
  1,                                                      // BaseBoardManufacturer
  2,                                                      // BaseBoardProductName
  3,                                                      // BaseBoardVersion
  4,                                                      // BaseBoardSerialNumber
  5,                                                      // BaseBoardAssetTag
  {                                                       // FeatureFlag
    1,                                                    // Motherboard           :1
    0,                                                    // RequiresDaughterCard  :1
    0,                                                    // Removable             :1
    1,                                                    // Replaceable           :1
    0,                                                    // HotSwappable          :1
    0                                                     // Reserved              :3
  },
  6,                                                      // BaseBoardChassisLocation
  0,                                                      // ChassisHandle;
  BaseBoardTypeMotherBoard,                               // BoardType;
  0,                                                      // NumberOfContainedObjectHandles;
  {
    0
  }                                                       // ContainedObjectHandles[1];
};
