/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

    Configuration.h

Abstract:

    Driver configuration include file

Revision History:
  ------------------------------------------------------------------------------
  Rev   Date<MM/DD/YYYY>    Name    Description
  ------------------------------------------------------------------------------

  ------------------------------------------------------------------------------
--*/

#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

//
// System Setup Page. Do not have to be sequential but have to be unique
//
#define ROOT_FORM_ID                        1
#define ROOT_MAIN_FORM_ID                   2
#define CPU_CONFIGURATION_FORM_ID           3
#define CPU_PWR_CONFIGURATION_FORM_ID       4
#define BOOT_CONFIGURATION_FORM_ID          5
#define IGD_FORM_ID                         6
#define SECURITY_CONFIGURATION_FORM_ID      7
#define SOUTH_CLUSTER_FORM_ID               8
#define DPTF_FORM_ID                        9
#define PLATFORM_INFORMATION_FORM_ID        10
#define DRIVE_CONFIGURATION_ID              11
#define SENSOR_CONFIGURATION_ID             12
#define LPSS_CONFIGURATION_ID               13
#define UNCORE_FORM_ID                      14
#define TPM_FORM_ID                         15
#define THERMAL_FORM_ID                     16
#define PASSWORD_SETTING_ID                 17
#define LAN_OPTIONS_FORM_ID                 18
#define AZALIA_OPTIONS_FORM_ID              19
#define MISC_OPTIONS_FORM_ID                20
#define USB_OPTIONS_FORM_ID                 21
#define PCIE_DEVICE_OPTIONS_FORM_ID         22
#define SYSTEM_COMPONENT_FORM_ID            23
#define DEBUG_CONFIGURATION_FORM_ID         24
#endif // #ifndef _CONFIGURATION_H
