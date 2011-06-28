/*++

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MiscSystemManufacturerData.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubClassDriver.h"

//
// Static (possibly build generated) System Manufacturer data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_MANUFACTURER_DATA, MiscSystemManufacturer)
= {
  STRING_TOKEN(STR_MISC_SYSTEM_MANUFACTURER),
  // SystemManufactrurer
  STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME),
  // SystemProductName
  STRING_TOKEN(STR_MISC_SYSTEM_VERSION),
  // SystemVersion
  STRING_TOKEN(STR_MISC_SYSTEM_SERIAL_NUMBER),
  // SystemSerialNumber
  {
    0xbadfaced,
    0xdead,
    0xbeef,
    {
      0x13,
      0x13,
      0x13,
      0x13,
      0x13,
      0x13,
      0x13,
      0x13
    }
  },
  // SystemUuid
  EfiSystemWakeupTypePowerSwitch  // SystemWakeupType
};

/* eof - MiscSystemManufacturerData.c */
