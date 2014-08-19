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
#include <Library/PcdLib.h>
#include <Drivers/PL35xSmc.h>
#include <AutoGen.h>

  INCLUDE AsmMacroIoLib.inc

  EXPORT  PL35xSmcInitialize
  EXPORT  PL35xSmcSetRefresh

  PRESERVE8
  AREA    ModuleInitializeSMC, CODE, READONLY

// IN r1 Smc Base Address
// IN r2 Smc Configuration Start Address
// IN r3 Smc Configuration End Address
// NOTE: This code is been called before any stack has been setup. It means some registers
//       could be overwritten (case of 'r0')
PL35xSmcInitialize
  // While (SmcConfigurationStart < SmcConfigurationEnd)
  cmp   r2, r3
  blxge lr

  // Write to set_cycle register(holding register for NOR 1 cycle register or NAND cycle register)
  ldr   r0, [r2, #0x4]
  str   r0, [r1, #PL350_SMC_SET_CYCLES_OFFSET]

  // Write to set_opmode register(holding register for NOR 1 opomode register or NAND opmode register)
  ldr   r0, [r2, #0x8]
  str   r0, [r1, #PL350_SMC_SET_OPMODE_OFFSET]

  // Write to direct_cmd register so that the NOR 1 registers(set-cycles and opmode) are updated with holding registers
  ldr   r0, =PL350_SMC_DIRECT_CMD_ADDR_CMD_UPDATE
  ldr   r4, [r2, #0x0]
  orr   r0, r0, r4
  str   r0, [r1, #PL350_SMC_DIRECT_CMD_OFFSET]

  add   r2, #0xC
  b     PL35xSmcInitialize

// IN r1 Smc Base Address
// IN r2 Smc Refresh Period 0
// IN r3 Smc Refresh Period 1
PL35xSmcSetRefresh
  str   r2, [r1, #PL350_SMC_REFRESH_0_OFFSET]
  str   r3, [r1, #PL350_SMC_REFRESH_1_OFFSET]
  blx lr

  END
