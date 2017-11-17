#
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
#  Copyright (c) 2016-2017, Linaro Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

#include <AsmMacroIoLibV8.h>

//EFI_PHYSICAL_ADDRESS
//GetPhysAddrTop (
//  VOID
//  );
ASM_FUNC(ArmGetPhysAddrTop)
  mrs   x0, id_aa64mmfr0_el1
  adr   x1, .LPARanges
  and   x0, x0, #7
  ldrb  w1, [x1, x0]
  mov   x0, #1
  lsl   x0, x0, x1
  ret

//
// Bits 0..2 of the AA64MFR0_EL1 system register encode the size of the
// physical address space support on this CPU:
// 0 == 32 bits, 1 == 36 bits, etc etc
// 6 and 7 are reserved
//
.LPARanges:
  .byte 32, 36, 40, 42, 44, 48, -1, -1

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
