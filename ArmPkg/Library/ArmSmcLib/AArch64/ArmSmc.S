//
//  Copyright (c) 2012-2023, Arm Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//  @par Reference(s):
//   - SMC Calling Convention (SMCCC), ARM DEN 0028E, EAC0, 1.4
//     (https://developer.arm.com/documentation/den0028/e/)
//

#include <AsmMacroIoLibV8.h>

ASM_FUNC(ArmCallSmc)
  // Push x0 on the stack - The stack must always be quad-word aligned
  str   x0, [sp, #-16]!

  // Load the SMC arguments values into the appropriate registers
  ldp   x10, x11, [x0, #80]
  ldp   x8, x9, [x0, #64]
  ldp   x6, x7, [x0, #48]
  ldp   x4, x5, [x0, #32]
  ldp   x2, x3, [x0, #16]
  ldp   x0, x1, [x0, #0]

  smc   #0

  // Pop the ARM_SMC_ARGS structure address from the stack into x13
  ldr   x13, [sp], #16

  // Store the SMC returned values into the ARM_SMC_ARGS structure.
  stp   x10, x11, [x13, #80]
  stp   x8, x9, [x13, #64]
  stp   x6, x7, [x13, #48]
  stp   x4, x5, [x13, #32]
  stp   x2, x3, [x13, #16]
  stp   x0, x1, [x13, #0]

  mov   x0, x13

  ret
