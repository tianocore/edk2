//
//  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroLib.h>

ASM_FUNC(ArmCallSmc)
  // Push x0 on the stack - The stack must always be quad-word aligned
  str   x19, [sp, #-16]!
  str   x0, [sp, #-16]!

  // Load the SMC arguments values into the appropriate registers
  ldp   x16, x17, [x0, #128]
  ldp   x14, x15, [x0, #112]
  ldp   x12, x13, [x0, #96]
  ldp   x10, x11, [x0, #80]
  ldp   x8, x9, [x0, #64]
  ldp   x6, x7, [x0, #48]
  ldp   x4, x5, [x0, #32]
  ldp   x2, x3, [x0, #16]
  ldp   x0, x1, [x0, #0]

  smc   #0

  // Pop the ARM_SMC_ARGS structure address from the stack into x9
  ldr   x19, [sp], #16

  // Store the SMC returned values into the ARM_SMC_ARGS structure.
  // A SMC call can return up to 4 values - we do not need to store back x4-x7.
  stp   x16, x17, [x19, #128]
  stp   x14, x15, [x19, #112]
  stp   x12, x13, [x19, #96]
  stp   x10, x11, [x19, #80]
  stp   x8, x9, [x19, #64]
  stp   x6, x7, [x19, #48]
  stp   x4, x5, [x19, #32]
  stp   x2, x3, [x19, #16]
  stp   x0, x1, [x19, #0]

  mov   x0, x19

  ldr   x19, [sp], #16

  ret
