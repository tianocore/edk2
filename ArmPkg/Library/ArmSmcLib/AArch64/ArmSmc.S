//
//  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroLib.h>

ASM_FUNC(ArmCallSmc)
  // Push x0 on the stack - The stack must always be quad-word aligned
  str   x0, [sp, #-16]!

  // Load the SMC arguments values into the appropriate registers
  ldp   x6, x7, [x0, #48]
  ldp   x4, x5, [x0, #32]
  ldp   x2, x3, [x0, #16]
  ldp   x0, x1, [x0, #0]

  smc   #0

  // Pop the ARM_SMC_ARGS structure address from the stack into x9
  ldr   x9, [sp], #16

  // Store the SMC returned values into the ARM_SMC_ARGS structure.
  // A SMC call can return up to 4 values - we do not need to store back x4-x7.
  stp   x2, x3, [x9, #16]
  stp   x0, x1, [x9, #0]

  mov   x0, x9

  ret
