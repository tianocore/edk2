/** @file
  Platform specific defines for constructing ACPI tables

  Copyright (c) 2012, 2013, Red Hat, Inc.
  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _Platform_h_INCLUDED_
#define _Platform_h_INCLUDED_

#include <PiDxe.h>
#include <IndustryStandard/Acpi.h>

//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_OEM_ID           'O','V','M','F',' ',' '   // OEMID 6 bytes long
#define EFI_ACPI_OEM_TABLE_ID     SIGNATURE_64('O','V','M','F','E','D','K','2') // OEM table id 8 bytes long
#define EFI_ACPI_OEM_REVISION     0x20130221
#define EFI_ACPI_CREATOR_ID       SIGNATURE_32('O','V','M','F')
#define EFI_ACPI_CREATOR_REVISION 0x00000099

#define SCI_INT_VECTOR  0x0009
#define SMI_CMD_IO_PORT 0xB2
#define ACPI_ENABLE     0xF1
#define ACPI_DISABLE    0xF0
#define S4BIOS_REQ      0x00
#define PM1a_EVT_BLK    0x0000b000
#define PM1a_CNT_BLK    0x0000b004
#define PM_TMR_BLK      0x0000b008
#define GPE0_BLK        0x0000afe0
#define PM1_EVT_LEN     0x04
#define PM1_CNT_LEN     0x02
#define PM_TM_LEN       0x04
#define GPE0_BLK_LEN    0x04
#define RESERVED        0x00
#define P_LVL2_LAT      0x0065
#define P_LVL3_LAT      0x03E9
#define FLUSH_SIZE      0x0000
#define FLUSH_STRIDE    0x0000
#define DUTY_OFFSET     0x00
#define DUTY_WIDTH      0x00
#define DAY_ALRM        0x00
#define MON_ALRM        0x00
#define CENTURY         0x00
#define FLAG            (EFI_ACPI_2_0_WBINVD | \
                         EFI_ACPI_2_0_PROC_C1 | \
                         EFI_ACPI_2_0_SLP_BUTTON | \
                         EFI_ACPI_2_0_RTC_S4 | \
                         EFI_ACPI_2_0_RESET_REG_SUP)
#define RESET_REG       0xCF9
#define RESET_VALUE     (BIT2 | BIT1) // PIIX3 Reset CPU + System Reset

//
// Byte-aligned IO port register block initializer for
// EFI_ACPI_2_0_GENERIC_ADDRESS_STRUCTURE
//
#define GAS2_IO(Base, Size) {                             \
          EFI_ACPI_2_0_SYSTEM_IO, /* AddressSpaceId    */ \
          (Size) * 8,             /* RegisterBitWidth  */ \
          0,                      /* RegisterBitOffset */ \
          0,                      /* Reserved          */ \
          (Base)                  /* Address           */ \
          }

#endif
