//
//  Copyright (c) 2012 - 2020, ARM Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroLib.h>

.text
.align 3

ASM_FUNC(ArmCallSvc)
  // Push frame pointer and return address on the stack
  stp   x29, x30, [sp, #-16]!
  mov   x29, sp

  // x0 is the ARM_SVC_ARGS structure address
  // x30 is used as a scratch register upon returning from SVC
  mov   x30, x0

  // Load the SVC arguments values into the appropriate registers
  ldp   x16, x17, [x0, #128]
  ldp   x14, x15, [x0, #112]
  ldp   x12, x13, [x0, #96]
  ldp   x10, x11, [x0, #80]
  ldp   x8, x9, [x0, #64]
  ldp   x6, x7, [x0, #48]
  ldp   x4, x5, [x0, #32]
  ldp   x2, x3, [x0, #16]
  ldp   x0, x1, [x0, #0]

  svc   #0
  // Prevent speculative execution beyond svc instruction
  dsb   nsh
  isb

  // Store the SVC returned values into the ARM_SVC_ARGS structure.
  // A SVC call can return up to 18 values
  stp   x16, x17, [x30, #128]
  stp   x14, x15, [x30, #112]
  stp   x12, x13, [x30, #96]
  stp   x10, x11, [x30, #80]
  stp   x8, x9, [x30, #64]
  stp   x6, x7, [x30, #48]
  stp   x4, x5, [x30, #32]
  stp   x2, x3, [x30, #16]
  stp   x0, x1, [x30, #0]

  ldp   x29, x30, [sp], #16

  ret
