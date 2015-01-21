/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

  AcpiTablePlatform.h


Abstract: File contains platform specific ACPI defines for use in ACPI tables


--*/
#ifndef _Platform_h_INCLUDED_
#define _Platform_h_INCLUDED_

#ifdef ECP_FLAG
#include "EdkIIGlueDxe.h"
#endif
#include <IndustryStandard/Acpi.h>
//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_OEM_ID           'O','E','M','I','D',' '   // OEMID 6 bytes long
#define EFI_ACPI_OEM_TABLE_ID     SIGNATURE_64('O','E','M','T','A','B','L','E') // OEM table id 8 bytes long
#define EFI_ACPI_OEM_REVISION     0x00000005
#define EFI_ACPI_CREATOR_ID       SIGNATURE_32('C','R','E','A')
#define EFI_ACPI_CREATOR_REVISION 0x0100000D
#define INT_MODEL       0x01
#define PM_PROFILE      EFI_ACPI_4_0_PM_PROFILE_MOBILE
#define SCI_INT_VECTOR  0x0009
#define SMI_CMD_IO_PORT 0x000000B2
#define ACPI_ENABLE     0x0A0
#define ACPI_DISABLE    0x0A1
#define S4BIOS_REQ      0x00
#define PSTATE_CNT      0x00
#define PM1a_EVT_BLK    0x00000400
#define PM1b_EVT_BLK    0x00000000
#define PM1a_CNT_BLK    0x00000404
#define PM1b_CNT_BLK    0x00000000
#define PM2_CNT_BLK     0x00000450
#define PM_TMR_BLK      0x00000408
#define GPE0_BLK        0x00000420
#define GPE1_BLK        0x00000000
#define PM1_EVT_LEN     0x04
#define PM1_CNT_LEN     0x02
#define PM2_CNT_LEN     0x01
#define PM_TM_LEN       0x04
#define GPE0_BLK_LEN    0x10
#define GPE1_BLK_LEN    0x00
#define GPE1_BASE       0x00
#define CST_CNT         0x00
#define P_LVL2_LAT      0x0064
#define P_LVL3_LAT      0x01F4
#define FLUSH_SIZE      0x0400
#define FLUSH_STRIDE    0x0010
#define DUTY_OFFSET     0x01
#define DUTY_WIDTH      0x03
#define DAY_ALRM        0x0D
#define MON_ALRM        0x00
#define CENTURY         0x32
#define FLAG            ( EFI_ACPI_4_0_WBINVD | EFI_ACPI_4_0_SLP_BUTTON | EFI_ACPI_4_0_RESET_REG_SUP | EFI_ACPI_4_0_RTC_S4)
#define IAPC_BOOT_ARCH  ( EFI_ACPI_4_0_VGA_NOT_PRESENT | EFI_ACPI_4_0_8042 | EFI_ACPI_4_0_LEGACY_DEVICES)
#define RESERVED        0x00

#endif
