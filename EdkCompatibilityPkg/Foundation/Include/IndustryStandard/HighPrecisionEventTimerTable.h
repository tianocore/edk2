/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  HighPrecisionEventTimerTable.h

Abstract:

  ACPI high precision event timer table definition, defined at 
  Intel IA-PC HPET (High Precision Event Timers) Specification.

--*/

#ifndef _HIGH_PRECISION_EVENT_TIMER_TABLE_H_
#define _HIGH_PRECISION_EVENT_TIMER_TABLE_H_

//
// Include files
//
#include "Acpi2_0.h"

//
// Ensure proper structure formats
//
#pragma pack(1)
//
// High Precision Event Timer Table header definition.
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER             Header;
  UINT32                                  EventTimerBlockId;
  EFI_ACPI_2_0_GENERIC_ADDRESS_STRUCTURE  BaseAddressLower32Bit;
  UINT8                                   HpetNumber;
  UINT16                                  MainCounterMinimumClockTickInPeriodicMode;
  UINT8                                   PageProtectionAndOemAttribute;
} EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER;

//
// HPET Revision (defined in spec)
//
#define EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_REVISION  0x01

//
// Page protection setting
// Values 3 through 15 are reserved for use by the specification
//
#define EFI_ACPI_NO_PAGE_PROTECTION   0
#define EFI_ACPI_4KB_PAGE_PROTECTION  1
#define EFI_ACPI_64KB_PAGE_PROTECTION 2

#pragma pack()

#endif
