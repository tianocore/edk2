/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  MiscChassisManufacturerData.c

Abstract:

  Static data is Chassis Manufacturer information.
  Chassis Manufacturer information is Misc. subclass type 5 and SMBIOS type 3.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Chassis Manufacturer data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_CHASSIS_MANUFACTURER_DATA, MiscChassisManufacturer)
= {
  STRING_TOKEN(STR_MISC_CHASSIS_MANUFACTURER),  // ChassisManufactrurer
  STRING_TOKEN(STR_MISC_CHASSIS_VERSION),       // ChassisVersion
  STRING_TOKEN(STR_MISC_CHASSIS_SERIAL_NUMBER), // ChassisSerialNumber
  STRING_TOKEN(STR_MISC_CHASSIS_ASSET_TAG),     // ChassisAssetTag
  {                               // ChassisTypeStatus
    EfiMiscChassisTypeUnknown,    // ChassisType
    0,                            // ChassisLockPresent
    0                             // Reserved
  },
  EfiChassisStateSafe,            // ChassisBootupState
  EfiChassisStateSafe,            // ChassisPowerSupplyState
  EfiChassisStateOther,           // ChassisThermalState
  EfiChassisSecurityStatusOther,  // ChassisSecurityState
  0,                              // ChassisOemDefined
  0,                              // ChassisHeight
  0,                              // ChassisNumberPowerCords
  0,                              // ChassisElementCount
  0,                              // ChassisElementRecordLength
  {                               // ChassisElements
    {0, 0, 0},                    // ChassisElementType
    0,                            // ChassisElementStructure
    EfiBaseBoardTypeUnknown,      // ChassisBaseBoard
    0,                            // ChassisElementMinimum
    0                             // ChassisElementMaximum
  },
};
