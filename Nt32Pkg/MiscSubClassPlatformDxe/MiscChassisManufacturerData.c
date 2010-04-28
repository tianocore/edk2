/**@file

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MiscChassisManufacturerData.c
  
Abstract: 

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Chassis Manufacturer data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_CHASSIS_MANUFACTURER_DATA, MiscChassisManufacturer) = {
  STRING_TOKEN(STR_MISC_CHASSIS_MANUFACTURER),  // ChassisManufactrurer
  STRING_TOKEN(STR_MISC_CHASSIS_VERSION),       // ChassisVersion
  STRING_TOKEN(STR_MISC_CHASSIS_SERIAL_NUMBER), // ChassisSerialNumber
  STRING_TOKEN(STR_MISC_CHASSIS_ASSET_TAG),     // ChassisAssetTag
  {                               // ChassisTypeStatus
    EfiMiscChassisTypeOther,      // ChassisType
    0,                            // ChassisLockPresent
    0                             // Reserved
  },
  EfiChassisStateOther,           // ChassisBootupState
  EfiChassisStateOther,           // ChassisPowerSupplyState
  EfiChassisStateOther,           // ChassisThermalState
  EfiChassisSecurityStatusOther,  // ChassisSecurityState
  0                               // ChassisOemDefined
};

/* eof - MiscChassisManufacaturerData.c */
