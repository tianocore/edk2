/** @file
  Platform specific defines for constructing ACPI tables

  Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
  Copyright (c) 2014, Pluribus Networks, Inc.
  Copyright (c) 2012, 2013, Red Hat, Inc.
  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _Platform_h_INCLUDED_
#define _Platform_h_INCLUDED_

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <IndustryStandard/SerialPortConsoleRedirectionTable.h>

//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_OEM_ID           'B','H','Y','V','E',' '   // OEMID 6 bytes long
#define EFI_ACPI_OEM_REVISION     0x1
#define EFI_ACPI_CREATOR_ID       SIGNATURE_32('B','H','Y','V')
#define EFI_ACPI_CREATOR_REVISION 0x00000001

#define INT_MODEL       0x01
#define SCI_INT_VECTOR  0x0009
#define SMI_CMD_IO_PORT 0xB2
#define ACPI_ENABLE     0xA0
#define ACPI_DISABLE    0xA1
#define S4BIOS_REQ      0x00
#define PM1a_EVT_BLK    0x00000400      /* TNXXX */
#define PM1b_EVT_BLK    0x00000000
#define PM1a_CNT_BLK    0x00000404      /* TNXXX */
#define PM1b_CNT_BLK    0x00000000
#define PM2_CNT_BLK     0x00000000
#define PM_TMR_BLK      0x00000408
#define GPE0_BLK        0x00000000
#define GPE1_BLK        0x00000000
#define PM1_EVT_LEN     0x04
#define PM1_CNT_LEN     0x02
#define PM2_CNT_LEN     0x00
#define PM_TM_LEN       0x04
#define GPE0_BLK_LEN    0x00
#define GPE1_BLK_LEN    0x00
#define GPE1_BASE       0x00
#define RESERVED        0x00
#define P_LVL2_LAT      0x0000
#define P_LVL3_LAT      0x0000
#define FLUSH_SIZE      0x0000
#define FLUSH_STRIDE    0x0000
#define DUTY_OFFSET     0x00
#define DUTY_WIDTH      0x00
#define DAY_ALRM        0x00
#define MON_ALRM        0x00
#define CENTURY         0x32
#define IAPC_BOOT_ARCH  0x12    /* 8042 present, disable PCIe ASPM */
#define FACP_FLAGS      (EFI_ACPI_1_0_WBINVD | EFI_ACPI_1_0_PROC_C1 |         \
                        EFI_ACPI_1_0_SLP_BUTTON | EFI_ACPI_1_0_TMR_VAL_EXT |  \
                        EFI_ACPI_2_0_RESET_REG_SUP |                          \
                        EFI_ACPI_3_0_FORCE_APIC_PHYSICAL_DESTINATION_MODE)
#define FACP_RESET_REG  {                                               \
        EFI_ACPI_3_0_SYSTEM_IO,         /* Address Space ID */          \
        8,                              /* Bit Width */                 \
        0,                              /* Bit Offset */                \
        EFI_ACPI_3_0_BYTE,              /* Byte Access */               \
        0xCF9                           /* I/O Port */                  \
}
#define FACP_RESET_VAL  0x6
#endif
