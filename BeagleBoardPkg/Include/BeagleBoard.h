/** @file
*  Header defining the BeagleBoard constants (Base addresses, sizes, flags)
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

#ifndef __BEAGLEBOARD_PLATFORM_H__
#define __BEAGLEBOARD_PLATFORM_H__

// DDR attributes
#define DDR_ATTRIBUTES_CACHED                ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED              ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

// SoC registers. L3 interconnects
#define SOC_REGISTERS_L3_PHYSICAL_BASE       0x68000000
#define SOC_REGISTERS_L3_PHYSICAL_LENGTH     0x08000000
#define SOC_REGISTERS_L3_ATTRIBUTES          ARM_MEMORY_REGION_ATTRIBUTE_DEVICE

// SoC registers. L4 interconnects
#define SOC_REGISTERS_L4_PHYSICAL_BASE       0x48000000
#define SOC_REGISTERS_L4_PHYSICAL_LENGTH     0x08000000
#define SOC_REGISTERS_L4_ATTRIBUTES          ARM_MEMORY_REGION_ATTRIBUTE_DEVICE


#if 0
/*******************************************
// Platform Memory Map
*******************************************/

// Can be NOR, DOC, DRAM, SRAM
#define ARM_EB_REMAP_BASE                     0x00000000
#define ARM_EB_REMAP_SZ                       0x04000000

// Motherboard Peripheral and On-chip peripheral
#define ARM_EB_SMB_MB_ON_CHIP_PERIPH_BASE     0x10000000
#define ARM_EB_SMB_MB_ON_CHIP_PERIPH_SZ       0x00100000
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

// DRAM
#define ARM_EB_DRAM_BASE                      0x70000000
#define ARM_EB_DRAM_SZ                        0x10000000

// Logic Tile
#define ARM_EB_LOGIC_TILE_BASE                0xC0000000
#define ARM_EB_LOGIC_TILE_SZ                  0x40000000

/*******************************************
// Motherboard peripherals
*******************************************/

// Define MotherBoard SYS flags offsets (from ARM_EB_BOARD_PERIPH_BASE)
#define ARM_EB_SYS_FLAGS_REG                  (ARM_EB_BOARD_PERIPH_BASE + 0x00030)
#define ARM_EB_SYS_FLAGS_SET_REG              (ARM_EB_BOARD_PERIPH_BASE + 0x00030)
#define ARM_EB_SYS_FLAGS_CLR_REG              (ARM_EB_BOARD_PERIPH_BASE + 0x00034)
#define ARM_EB_SYS_FLAGS_NV_REG               (ARM_EB_BOARD_PERIPH_BASE + 0x00038)
#define ARM_EB_SYS_FLAGS_NV_SET_REG           (ARM_EB_BOARD_PERIPH_BASE + 0x00038)
#define ARM_EB_SYS_FLAGS_NV_CLR_REG           (ARM_EB_BOARD_PERIPH_BASE + 0x0003C)
#define ARM_EB_SYS_CLCD                       (ARM_EB_BOARD_PERIPH_BASE + 0x00050)
#define ARM_EB_SYS_PROCID0_REG                (ARM_EB_BOARD_PERIPH_BASE + 0x00084)
#define ARM_EB_SYS_PROCID1_REG                (ARM_EB_BOARD_PERIPH_BASE + 0x00088)
#define ARM_EB_SYS_CFGDATA_REG                (ARM_EB_BOARD_PERIPH_BASE + 0x000A0)
#define ARM_EB_SYS_CFGCTRL_REG                (ARM_EB_BOARD_PERIPH_BASE + 0x000A4)
#define ARM_EB_SYS_CFGSTAT_REG                (ARM_EB_BOARD_PERIPH_BASE + 0x000A8)

// SP810 Controller
#define SP810_CTRL_BASE                       (ARM_EB_BOARD_PERIPH_BASE + 0x01000)

// SYSTRCL Register
#define ARM_EB_SYSCTRL                        0x10001000

// Uart0
#define PL011_CONSOLE_UART_BASE               (ARM_EB_BOARD_PERIPH_BASE + 0x09000)
#define PL011_CONSOLE_UART_SPEED              115200

// SP804 Timer Bases
#define SP804_TIMER0_BASE                     (ARM_EB_BOARD_PERIPH_BASE + 0x11000)
#define SP804_TIMER1_BASE                     (ARM_EB_BOARD_PERIPH_BASE + 0x11020)
#define SP804_TIMER2_BASE                     (ARM_EB_BOARD_PERIPH_BASE + 0x12000)
#define SP804_TIMER3_BASE                     (ARM_EB_BOARD_PERIPH_BASE + 0x12020)

// PL301 RTC
#define PL031_RTC_BASE                        (ARM_EB_BOARD_PERIPH_BASE + 0x17000)

// Dynamic Memory Controller Base
#define ARM_EB_DMC_BASE                       0x10018000

// Static Memory Controller Base
#define ARM_EB_SMC_CTRL_BASE                  0x10080000

#define PL111_CLCD_BASE                       0x10020000
//TODO: FIXME ... Reserved the memory in UEFI !!! Otherwise risk of corruption
#define PL111_CLCD_VRAM_BASE                  0x78000000

#define ARM_EB_SYS_OSCCLK4                    0x1000001C


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


// PL031 RTC - Other settings
#define PL031_PPM_ACCURACY                      300000000

/*******************************************
// Interrupt Map
*******************************************/

// Timer Interrupts
#define TIMER01_INTERRUPT_NUM                34
#define TIMER23_INTERRUPT_NUM                35


/*******************************************
// EFI Memory Map in Permanent Memory (DRAM)
*******************************************/

// This region is allocated at the bottom of the DRAM. It will be used
// for fixed address allocations such as Vector Table
#define ARM_EB_EFI_FIX_ADDRESS_REGION_SZ        SIZE_8MB

// This region is the memory declared to PEI as permanent memory for PEI
// and DXE. EFI stacks and heaps will be declared in this region.
#define ARM_EB_EFI_MEMORY_REGION_SZ             0x1000000
#endif

typedef enum {
  REVISION_XM,
  REVISION_UNKNOWN0,
  REVISION_UNKNOWN1,
  REVISION_UNKNOWN2,
  REVISION_UNKNOWN3,
  REVISION_C4,
  REVISION_C123,
  REVISION_AB,
} BEAGLEBOARD_REVISION;

#endif
