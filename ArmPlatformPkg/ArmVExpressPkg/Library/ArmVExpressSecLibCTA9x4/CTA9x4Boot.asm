//
//  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <Library/ArmPlatformLib.h>
#include <Drivers/PL35xSmc.h>
#include <ArmPlatform.h>
#include <AutoGen.h>

  INCLUDE AsmMacroIoLib.inc

  EXPORT  ArmPlatformSecBootAction
  EXPORT  ArmPlatformSecBootMemoryInit
  IMPORT  PL35xSmcInitialize

  PRESERVE8
  AREA    CTA9x4BootMode, CODE, READONLY

//
// For each Chip Select: ChipSelect / SetCycle / SetOpMode
//
VersatileExpressSmcConfiguration
    // NOR Flash 0
    DCD     PL350_SMC_DIRECT_CMD_ADDR_CS(0)
    DCD     PL350_SMC_SET_CYCLE_NAND_T_RC(0xA) :OR: PL350_SMC_SET_CYCLE_NAND_T_WC(0x3) :OR: PL350_SMC_SET_CYCLE_NAND_T_REA(0x1) :OR: PL350_SMC_SET_CYCLE_NAND_T_WP(0x7) :OR: PL350_SMC_SET_CYCLE_NAND_T_AR(0x1)
    DCD     PL350_SMC_SET_OPMODE_MEM_WIDTH_32 :OR: PL350_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_CONT :OR: PL350_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_CONT :OR: PL350_SMC_SET_OPMODE_SET_ADV

    // NOR Flash 1
    DCD     PL350_SMC_DIRECT_CMD_ADDR_CS(4)
    DCD     PL350_SMC_SET_CYCLE_NAND_T_RC(0xA) :OR: PL350_SMC_SET_CYCLE_NAND_T_WC(0x3) :OR: PL350_SMC_SET_CYCLE_NAND_T_REA(0x1) :OR: PL350_SMC_SET_CYCLE_NAND_T_WP(0x7) :OR: PL350_SMC_SET_CYCLE_NAND_T_AR(0x1)
    DCD     PL350_SMC_SET_OPMODE_MEM_WIDTH_32 :OR: PL350_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_CONT :OR: PL350_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_CONT :OR: PL350_SMC_SET_OPMODE_SET_ADV

    // SRAM
    DCD     PL350_SMC_DIRECT_CMD_ADDR_CS(2)
    DCD     PL350_SMC_SET_CYCLE_SRAM_T_RC(0x8) :OR: PL350_SMC_SET_CYCLE_SRAM_T_WC(0x5) :OR: PL350_SMC_SET_CYCLE_SRAM_T_CEOE(0x1) :OR: PL350_SMC_SET_CYCLE_SRAM_T_WP(0x6) :OR: PL350_SMC_SET_CYCLE_SRAM_T_PC(0x1) :OR: PL350_SMC_SET_CYCLE_SRAM_T_TR(0x1)
    DCD     PL350_SMC_SET_OPMODE_MEM_WIDTH_32 :OR: PL350_SMC_SET_OPMODE_SET_ADV

    // Usb/Eth/VRAM
    DCD     PL350_SMC_DIRECT_CMD_ADDR_CS(3)
    DCD     PL350_SMC_SET_CYCLE_SRAM_T_RC(0xA) :OR: PL350_SMC_SET_CYCLE_SRAM_T_WC(0xA) :OR: PL350_SMC_SET_CYCLE_SRAM_T_CEOE(0x2) :OR: PL350_SMC_SET_CYCLE_SRAM_T_WP(0x2) :OR: PL350_SMC_SET_CYCLE_SRAM_T_PC(0x3) :OR: PL350_SMC_SET_CYCLE_SRAM_T_TR(0x6)
    DCD     PL350_SMC_SET_OPMODE_MEM_WIDTH_32 :OR: PL350_SMC_SET_OPMODE_SET_RD_SYNC :OR: PL350_SMC_SET_OPMODE_SET_WR_SYNC

    // Memory Mapped Peripherals
    DCD     PL350_SMC_DIRECT_CMD_ADDR_CS(7)
    DCD     PL350_SMC_SET_CYCLE_SRAM_T_RC(0x6) :OR: PL350_SMC_SET_CYCLE_SRAM_T_WC(0x5) :OR: PL350_SMC_SET_CYCLE_SRAM_T_CEOE(0x1) :OR: PL350_SMC_SET_CYCLE_SRAM_T_WP(0x2) :OR: PL350_SMC_SET_CYCLE_SRAM_T_PC(0x1) :OR: PL350_SMC_SET_CYCLE_SRAM_T_TR(0x1)
    DCD     PL350_SMC_SET_OPMODE_MEM_WIDTH_32 :OR: PL350_SMC_SET_OPMODE_SET_RD_SYNC :OR: PL350_SMC_SET_OPMODE_SET_WR_SYNC

    // VRAM
    DCD     PL350_SMC_DIRECT_CMD_ADDR_CS(1)
    DCD     0x00049249
    DCD     PL350_SMC_SET_OPMODE_MEM_WIDTH_32 :OR: PL350_SMC_SET_OPMODE_SET_RD_SYNC :OR: PL350_SMC_SET_OPMODE_SET_WR_SYNC
VersatileExpressSmcConfigurationEnd

/**
  Call at the beginning of the platform boot up

  This function allows the firmware platform to do extra actions at the early
  stage of the platform power up.

  Note: This function must be implemented in assembler as there is no stack set up yet

**/
ArmPlatformSecBootAction
  bx  lr

/**
  Initialize the memory where the initial stacks will reside

  This memory can contain the initial stacks (Secure and Secure Monitor stacks).
  In some platform, this region is already initialized and the implementation of this function can
  do nothing. This memory can also represent the Secure RAM.
  This function is called before the satck has been set up. Its implementation must ensure the stack
  pointer is not used (probably required to use assembly language)

**/
ArmPlatformSecBootMemoryInit
  mov   r5, lr

  //
  // Initialize PL354 SMC
  //
  LoadConstantToReg (ARM_VE_SMC_CTRL_BASE, r1)
  ldr   r2, =VersatileExpressSmcConfiguration
  ldr   r3, =VersatileExpressSmcConfigurationEnd
  blx   PL35xSmcInitialize

  //
  // Page mode setup for VRAM
  //
  LoadConstantToReg (VRAM_MOTHERBOARD_BASE, r2)

  // Read current state
  ldr     r0, [r2, #0]
  ldr     r0, [r2, #0]
  ldr     r0, = 0x00000000
  str     r0, [r2, #0]
  ldr     r0, [r2, #0]

  // Enable page mode
  ldr     r0, [r2, #0]
  ldr     r0, [r2, #0]
  ldr     r0, = 0x00000000
  str     r0, [r2, #0]
  ldr     r0, = 0x00900090
  str     r0, [r2, #0]

  // Confirm page mode enabled
  ldr     r0, [r2, #0]
  ldr     r0, [r2, #0]
  ldr     r0, = 0x00000000
  str     r0, [r2, #0]
  ldr     r0, [r2, #0]

  bx    r5

  END
