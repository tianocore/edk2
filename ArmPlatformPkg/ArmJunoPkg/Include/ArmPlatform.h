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

// On-Chip non-secure SRAM
#define ARM_JUNO_NON_SECURE_SRAM_BASE         0x2E000000
#define ARM_JUNO_NON_SECURE_SRAM_SZ           SIZE_16MB

// SOC peripherals (HDLCD, UART, I2C, I2S, USB, SMC-PL354, etc)
#define ARM_JUNO_SOC_PERIPHERALS_BASE         0x7FF50000
#define ARM_JUNO_SOC_PERIPHERALS_SZ           (SIZE_64KB * 9)

// 6GB of DRAM from the 64bit address space
#define ARM_JUNO_EXTRA_SYSTEM_MEMORY_BASE     0x0880000000
#define ARM_JUNO_EXTRA_SYSTEM_MEMORY_SZ       (SIZE_2GB + SIZE_4GB)

#endif
