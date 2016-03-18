/** @file
*  Header defining Versatile Express constants (Base addresses, sizes, flags)
*
*  Copyright (c) 2012, ARM Limited. All rights reserved.
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

#ifndef __ARM_VEXPRESS_CTA15A7_H__
#define __ARM_VEXPRESS_CTA15A7_H__

#include <VExpressMotherBoard.h>

/***********************************************************************************
// Platform Memory Map
************************************************************************************/

// Motherboard Peripheral and On-chip peripheral
#define ARM_VE_BOARD_PERIPH_BASE              0x1C010000

#ifdef ARM_BIGLITTLE_TC2

// Secure NOR Flash
#define ARM_VE_SEC_NOR0_BASE                  0x00000000
#define ARM_VE_SEC_NOR0_SZ                    SIZE_64MB

// Secure RAM
#define ARM_VE_SEC_RAM0_BASE                  0x04000000
#define ARM_VE_SEC_RAM0_SZ                    SIZE_64MB

#endif

// NOR Flash 0
#define ARM_VE_SMB_NOR0_BASE                  0x08000000
#define ARM_VE_SMB_NOR0_SZ                    SIZE_64MB
// NOR Flash 1
#define ARM_VE_SMB_NOR1_BASE                  0x0C000000
#define ARM_VE_SMB_NOR1_SZ                    SIZE_64MB

// SRAM
#define ARM_VE_SMB_SRAM_BASE                  0x14000000
#define ARM_VE_SMB_SRAM_SZ                    SIZE_32MB

// USB, Ethernet, VRAM
#ifdef ARM_BIGLITTLE_TC2
#define ARM_VE_SMB_PERIPH_BASE                0x18000000
#define ARM_VE_SMB_PERIPH_SZ                  (SIZE_64MB + SIZE_32MB + SIZE_16MB)
#else
#define ARM_VE_SMB_PERIPH_BASE                0x1C000000
#define ARM_VE_SMB_PERIPH_SZ                  (SIZE_64MB + SIZE_16MB)
#endif
#define PL111_CLCD_VRAM_MOTHERBOARD_BASE      ARM_VE_SMB_PERIPH_BASE

// On-Chip non-secure ROM
#ifdef ARM_BIGLITTLE_TC2
#define ARM_VE_TC2_NON_SECURE_ROM_BASE        0x1F000000
#define ARM_VE_TC2_NON_SECURE_ROM_SZ          SIZE_16MB
#endif

// On-Chip Peripherals
#define ARM_VE_ONCHIP_PERIPH_BASE             0x20000000
#define ARM_VE_ONCHIP_PERIPH_SZ               0x10000000

// On-Chip non-secure SRAM
#ifdef ARM_BIGLITTLE_TC2
#define ARM_VE_TC2_NON_SECURE_SRAM_BASE       0x2E000000
#define ARM_VE_TC2_NON_SECURE_SRAM_SZ         SIZE_64KB
#endif

// Allocate a section for the VRAM (Video RAM)
// If 0 then allow random memory allocation
#define LCD_VRAM_CORE_TILE_BASE               0

// Define SEC phase sync point
#define ARM_SEC_EVENT_BOOT_IMAGE_TABLE_IS_AVAILABLE   (ARM_SEC_EVENT_MAX + 1)

/***********************************************************************************
   Core Tile memory-mapped Peripherals
************************************************************************************/

// PL354 Static Memory Controller Base
#ifdef ARM_BIGLITTLE_TC2
#define ARM_VE_SMC_CTRL_BASE                    0x7FFD0000
#else
#define ARM_VE_SMC_CTRL_BASE                    (ARM_VE_BOARD_PERIPH_BASE + 0xE1000)
#endif

#define ARM_CTA15A7_SCC_BASE                    0x7FFF0000
#define ARM_CTA15A7_SCC_CFGREG48                (ARM_CTA15A7_SCC_BASE + 0x700)

#define ARM_CTA15A7_SCC_SYSINFO                 ARM_CTA15A7_SCC_CFGREG48

#define ARM_CTA15A7_SCC_SYSINFO_CLUSTER_A7_NUM_CPU(val)         (((val) >> 20) & 0xF)
#define ARM_CTA15A7_SCC_SYSINFO_CLUSTER_A15_NUM_CPU(val)        (((val) >> 16) & 0xF)
#define ARM_CTA15A7_SCC_SYSINFO_ACTIVE_CLUSTER_A15              (1 << 0)
#define ARM_CTA15A7_SCC_SYSINFO_ACTIVE_CLUSTER_A7               (1 << 1)
#define ARM_CTA15A7_SCC_SYSINFO_UEFI_RESTORE_DEFAULT_NORFLASH   (1 << 4)

#define ARM_CTA15A7_SPC_BASE                    0x7FFF0B00
#define ARM_CTA15A7_SPC_WAKE_INT_MASK           (ARM_CTA15A7_SPC_BASE + 0x24)
#define ARM_CTA15A7_SPC_STANDBYWFI_STAT         (ARM_CTA15A7_SPC_BASE + 0x3C)
#define ARM_CTA15A7_SPC_A15_BX_ADDR0            (ARM_CTA15A7_SPC_BASE + 0x68)
#define ARM_CTA15A7_SPC_A15_BX_ADDR1            (ARM_CTA15A7_SPC_BASE + 0x6C)
#define ARM_CTA15A7_SPC_A15_BX_ADDR2            (ARM_CTA15A7_SPC_BASE + 0x70)
#define ARM_CTA15A7_SPC_A15_BX_ADDR3            (ARM_CTA15A7_SPC_BASE + 0x74)
#define ARM_CTA15A7_SPC_A7_BX_ADDR0             (ARM_CTA15A7_SPC_BASE + 0x78)
#define ARM_CTA15A7_SPC_A7_BX_ADDR1             (ARM_CTA15A7_SPC_BASE + 0x7C)
#define ARM_CTA15A7_SPC_A7_BX_ADDR2             (ARM_CTA15A7_SPC_BASE + 0x80)
#define ARM_CTA15A7_SPC_A7_BX_ADDR3             (ARM_CTA15A7_SPC_BASE + 0x84)

#define ARM_CTA15A7_SPC_WAKE_INT_MASK_IRQ_A15_MASK_0  (1 << 0)
#define ARM_CTA15A7_SPC_WAKE_INT_MASK_IRQ_A15_MASK_1  (1 << 1)
#define ARM_CTA15A7_SPC_WAKE_INT_MASK_FIQ_A15_MASK_0  (1 << 2)
#define ARM_CTA15A7_SPC_WAKE_INT_MASK_FIQ_A15_MASK_1  (1 << 3)
#define ARM_CTA15A7_SPC_WAKE_INT_MASK_IRQ_A7_MASK_0   (1 << 4)
#define ARM_CTA15A7_SPC_WAKE_INT_MASK_IRQ_A7_MASK_1   (1 << 5)
#define ARM_CTA15A7_SPC_WAKE_INT_MASK_IRQ_A7_MASK_2   (1 << 6)
#define ARM_CTA15A7_SPC_WAKE_INT_MASK_FIQ_A7_MASK_0   (1 << 7)
#define ARM_CTA15A7_SPC_WAKE_INT_MASK_FIQ_A7_MASK_1   (1 << 8)
#define ARM_CTA15A7_SPC_WAKE_INT_MASK_FIQ_A7_MASK_2   (1 << 9)

#define ARM_CTA15A7_SPC_STANDBYWFI_STAT_A15_0   (1 << 0)
#define ARM_CTA15A7_SPC_STANDBYWFI_STAT_A15_1   (1 << 1)
#define ARM_CTA15A7_SPC_STANDBYWFI_STAT_A15_L2  (1 << 2)
#define ARM_CTA15A7_SPC_STANDBYWFI_STAT_A7_0    (1 << 3)
#define ARM_CTA15A7_SPC_STANDBYWFI_STAT_A7_1    (1 << 4)
#define ARM_CTA15A7_SPC_STANDBYWFI_STAT_A7_2    (1 << 5)
#define ARM_CTA15A7_SPC_STANDBYWFI_STAT_A7_L2   (1 << 6)


/***********************************************************************************
// Memory-mapped peripherals
************************************************************************************/

/*// SP810 Controller
#undef SP810_CTRL_BASE
#define SP810_CTRL_BASE                         0x1C020000

// PL111 Colour LCD Controller
#define PL111_CLCD_SITE                         ARM_VE_MOTHERBOARD_SITE
#define PL111_CLCD_MOTHERBOARD_VIDEO_MODE_OSC_ID  1
#define PL111_CLCD_CORE_TILE_VIDEO_MODE_OSC_ID  1

// VRAM offset for the PL111 Colour LCD Controller on the motherboard
#define VRAM_MOTHERBOARD_BASE                     (ARM_VE_SMB_PERIPH_BASE   + 0x00000)*/

#endif
