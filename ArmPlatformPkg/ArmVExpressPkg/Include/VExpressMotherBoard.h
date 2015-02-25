/** @file
*  Header defining Versatile Express constants (Base addresses, sizes, flags)
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
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

#ifndef __VEXPRESSMOTHERBOARD_H_
#define __VEXPRESSMOTHERBOARD_H_

#include <ArmPlatform.h>

/***********************************************************************************
// Motherboard memory-mapped peripherals
************************************************************************************/

// Define MotherBoard SYS flags offsets (from ARM_VE_BOARD_PERIPH_BASE)
#define ARM_VE_SYS_ID_REG                         (ARM_VE_BOARD_PERIPH_BASE + 0x00000)
#define ARM_VE_SYS_SW_REG                         (ARM_VE_BOARD_PERIPH_BASE + 0x00004)
#define ARM_VE_SYS_LED_REG                        (ARM_VE_BOARD_PERIPH_BASE + 0x00008)
#define ARM_VE_SYS_FLAGS_REG                      (ARM_VE_BOARD_PERIPH_BASE + 0x00030)
#define ARM_VE_SYS_FLAGS_SET_REG                  (ARM_VE_BOARD_PERIPH_BASE + 0x00030)
#define ARM_VE_SYS_FLAGS_CLR_REG                  (ARM_VE_BOARD_PERIPH_BASE + 0x00034)
#define ARM_VE_SYS_FLAGS_NV_REG                   (ARM_VE_BOARD_PERIPH_BASE + 0x00038)
#define ARM_VE_SYS_FLAGS_NV_SET_REG               (ARM_VE_BOARD_PERIPH_BASE + 0x00038)
#define ARM_VE_SYS_FLAGS_NV_CLR_REG               (ARM_VE_BOARD_PERIPH_BASE + 0x0003C)
#define ARM_VE_SYS_FLASH                          (ARM_VE_BOARD_PERIPH_BASE + 0x0004C)
#define ARM_VE_SYS_CFGSWR_REG                     (ARM_VE_BOARD_PERIPH_BASE + 0x00058)
#define ARM_VE_SYS_MISC                           (ARM_VE_BOARD_PERIPH_BASE + 0x00060)
#define ARM_VE_SYS_PROCID0_REG                    (ARM_VE_BOARD_PERIPH_BASE + 0x00084)
#define ARM_VE_SYS_PROCID1_REG                    (ARM_VE_BOARD_PERIPH_BASE + 0x00088)
#define ARM_VE_SYS_CFGDATA_REG                    (ARM_VE_BOARD_PERIPH_BASE + 0x000A0)
#define ARM_VE_SYS_CFGCTRL_REG                    (ARM_VE_BOARD_PERIPH_BASE + 0x000A4)
#define ARM_VE_SYS_CFGSTAT_REG                    (ARM_VE_BOARD_PERIPH_BASE + 0x000A8)

// SP810 Controller
#ifndef SP810_CTRL_BASE
#define SP810_CTRL_BASE                           (ARM_VE_BOARD_PERIPH_BASE + 0x01000)
#endif

// PL111 Colour LCD Controller - motherboard
#define PL111_CLCD_MOTHERBOARD_BASE               (ARM_VE_BOARD_PERIPH_BASE + 0x1F000)
#define PL111_CLCD_MOTHERBOARD_VIDEO_MODE_OSC_ID  1

// VRAM offset for the PL111 Colour LCD Controller on the motherboard
#define VRAM_MOTHERBOARD_BASE                     (ARM_VE_SMB_PERIPH_BASE   + 0x00000)

#define ARM_VE_SYS_PROC_ID_HBI                    0xFFF
#define ARM_VE_SYS_PROC_ID_MASK                   (UINT32)(0xFFU << 24)
#define ARM_VE_SYS_PROC_ID_UNSUPPORTED            (UINT32)(0xFFU << 24)
#define ARM_VE_SYS_PROC_ID_CORTEX_A9              (UINT32)(0x0CU << 24)
#define ARM_VE_SYS_PROC_ID_CORTEX_A5              (UINT32)(0x12U << 24)
#define ARM_VE_SYS_PROC_ID_CORTEX_A15             (UINT32)(0x14U << 24)
#define ARM_VE_SYS_PROC_ID_CORTEX_A7              (UINT32)(0x18U << 24)
#define ARM_VE_SYS_PROC_ID_CORTEX_A12             (UINT32)(0x1CU << 24)

// Boot Master Select:
// 0 = Site 1 boot master
// 1 = Site 2 boot master
#define ARM_VE_SYS_MISC_MASTERSITE                (1 << 14)
//
// Sites where the peripheral is fitted
//
#define ARM_VE_UNSUPPORTED                        ~0
#define ARM_VE_MOTHERBOARD_SITE                   0
#define ARM_VE_DAUGHTERBOARD_1_SITE               1
#define ARM_VE_DAUGHTERBOARD_2_SITE               2

#define VIRTUAL_SYS_CFG(site,func)                (((site) << 24) | (func))

//
// System Configuration Control Functions
//
#define SYS_CFG_OSC                               1
#define SYS_CFG_VOLT                              2
#define SYS_CFG_AMP                               3
#define SYS_CFG_TEMP                              4
#define SYS_CFG_RESET                             5
#define SYS_CFG_SCC                               6
#define SYS_CFG_MUXFPGA                           7
#define SYS_CFG_SHUTDOWN                          8
#define SYS_CFG_REBOOT                            9
#define SYS_CFG_DVIMODE                           11
#define SYS_CFG_POWER                             12
// Oscillator for Site 1
#define SYS_CFG_OSC_SITE1                         VIRTUAL_SYS_CFG(ARM_VE_DAUGHTERBOARD_1_SITE,SYS_CFG_OSC)
// Oscillator for Site 2
#define SYS_CFG_OSC_SITE2                         VIRTUAL_SYS_CFG(ARM_VE_DAUGHTERBOARD_2_SITE,SYS_CFG_OSC)
// Can not access the battery backed-up hardware clock on the Versatile Express motherboard
#define SYS_CFG_RTC                               VIRTUAL_SYS_CFG(ARM_VE_UNSUPPORTED,1)

//
// System ID
//
// All RTSM VE models have the same System ID : 0x225F500
//
// FVP models have a different System ID.
// Default Base model System ID : 0x00201100
// [31:28] Rev     - Board revision:          0x0     = Rev A
// [27:16] HBI     - HBI board number in BCD: 0x020   = v8 Base Platform
// [15:12] Variant - Build variant of board:  0x1     = Variant B. (GIC 64k map)
// [11:8]  Plat    - Platform type:           0x1     = Model
// [7:0]   FPGA    - FPGA build, BCD coded:   0x00
//
//HBI = 010 = Foundation Model
//HBI = 020 = Base Platform
//
// And specifically, the GIC register banks start at the following
// addresses:
//              Variant = 0             Variant = 1
//GICD          0x2c001000              0x2f000000
//GICC          0x2c002000              0x2c000000
//GICH          0x2c004000              0x2c010000
//GICV          0x2c006000              0x2c020000

#define ARM_FVP_BASE_BOARD_SYS_ID       (0x00200100)
#define ARM_FVP_FOUNDATION_BOARD_SYS_ID (0x00100100)

#define ARM_FVP_SYS_ID_REV_MASK        (UINT32)(0xFUL   << 28)
#define ARM_FVP_SYS_ID_HBI_MASK        (UINT32)(0xFFFUL << 16)
#define ARM_FVP_SYS_ID_VARIANT_MASK    (UINT32)(0xFUL   << 12)
#define ARM_FVP_SYS_ID_PLAT_MASK       (UINT32)(0xFUL   << 8 )
#define ARM_FVP_SYS_ID_FPGA_MASK       (UINT32)(0xFFUL  << 0 )
#define ARM_FVP_GIC_VE_MMAP            0x0
#define ARM_FVP_GIC_BASE_MMAP          (UINT32)(1 << 12)

// The default SYS_IDs. These can be changed when starting the model.
#define ARM_RTSM_SYS_ID                (0x225F500)
#define ARM_FVP_BASE_SYS_ID            (ARM_FVP_BASE_BOARD_SYS_ID | ARM_FVP_GIC_BASE_MMAP)
#define ARM_FVP_FOUNDATION_SYS_ID      (ARM_FVP_FOUNDATION_BOARD_SYS_ID | ARM_FVP_GIC_BASE_MMAP)

#endif /* VEXPRESSMOTHERBOARD_H_ */
