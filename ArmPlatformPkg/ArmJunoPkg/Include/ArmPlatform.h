/** @file
*
*  Copyright (c) 2013-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __ARM_JUNO_H__
#define __ARM_JUNO_H__

#include <VExpressMotherBoard.h>

/***********************************************************************************
// Platform Memory Map
************************************************************************************/

// Motherboard Peripheral and On-chip peripheral
#define ARM_VE_BOARD_PERIPH_BASE              0x1C010000

// NOR Flash 0
#define ARM_VE_SMB_NOR0_BASE                  0x08000000
#define ARM_VE_SMB_NOR0_SZ                    SIZE_64MB

// Off-Chip peripherals (USB, Ethernet, VRAM)
#define ARM_VE_SMB_PERIPH_BASE                0x18000000
#define ARM_VE_SMB_PERIPH_SZ                  (SIZE_64MB + SIZE_2MB)

// On-Chip non-secure ROM
#define ARM_JUNO_NON_SECURE_ROM_BASE          0x1F000000
#define ARM_JUNO_NON_SECURE_ROM_SZ            SIZE_16MB

// On-Chip Peripherals
#define ARM_JUNO_PERIPHERALS_BASE             0x20000000
#define ARM_JUNO_PERIPHERALS_SZ               0x0E000000

#define ARM_JUNO_GIV2M_MSI_BASE               0x2c1c0000
#define ARM_JUNO_GIV2M_MSI_SZ                 SIZE_256KB

// On-Chip non-secure SRAM
#define ARM_JUNO_NON_SECURE_SRAM_BASE         0x2E000000
#define ARM_JUNO_NON_SECURE_SRAM_SZ           SIZE_16MB

// SOC peripherals (HDLCD, UART, I2C, I2S, USB, SMC-PL354, etc)
#define ARM_JUNO_SOC_PERIPHERALS_BASE         0x7FF50000
#define ARM_JUNO_SOC_PERIPHERALS_SZ           (SIZE_64KB * 9)

// 6GB of DRAM from the 64bit address space
#define ARM_JUNO_EXTRA_SYSTEM_MEMORY_BASE     0x0880000000
#define ARM_JUNO_EXTRA_SYSTEM_MEMORY_SZ       (SIZE_2GB + SIZE_4GB)

//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_ARM_OEM_ID           'A','R','M','L','T','D'   // OEMID 6 bytes long
#define EFI_ACPI_ARM_OEM_TABLE_ID     SIGNATURE_64('A','R','M','-','J','U','N','O') // OEM table id 8 bytes long
#define EFI_ACPI_ARM_OEM_REVISION     0x20140727
#define EFI_ACPI_ARM_CREATOR_ID       SIGNATURE_32('A','R','M',' ')
#define EFI_ACPI_ARM_CREATOR_REVISION 0x00000099

// A macro to initialise the common header part of EFI ACPI tables as defined by
// EFI_ACPI_DESCRIPTION_HEADER structure.
#define ARM_ACPI_HEADER(Signature, Type, Revision) {              \
    Signature,                      /* UINT32  Signature */       \
    sizeof (Type),                  /* UINT32  Length */          \
    Revision,                       /* UINT8   Revision */        \
    0,                              /* UINT8   Checksum */        \
    { EFI_ACPI_ARM_OEM_ID },        /* UINT8   OemId[6] */        \
    EFI_ACPI_ARM_OEM_TABLE_ID,      /* UINT64  OemTableId */      \
    EFI_ACPI_ARM_OEM_REVISION,      /* UINT32  OemRevision */     \
    EFI_ACPI_ARM_CREATOR_ID,        /* UINT32  CreatorId */       \
    EFI_ACPI_ARM_CREATOR_REVISION   /* UINT32  CreatorRevision */ \
  }

#define JUNO_WATCHDOG_COUNT  2

// Define if the exported ACPI Tables are based on ACPI 5.0 spec or latest
//#define ARM_JUNO_ACPI_5_0

//
// Address of the system registers that contain the MAC address
// assigned to the PCI Gigabyte Ethernet device.
//

#define ARM_JUNO_SYS_PCIGBE_L  (ARM_VE_BOARD_PERIPH_BASE + 0x74)
#define ARM_JUNO_SYS_PCIGBE_H  (ARM_VE_BOARD_PERIPH_BASE + 0x78)

#endif
