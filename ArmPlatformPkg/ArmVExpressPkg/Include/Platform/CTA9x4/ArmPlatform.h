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

#ifndef __ARM_VEXPRESS_H__
#define __ARM_VEXPRESS_H__

/*******************************************
// Platform Memory Map
*******************************************/

// Can be NOR0, NOR1, DRAM
#define ARM_VE_REMAP_BASE                       0x00000000
#define ARM_VE_REMAP_SZ                         0x04000000

// Motherboard Peripheral and On-chip peripheral
#define ARM_VE_SMB_MB_ON_CHIP_PERIPH_BASE       0x10000000
#define ARM_VE_SMB_MB_ON_CHIP_PERIPH_SZ         0x10000000 /* 256 MB */
#define ARM_VE_BOARD_PERIPH_BASE                0x10000000
#define ARM_VE_CHIP_PERIPH_BASE                 0x10020000

// SMC
#define ARM_VE_SMC_BASE                         0x40000000
#define ARM_VE_SMC_SZ                           0x1C000000

// NOR Flash 1
#define ARM_VE_SMB_NOR0_BASE                    0x40000000
#define ARM_VE_SMB_NOR0_SZ                      0x04000000 /* 64 MB */
// NOR Flash 2
#define ARM_VE_SMB_NOR1_BASE                    0x44000000
#define ARM_VE_SMB_NOR1_SZ                      0x04000000 /* 64 MB */
// SRAM
#define ARM_VE_SMB_SRAM_BASE                    0x48000000
#define ARM_VE_SMB_SRAM_SZ                      0x02000000 /* 32 MB */
// USB, Ethernet, VRAM
#define ARM_VE_SMB_PERIPH_BASE                  0x4C000000
#define ARM_VE_SMB_PERIPH_VRAM                  0x4C000000
#define ARM_VE_SMB_PERIPH_SZ                    0x04000000 /* 32 MB */

// DRAM
#define ARM_VE_DRAM_BASE                        0x60000000
#define ARM_VE_DRAM_SZ                          0x40000000

// External AXI between daughterboards (Logic Tile)
#define ARM_VE_EXT_AXI_BASE                     0xE0000000
#define ARM_VE_EXT_AXI_SZ                       0x20000000

/*******************************************
// Motherboard peripherals
*******************************************/

// Define MotherBoard SYS flags offsets (from ARM_VE_BOARD_PERIPH_BASE)
#define ARM_VE_SYS_FLAGS_REG                    (ARM_VE_BOARD_PERIPH_BASE + 0x00030)
#define ARM_VE_SYS_FLAGS_SET_REG                (ARM_VE_BOARD_PERIPH_BASE + 0x00030)
#define ARM_VE_SYS_FLAGS_CLR_REG                (ARM_VE_BOARD_PERIPH_BASE + 0x00034)
#define ARM_VE_SYS_FLAGS_NV_REG                 (ARM_VE_BOARD_PERIPH_BASE + 0x00038)
#define ARM_VE_SYS_FLAGS_NV_SET_REG             (ARM_VE_BOARD_PERIPH_BASE + 0x00038)
#define ARM_VE_SYS_FLAGS_NV_CLR_REG             (ARM_VE_BOARD_PERIPH_BASE + 0x0003C)
#define ARM_VE_SYS_PROCID0_REG                  (ARM_VE_BOARD_PERIPH_BASE + 0x00084)
#define ARM_VE_SYS_PROCID1_REG                  (ARM_VE_BOARD_PERIPH_BASE + 0x00088)
#define ARM_VE_SYS_CFGDATA_REG                  (ARM_VE_BOARD_PERIPH_BASE + 0x000A0)
#define ARM_VE_SYS_CFGCTRL_REG                  (ARM_VE_BOARD_PERIPH_BASE + 0x000A4)
#define ARM_VE_SYS_CFGSTAT_REG                  (ARM_VE_BOARD_PERIPH_BASE + 0x000A8)

// SP810 Controller
#define SP810_CTRL_BASE                      (ARM_VE_BOARD_PERIPH_BASE + 0x01000)

// Uart0
#define PL011_CONSOLE_UART_BASE                (ARM_VE_BOARD_PERIPH_BASE + 0x09000)
#define PL011_CONSOLE_UART_SPEED               38400

// SP804 Timer Bases
#define SP804_TIMER0_BASE                    (ARM_VE_BOARD_PERIPH_BASE + 0x11000)
#define SP804_TIMER1_BASE                    (ARM_VE_BOARD_PERIPH_BASE + 0x11020)
#define SP804_TIMER2_BASE                    (ARM_VE_BOARD_PERIPH_BASE + 0x12000)
#define SP804_TIMER3_BASE                    (ARM_VE_BOARD_PERIPH_BASE + 0x12020)

// Dynamic Memory Controller Base
#define ARM_VE_DMC_BASE                         0x100E0000

// Static Memory Controller Base
#define ARM_VE_SMC_CTRL_BASE                    0x100E1000

// System Configuration Controller register Base addresses
//#define ARM_VE_SYS_CFG_CTRL_BASE                0x100E2000
#define ARM_VE_SYS_CFGRW0_REG                   0x100E2000
#define ARM_VE_SYS_CFGRW1_REG                   0x100E2004
#define ARM_VE_SYS_CFGRW2_REG                   0x100E2008

#define ARM_VE_CFGRW1_TZASC_EN_BIT_MASK         0x2000
#define ARM_VE_CFGRW1_REMAP_NOR0                0
#define ARM_VE_CFGRW1_REMAP_NOR1                (1 << 28)
#define ARM_VE_CFGRW1_REMAP_EXT_AXI             (1 << 29)
#define ARM_VE_CFGRW1_REMAP_DRAM                (1 << 30)

// TZPC Base Address
#define ARM_VE_TZPC_BASE                        0x100E6000

// PL301 Fast AXI Base Address
#define ARM_VE_FAXI_BASE                        0x100E9000

// TZASC Defintions
#define ARM_VE_TZASC_BASE                       0x100EC000
#define ARM_VE_DECPROT_BIT_TZPC                 (1 << 6)
#define ARM_VE_DECPROT_BIT_DMC_TZASC            (1 << 11)
#define ARM_VE_DECPROT_BIT_NMC_TZASC            (1 << 12)
#define ARM_VE_DECPROT_BIT_SMC_TZASC            (1 << 13)
#define ARM_VE_DECPROT_BIT_EXT_MAST_TZ          (1)
#define ARM_VE_DECPROT_BIT_DMC_TZASC_LOCK       (1 << 3)
#define ARM_VE_DECPROT_BIT_NMC_TZASC_LOCK       (1 << 4)
#define ARM_VE_DECPROT_BIT_SMC_TZASC_LOCK       (1 << 5)

// L2x0 Cache Controller Base Address
//#define ARM_VE_L2x0_CTLR_BASE                   0x1E00A000


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
#define ARM_VE_EFI_FIX_ADDRESS_REGION_SZ        SIZE_8MB

// This region is the memory declared to PEI as permanent memory for PEI
// and DXE. EFI stacks and heaps will be declared in this region.
#define ARM_VE_EFI_MEMORY_REGION_SZ             0x1000000


#endif 
