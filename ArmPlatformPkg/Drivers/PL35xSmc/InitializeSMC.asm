//
//  Copyright (c) 2011, ARM Limited. All rights reserved.
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
#include <Library/PcdLib.h>
#include <Drivers/PL35xSmc.h>
#include <AutoGen.h>

  INCLUDE AsmMacroIoLib.inc
  
  EXPORT  SMCInitializeNOR
  EXPORT  SMCInitializeSRAM
  EXPORT  SMCInitializePeripherals
  EXPORT  SMCInitializeVRAM

  PRESERVE8
  AREA    ModuleInitializeSMC, CODE, READONLY
  
// CS0  CS0-Interf0     NOR1 flash on the motherboard
// CS1  CS1-Interf0     Reserved for the motherboard
// CS2  CS2-Interf0     SRAM on the motherboard
// CS3  CS3-Interf0     memory-mapped Ethernet and USB controllers on the motherboard
// CS4  CS0-Interf1     NOR2 flash on the motherboard
// CS5  CS1-Interf1     memory-mapped peripherals
// CS6  CS2-Interf1     memory-mapped peripherals
// CS7  CS3-Interf1     system memory-mapped peripherals on the motherboard.

// IN r1 SmcBase
// IN r2 ChipSelect
// NOTE: This code is been called before any stack has been setup. It means some registers
//       could be overwritten (case of 'r0')
SMCInitializeNOR
  // Write to set_cycle register(holding register for NOR 1 cycle register or NAND cycle register)
  //   - Read cycle timeout  = 0xA (0:3)
  //   - Write cycle timeout = 0x3(7:4)
  //   - OE Assertion Delay  = 0x9(11:8)
  //   - WE Assertion delay  = 0x3(15:12)
  //   - Page cycle timeout  = 0x2(19:16)
  ldr     r0, = 0x0002393A
  str     r0, [r1, #PL354_SMC_SET_CYCLES_OFFSET]
  
  // Write to set_opmode register(holding register for NOR 1 opomode register or NAND opmode register)
  ldr     r0, = (PL354_SMC_SET_OPMODE_MEM_WIDTH_32 :OR: PL354_SMC_SET_OPMODE_SET_RD_BURST_LENGTH_CONT :OR: PL354_SMC_SET_OPMODE_SET_WR_BURST_LENGTH_CONT :OR: PL354_SMC_SET_OPMODE_SET_ADV)
  str     r0, [r1, #PL354_SMC_SET_OPMODE_OFFSET]

  // Write to direct_cmd register so that the NOR 1 registers(set-cycles and opmode) are updated with holding registers
  ldr     r0, =PL354_SMC_DIRECT_CMD_ADDR_CMD_UPDATE
  orr     r0, r0, r2
  str     r0, [r1, #PL354_SMC_DIRECT_CMD_OFFSET]
  
  bx    lr


//
// Setup SRAM (CS2-Interface0)
//
SMCInitializeSRAM
  ldr     r0, = 0x00027158
  str     r0, [r1, #PL354_SMC_SET_CYCLES_OFFSET]

  ldr     r0, =(PL354_SMC_SET_OPMODE_MEM_WIDTH_32 :OR: PL354_SMC_SET_OPMODE_SET_ADV)
  str     r0, [r1, #PL354_SMC_SET_OPMODE_OFFSET]
  
  ldr     r0, =(PL354_SMC_DIRECT_CMD_ADDR_CMD_UPDATE :OR: PL354_SMC_DIRECT_CMD_ADDR_CS(0,2))
  str     r0, [r1, #PL354_SMC_DIRECT_CMD_OFFSET]

  bx    lr

SMCInitializePeripherals
//
// USB/Eth/VRAM (CS3-Interface0)
//
  ldr     r0, = 0x000CD2AA
  str     r0, [r1, #PL354_SMC_SET_CYCLES_OFFSET]
 
  ldr     r0, =(PL354_SMC_SET_OPMODE_MEM_WIDTH_32 :OR: PL354_SMC_SET_OPMODE_SET_RD_SYNC :OR: PL354_SMC_SET_OPMODE_SET_WR_SYNC)
  str     r0, [r1, #PL354_SMC_SET_OPMODE_OFFSET]
        
  ldr     r0, =(PL354_SMC_DIRECT_CMD_ADDR_CMD_UPDATE :OR: PL354_SMC_DIRECT_CMD_ADDR_CS(0,3))
  str     r0, [r1, #PL354_SMC_DIRECT_CMD_OFFSET]


//
// Setup Peripherals (CS3-Interface1)
//
  ldr     r0, = 0x00025156
  str     r0, [r1, #PL354_SMC_SET_CYCLES_OFFSET]
 
  ldr     r0, =(PL354_SMC_SET_OPMODE_MEM_WIDTH_32 :OR: PL354_SMC_SET_OPMODE_SET_RD_SYNC :OR: PL354_SMC_SET_OPMODE_SET_WR_SYNC)
  str     r0, [r1, #PL354_SMC_SET_OPMODE_OFFSET]
        
  ldr     r0, =(PL354_SMC_DIRECT_CMD_ADDR_CMD_UPDATE :OR: PL354_SMC_DIRECT_CMD_ADDR_CS(1,3))
  str     r0, [r1, #PL354_SMC_DIRECT_CMD_OFFSET]

  bx    lr


// IN r1 SmcBase
// IN r2 VideoSRamBase
// NOTE: This code is been called before any stack has been setup. It means some registers
//       could be overwritten (case of 'r0')
SMCInitializeVRAM
  //
  // Setup VRAM (CS1-Interface0)
  //
  ldr     r0, =  0x00049249
  str     r0, [r1, #PL354_SMC_SET_CYCLES_OFFSET]
 
  ldr     r0, = (PL354_SMC_SET_OPMODE_MEM_WIDTH_32 :OR: PL354_SMC_SET_OPMODE_SET_RD_SYNC :OR: PL354_SMC_SET_OPMODE_SET_WR_SYNC)
  str     r0, [r1, #PL354_SMC_SET_OPMODE_OFFSET]
        
  ldr     r0, = (PL354_SMC_DIRECT_CMD_ADDR_CMD_UPDATE :OR: PL354_SMC_DIRECT_CMD_ADDR_CS(0,1))
  str     r0, [r1, #PL354_SMC_DIRECT_CMD_OFFSET]
  
  //
  // Page mode setup for VRAM
  //

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
  
  bx    lr
  
  END
