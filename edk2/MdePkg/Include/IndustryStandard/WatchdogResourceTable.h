/** 
	@file   
  ACPI Watchdog Resource Table as defined at
  Microsoft Hardware Watchdog Timer Specification.

  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
**/

#ifndef _WATCHDOG_RESOURCE_TABLE_H_
#define _WATCHDOG_RESOURCE_TABLE_H_

//
// Include files
//
#include "Acpi2_0.h"

//
// Ensure proper structure formats
//
#pragma pack(1)
//
// Watchdog Resource Table definition.
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER             Header;
  EFI_ACPI_2_0_GENERIC_ADDRESS_STRUCTURE  ControlRegisterAddress;
  EFI_ACPI_2_0_GENERIC_ADDRESS_STRUCTURE  CountRegisterAddress;
  UINT16                                  PCIDeviceID;
  UINT16                                  PCIVendorID;
  UINT8                                   PCIBusNumber;
  UINT8                                   PCIDeviceNumber;
  UINT8                                   PCIFunctionNumber;
  UINT8                                   PCISegment;
  UINT16                                  MaxCount;
  UINT8                                   Units;
} EFI_ACPI_WATCHDOG_RESOURCE_1_0_TABLE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER             Header;
  UINT32                                  WatchdogHeaderLength;
  UINT8                                   PCISegment;
  UINT8                                   PCIBusNumber;
  UINT8                                   PCIDeviceNumber;
  UINT8                                   PCIFunctionNumber;
  UINT32                                  TimerPeriod;
  UINT32                                  MaxCount;
  UINT32                                  MinCount;
  UINT8                                   WatchdogFlags;  
  UINT8                                   Reserved_57[3];  
  UINT32                                  NumberWatchdogInstructionEntries;
} EFI_ACPI_WATCHDOG_RESOURCE_2_0_TABLE;

typedef struct {
  UINT8                                   WatchdogAction;
  UINT8                                   InstructionFlags;
  UINT8                                   Reserved_2;
  UINT8                                   RegisterSize;
  EFI_ACPI_2_0_GENERIC_ADDRESS_STRUCTURE  RegisterRegion;
  UINT32                                  Value;
  UINT32                                  Mask;
} EFI_ACPI_WATCHDOG_RESOURCE_2_0_WATCHDOG_ACTION_INSTRUCTION_ENTRY;

#pragma pack()

//
// WDRT Revision (defined in spec)
//
#define EFI_ACPI_WATCHDOG_RESOURCE_1_0_TABLE_REVISION  0x01
#define EFI_ACPI_WATCHDOG_RESOURCE_2_0_TABLE_REVISION  0x02

//
// WDRT 1.0 Count Unit
//
#define EFI_ACPI_WDRT_1_0_COUNT_UNIT_1_SEC_PER_COUNT        1
#define EFI_ACPI_WDRT_1_0_COUNT_UNIT_100_MILLISEC_PER_COUNT 2
#define EFI_ACPI_WDRT_1_0_COUNT_UNIT_10_MILLISEC_PER_COUNT  3

//
// WDRT 2.0 Flags
//
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ENABLED                0x1
#define EFI_ACPI_WDRT_2_0_WATCHDOG_STOPPED_IN_SLEEP_STATE 0x80

//
// WDRT 2.0 Watchdog Actions
//
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_RESET                          0x1
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_QUERY_CURRENT_COUNTDOWN_PERIOD 0x4
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_QUERY_COUNTDOWN_PERIOD         0x5
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_SET_COUNTDOWN_PERIOD           0x6
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_QUERY_RUNNING_STATE            0x8
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_SET_RUNNING_STATE              0x9
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_QUERY_STOPPED_STATE            0xA
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_SET_STOPPED_STATE              0xB
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_QUERY_ REBOOT                  0x10
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_SET_REBOOT                     0x11
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_QUERY_SHUTDOWN                 0x12
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_SET_SHUTDOWN                   0x13
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_QUERY_WATCHDOG_STATUS          0x20
#define EFI_ACPI_WDRT_2_0_WATCHDOG_ACTION_SET_WATCHDOG_STATUS            0x21

//
// WDRT 2.0 Watchdog Action Entry Instruction Flags
//
#define EFI_ACPI_WDRT_2_0_WATCHDOG_INSTRUCTION_READ_VALUE        0x0
#define EFI_ACPI_WDRT_2_0_WATCHDOG_INSTRUCTION_READ_COUNTDOWN    0x1
#define EFI_ACPI_WDRT_2_0_WATCHDOG_INSTRUCTION_WRITE_VALUE       0x2
#define EFI_ACPI_WDRT_2_0_WATCHDOG_INSTRUCTION_WRITE_COUNTDOWN   0x3
#define EFI_ACPI_WDRT_2_0_WATCHDOG_INSTRUCTION_PRESERVE_REGISTER 0x80

#endif
