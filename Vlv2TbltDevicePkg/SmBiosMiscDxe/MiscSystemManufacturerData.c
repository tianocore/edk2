/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  MiscSystemManufacturerData.c

Abstract:

  Static data of System manufacturer information.
  System manufacturer information is Misc. subclass type 3 and SMBIOS type 1.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) System Manufacturer data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_SYSTEM_MANUFACTURER_DATA, MiscSystemManufacturer)
= {
  STRING_TOKEN(STR_MISC_SYSTEM_MANUFACTURER),  // SystemManufactrurer
  STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME),  // SystemProductName
  STRING_TOKEN(STR_MISC_SYSTEM_VERSION),       // SystemVersion
  STRING_TOKEN(STR_MISC_SYSTEM_SERIAL_NUMBER), // SystemSerialNumber
  {                                            // SystemUuid
  	//
    // Undefined GUID but can be programmed later.
    //0xFFFFFFFF, 0xFFFF, 0xFFFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF

    // Undefined GUID that cannot be programmed later.
    //0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	// TODO Hard code here for WHCT test.
	0xa5000288, 0x6462, 0x4524, 0x98, 0x6a, 0x9b, 0x77, 0x37, 0xe3, 0x15, 0xcf
	//
  },
  EfiSystemWakeupTypePowerSwitch  // SystemWakeupType
};
