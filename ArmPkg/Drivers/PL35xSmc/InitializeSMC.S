#
#  Copyright (c) 2011, ARM Limited. All rights reserved.
#  
#  This program and the accompanying materials                          
#  are licensed and made available under the terms and conditions of the BSD License         
#  which accompanies this distribution.  The full text of the license may be found at        
#  http:#opensource.org/licenses/bsd-license.php                                            
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#
#

#include <AsmMacroIoLib.h>
#include <Library/PcdLib.h>
#include <AutoGen.h>
#include <AsmMacroIoLib.h>

#Start of the code section
.text

#Maintain 8 byte alignment
.align 3

#Export Initialize SMC symbol
GCC_ASM_EXPORT(InitializeSMC)

# Static memory configuation definitions for SMC
.set        SmcDirectCmd,              0x10
.set        SmcSetCycles,              0x14
.set        SmcSetOpMode,              0x18

# CS0  CS0-Interf0     NOR1 flash on the motherboard
# CS1  CS1-Interf0     Reserved for the motherboard
# CS2  CS2-Interf0     SRAM on the motherboard
# CS3  CS3-Interf0     memory-mapped Ethernet and USB controllers on the motherboard
# CS4  CS0-Interf1     NOR2 flash on the motherboard
# CS5  CS1-Interf1     memory-mapped peripherals
# CS6  CS2-Interf1     memory-mapped peripherals
# CS7  CS3-Interf1     system memory-mapped peripherals on the motherboard.

# IN r1 SmcBase
# IN r2 VideoSRamBase
# NOTE: This code is been called before any stack has been setup. It means some registers
#       could be overwritten (case of 'r0')


ASM_PFX(InitializeSMC):
#
# Setup NOR1 (CS0-Interface0)
#

  #Write to set_cycle register(holding register for NOR 1 cycle register or NAND cycle register)
     #Read cycle timeout = 0xA (0:3)
     #Write cycle timeout = 0x3(7:4)
     #OE Assertion Delay = 0x9(11:8)
     #WE Assertion delay = 0x3(15:12)
     #Page cycle timeout = 0x2(19:16)  
  LoadConstantToReg (0x0002393A,r0)       @ldr     r0, = 0x0002393A
  str     r0, [r1, #SmcSetCycles]
  
  #Write to set_opmode register(holding register for NOR 1 opomode register or NAND opmode register)
     # 0x00000002 = MemoryWidth: 32bit
     # 0x00000028 = ReadMemoryBurstLength:continuous
     # 0x00000280 = WriteMemoryBurstLength:continuous
     # 0x00000800 = Set Address Valid
  LoadConstantToReg (0x00000AAA,r0)       @ldr     r0, = 0x00000AAA
  str     r0, [r1, #SmcSetOpMode]    

 #Write to direct_cmd register so that the NOR 1 registers(set-cycles and opmode) are updated with holding registers
     # 0x00000000 = ChipSelect0-Interface 0
     # 0x00400000 = CmdTypes: UpdateRegs
  LoadConstantToReg (0x00400000,r0)       @ldr     r0, = 0x00400000
  str     r0, [r1, #SmcDirectCmd]                              
  
  
#
# Setup SRAM (CS2-Interface0)
#
  LoadConstantToReg (0x00027158,r0)       @ldr     r0, = 0x00027158
  str     r0, [r1, #SmcSetCycles]

  # 0x00000002 = MemoryWidth: 32bit
  # 0x00000800 = Set Address Valid
  LoadConstantToReg (0x00000802,r0)       @ldr     r0, = 0x00000802
  str     r0, [r1, #SmcSetOpMode]
  
  # 0x01000000 = ChipSelect2-Interface 0
  # 0x00400000 = CmdTypes: UpdateRegs
  LoadConstantToReg (0x01400000,r0)       @ldr     r0, = 0x01400000
  str     r0, [r1, #SmcDirectCmd]

#
# USB/Eth/VRAM (CS3-Interface0)
#
  LoadConstantToReg (0x000CD2AA,r0)      @ldr     r0, = 0x000CD2AA
  str     r0, [r1, #SmcSetCycles]
 
  # 0x00000002 = MemoryWidth: 32bit
  # 0x00000004 = Memory reads are synchronous
  # 0x00000040 = Memory writes are synchronous
  LoadConstantToReg (0x00000046,r0)      @ldr     r0, = 0x00000046
  str     r0, [r1, #SmcSetOpMode]  
        
  # 0x01800000 = ChipSelect3-Interface 0
  # 0x00400000 = CmdTypes: UpdateRegs
  LoadConstantToReg (0x01C00000,r0)      @ldr     r0, = 0x01C00000
  str     r0, [r1, #SmcDirectCmd]  

#
# Setup NOR3 (CS0-Interface1)
#
  LoadConstantToReg (0x0002393A,r0)      @ldr     r0, = 0x0002393A
  str     r0, [r1, #SmcSetCycles]
 
  # 0x00000002 = MemoryWidth: 32bit
  # 0x00000028 = ReadMemoryBurstLength:continuous
  # 0x00000280 = WriteMemoryBurstLength:continuous
  # 0x00000800 = Set Address Valid
  LoadConstantToReg (0x00000AAA,r0)      @ldr     r0, = 0x00000AAA
  str     r0, [r1, #SmcSetOpMode]  
        
  # 0x02000000 = ChipSelect0-Interface 1
  # 0x00400000 = CmdTypes: UpdateRegs
  LoadConstantToReg (0x02400000,r0)      @ldr     r0, = 0x02400000
  str     r0, [r1, #SmcDirectCmd]  
 
#
# Setup Peripherals (CS3-Interface1)
#
  LoadConstantToReg (0x00025156,r0)      @ldr     r0, = 0x00025156
  str     r0, [r1, #SmcSetCycles]
 
  # 0x00000002 = MemoryWidth: 32bit
  # 0x00000004 = Memory reads are synchronous
  # 0x00000040 = Memory writes are synchronous
  LoadConstantToReg (0x00000046,r0)      @ldr     r0, = 0x00000046
  str     r0, [r1, #SmcSetOpMode]  
        
  # 0x03800000 = ChipSelect3-Interface 1
  # 0x00400000 = CmdTypes: UpdateRegs
  LoadConstantToReg (0x03C00000,r0)      @ldr     r0, = 0x03C00000
  str     r0, [r1, #SmcDirectCmd] 

#
# Setup VRAM (CS1-Interface0)
#
  LoadConstantToReg (0x00049249,r0)      @ldr     r0, =  0x00049249
  str     r0, [r1, #SmcSetCycles]
 
  # 0x00000002 = MemoryWidth: 32bit
  # 0x00000004 = Memory reads are synchronous
  # 0x00000040 = Memory writes are synchronous
  LoadConstantToReg (0x00000046,r0)      @ldr     r0, = 0x00000046
  str     r0, [r1, #SmcSetOpMode]  
        
  # 0x00800000 = ChipSelect1-Interface 0
  # 0x00400000 = CmdTypes: UpdateRegs
  LoadConstantToReg (0x00C00000,r0)      @ldr     r0, = 0x00C00000
  str     r0, [r1, #SmcDirectCmd]  
  
#
# Page mode setup for VRAM
#
  #read current state 
  ldr     r0, [r2, #0]  
  ldr     r0, [r2, #0]  
  LoadConstantToReg (0x00000000,r0)      @ldr     r0, = 0x00000000
  str     r0, [r2, #0] 
  ldr     r0, [r2, #0]  

  #enable page mode 
  ldr     r0, [r2, #0]  
  ldr     r0, [r2, #0]  
  LoadConstantToReg (0x00000000,r0)      @ldr     r0, = 0x00000000
  str     r0, [r2, #0] 
  LoadConstantToReg (0x00900090,r0)      @ldr     r0, = 0x00900090
  str     r0, [r2, #0] 

  #confirm page mode enabled
  ldr     r0, [r2, #0]  
  ldr     r0, [r2, #0]  
  LoadConstantToReg (0x00000000,r0)      @ldr     r0, = 0x00000000
  str     r0, [r2, #0] 
  ldr     r0, [r2, #0]  
  
  bx    lr
  
ASM_FUNCTION_REMOVE_IF_UNREFERENCED