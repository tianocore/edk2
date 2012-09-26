/** @file
*  Header defining Versatile Express constants (Base addresses, sizes, flags)
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

#endif /* VEXPRESSMOTHERBOARD_H_ */
