/** @file
  This file provides Smbios Type1 Data

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosMisc.h"

//
// Static (possibly build generated) System Manufacturer data.
//
SMBIOS_MISC_TABLE_DATA (SMBIOS_TABLE_TYPE1, MiscSystemManufacturer) = {
  {                                               // Hdr
    EFI_SMBIOS_TYPE_SYSTEM_INFORMATION,           // Type,
    0,                                            // Length,
    0                                             // Handle
  },
  1,                                              // Manufacturer
  2,                                              // ProductName
  3,                                              // Version
  4,                                              // SerialNumber
  {                                               // Uuid
    0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
  },
  SystemWakeupTypePowerSwitch,                    // SystemWakeupType
  5,                                              // SKUNumber,
  6                                               // Family
};
