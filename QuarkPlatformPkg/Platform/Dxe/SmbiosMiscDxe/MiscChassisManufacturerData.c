/** @file
This driver parses the mMiscSubclassDataTable structure and reports
any generated data to the DataHub.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/


#include "CommonHeader.h"

#include "SmbiosMisc.h"


//
// Static (possibly build generated) Chassis Manufacturer data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_CHASSIS_MANUFACTURER, MiscChassisManufacturer) = {
  STRING_TOKEN(STR_MISC_CHASSIS_MANUFACTURER),  // ChassisManufactrurer
  STRING_TOKEN(STR_MISC_CHASSIS_VERSION),       // ChassisVersion
  STRING_TOKEN(STR_MISC_CHASSIS_SERIAL_NUMBER), // ChassisSerialNumber
  STRING_TOKEN(STR_MISC_CHASSIS_ASSET_TAG),     // ChassisAssetTag
  {                                             // ChassisTypeStatus
    EfiMiscChassisTypeDeskTop,                  // ChassisType
    0,                                          // ChassisLockPresent
    0                                           // Reserved
  },
  EfiChassisStateSafe,                          // ChassisBootupState
  EfiChassisStateSafe,                          // ChassisPowerSupplyState
  EfiChassisStateOther,                         // ChassisThermalState
  EfiChassisSecurityStatusOther,                // ChassisSecurityState
  0                                             // ChassisOemDefined
};
