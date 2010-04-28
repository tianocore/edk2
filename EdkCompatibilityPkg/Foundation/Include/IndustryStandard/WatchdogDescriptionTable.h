/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  WatchdogDescriptionTable.h

Abstract:

  ACPI Watchdog Description Table as defined in Intel
  ICH Family Watchdog Timer (WDT) Application Note (AP-725)

--*/

#ifndef _WATCH_DOG_DESCRIPTION_TABLE_H_
#define _WATCH_DOG_DESCRIPTION_TABLE_H_

//
// Include files
//
#include "Acpi2_0.h"

//
// Ensure proper structure formats
//
#pragma pack(1)
//
// WDDT structure
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER             Header;
  UINT16                                  SpecVersion;
  UINT16                                  TableVersion;
  UINT16                                  Vid;
  EFI_ACPI_2_0_GENERIC_ADDRESS_STRUCTURE  BaseAddress;
  UINT16                                  TimerMaxCount;
  UINT16                                  TimerMinCount;
  UINT16                                  TimerCountPeriod;
  UINT16                                  Status;
  UINT16                                  Capability;
} EFI_ACPI_1_0_WATCH_DOG_DESCRIPTION_TABLE;

//
// "WDDT" Watchdog Description Table signatures
//
#define EFI_ACPI_1_0_WDDT_SIGNATURE 0x54444457

#pragma pack()

//
// WDDT Revision
//
#define EFI_ACPI_WATCHDOG_DESCRIPTION_1_0_TABLE_REVISION  0x01

//
// WDDT Spec Version
//
#define EFI_ACPI_WDDT_SPEC_1_0_VERSION                0x01

//
// WDDT Description Table Version
//
#define EFI_ACPI_WDDT_TABLE_1_0_VERSION               0x01

//
// WDT Status
//
#define EFI_ACPI_WDDT_STATUS_AVAILABLE             0x0001
#define EFI_ACPI_WDDT_STATUS_ACTIVE                0x0002
#define EFI_ACPI_WDDT_STATUS_OWNED_BY_BIOS         0x0000
#define EFI_ACPI_WDDT_STATUS_OWNED_BY_OS           0x0004
#define EFI_ACPI_WDDT_STATUS_USER_RESET_EVENT      0x0800
#define EFI_ACPI_WDDT_STATUS_WDT_EVENT             0x1000
#define EFI_ACPI_WDDT_STATUS_POWER_FAIL_EVENT      0x2000
#define EFI_ACPI_WDDT_STATUS_UNKNOWN_RESET_EVENT   0x4000

//
// WDT Capability
//
#define EFI_ACPI_WDDT_CAPABILITY_AUTO_RESET            0x0001
#define EFI_ACPI_WDDT_CAPABILITY_ALERT_SUPPORT         0x0002
#define EFI_ACPI_WDDT_CAPABILITY_PLATFORM_SHUTDOWN     0x0004
#define EFI_ACPI_WDDT_CAPABILITY_IMMEDIATE_SHUTDOWN    0x0008
#define EFI_ACPI_WDDT_CAPABILITY_BIOS_HANDOFF_SUPPORT  0x0010

#endif
