/** @file
This driver parses the mMiscSubclassDataTable structure and reports
any generated data using smbios protocol.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/


#include "CommonHeader.h"

#include "SmbiosMisc.h"


//
// Static (possibly build generated) System Manufacturer data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_MANUFACTURER, MiscSystemManufacturer) = {
  STRING_TOKEN(STR_MISC_SYSTEM_MANUFACTURER),   // SystemManufactrurer
  STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME),   // SystemProductName
  STRING_TOKEN(STR_MISC_SYSTEM_VERSION),        // SystemVersion
  STRING_TOKEN(STR_MISC_SYSTEM_SERIAL_NUMBER),  // SystemSerialNumber
  {                                             // SystemUuid
    0x13ffef23, 0x8654, 0x46da, {0xa4, 0x7, 0x39, 0xc9, 0x12, 0x2, 0xd3, 0x56}
  },
  EfiSystemWakeupTypePowerSwitch,               // SystemWakeupType
  STRING_TOKEN(STR_MISC_SYSTEM_SKU_NUMBER),     // SystemSKUNumber
  STRING_TOKEN(STR_MISC_SYSTEM_FAMILY),         // SystemFamily
};
