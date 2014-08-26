/** @file
*  Header defining RealView EB constants (Base addresses, sizes, flags)
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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

#ifndef __ARM_EB_H__
#define __ARM_EB_H__

/*******************************************
// Platform Memory Map
*******************************************/

// Can be NOR, DOC, DRAM, SRAM
#define ARM_EB_REMAP_BASE                     0x00000000
#define ARM_EB_REMAP_SZ                       0x04000000

// Motherboard Peripheral and On-chip peripheral
#define ARM_EB_SMB_MB_ON_CHIP_PERIPH_BASE     0x10000000
#define ARM_EB_SMB_MB_ON_CHIP_PERIPH_SZ       0x10000000
#define ARM_EB_BOARD_PERIPH_BASE              0x10000000
//#define ARM_EB_CHIP_PERIPH_BASE             0x10020000

// SMC
#define ARM_EB_SMC_BASE                       0x40000000
#define ARM_EB_SMC_SZ                         0x20000000

// NOR Flash 1
#define ARM_EB_SMB_NOR_BASE                   0x40000000
#define ARM_EB_SMB_NOR_SZ                     0x04000000 /* 64 MB */
// DOC Flash
#define ARM_EB_SMB_DOC_BASE                   0x44000000
#define ARM_EB_SMB_DOC_SZ                     0x04000000 /* 64 MB */
// SRAM
#define ARM_EB_SMB_SRAM_BASE                  0x48000000
#define ARM_EB_SMB_SRAM_SZ                    0x02000000 /* 32 MB */
// USB, Ethernet, VRAM
#define ARM_EB_SMB_PERIPH_BASE                0x4E000000
//#define ARM_EB_SMB_PERIPH_VRAM              0x4C000000
#define ARM_EB_SMB_PERIPH_SZ                  0x02000000 /* 32 MB */

// Logic Tile
#define ARM_EB_LOGIC_TILE_BASE                0xC0000000
#define ARM_EB_LOGIC_TILE_SZ                  0x40000000

/*******************************************
// Motherboard peripherals
*******************************************/

// Define MotherBoard SYS flags offsets (from ARM_EB_BOARD_PERIPH_BASE)
#define ARM_EB_SYS_ID_REG                     (ARM_EB_BOARD_PERIPH_BASE + 0x00000)
#define ARM_EB_SYS_OSC4_REG                   (ARM_EB_BOARD_PERIPH_BASE + 0x0001C)
#define ARM_EB_SYS_LOCK_REG                   (ARM_EB_BOARD_PERIPH_BASE + 0x00020)
#define ARM_EB_SYS_100HZ_REG                  (ARM_EB_BOARD_PERIPH_BASE + 0x00024)
#define ARM_EB_SYS_FLAGS_REG                  (ARM_EB_BOARD_PERIPH_BASE + 0x00030)
#define ARM_EB_SYS_FLAGS_SET_REG              (ARM_EB_BOARD_PERIPH_BASE + 0x00030)
#define ARM_EB_SYS_FLAGS_CLR_REG              (ARM_EB_BOARD_PERIPH_BASE + 0x00034)
#define ARM_EB_SYS_FLAGS_NV_REG               (ARM_EB_BOARD_PERIPH_BASE + 0x00038)
#define ARM_EB_SYS_FLAGS_NV_SET_REG           (ARM_EB_BOARD_PERIPH_BASE + 0x00038)
#define ARM_EB_SYS_FLAGS_NV_CLR_REG           (ARM_EB_BOARD_PERIPH_BASE + 0x0003C)
#define ARM_EB_SYS_RESETCTL_REG               (ARM_EB_BOARD_PERIPH_BASE + 0x00040)
#define ARM_EB_SYS_CLCD_REG                   (ARM_EB_BOARD_PERIPH_BASE + 0x00050)
#define ARM_EB_SYS_PROCID0_REG                (ARM_EB_BOARD_PERIPH_BASE + 0x00084)
#define ARM_EB_SYS_PROCID1_REG                (ARM_EB_BOARD_PERIPH_BASE + 0x00088)

// SP810 Controller
#define SP810_CTRL_BASE                       (ARM_EB_BOARD_PERIPH_BASE + 0x01000)

// SYSTRCL Register
#define ARM_EB_SYSCTRL                        0x10001000

// Dynamic Memory Controller Base
#define ARM_EB_DMC_BASE                       0x10018000

// Static Memory Controller Base
#define ARM_EB_SMC_CTRL_BASE                  0x10080000

//Note: Moving the framebuffer into the 0x70000000-0x80000000 region does not seem to work
#define PL111_CLCD_VRAM_BASE                  0x00100000

/*// System Configuration Controller register Base addresses
//#define ARM_EB_SYS_CFG_CTRL_BASE                0x100E2000
#define ARM_EB_SYS_CFGRW0_REG                   0x100E2000
#define ARM_EB_SYS_CFGRW1_REG                   0x100E2004
#define ARM_EB_SYS_CFGRW2_REG                   0x100E2008

#define ARM_EB_CFGRW1_REMAP_NOR0                0
#define ARM_EB_CFGRW1_REMAP_NOR1                (1 << 28)
#define ARM_EB_CFGRW1_REMAP_EXT_AXI             (1 << 29)
#define ARM_EB_CFGRW1_REMAP_DRAM                (1 << 30)

// PL301 Fast AXI Base Address
#define ARM_EB_FAXI_BASE                        0x100E9000

// L2x0 Cache Controller Base Address
//#define ARM_EB_L2x0_CTLR_BASE                   0x1E00A000*/

#define ARM_EB_SYS_PROC_ID_MASK                  (UINT32)(0xFFU << 24)
#define ARM_EB_SYS_PROC_ID_CORTEX_A8             (UINT32)(0x0EU << 24)
#define ARM_EB_SYS_PROC_ID_CORTEX_A9             (UINT32)(0x0CU << 24)

/*******************************************
// System Configuration Control
*******************************************/

// Sites where the peripheral is fitted
#define ARM_EB_UNSUPPORTED                        ~0

#define VIRTUAL_SYS_CFG(site,func)                     (((site) << 24) | (func))

#define SYS_CFG_RTC                               VIRTUAL_SYS_CFG(ARM_EB_UNSUPPORTED,1)

#endif
