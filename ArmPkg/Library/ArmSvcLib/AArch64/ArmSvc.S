//
//  Copyright (c) 2012 - 2017, ARM Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

.text
.align 3

GCC_ASM_EXPORT(ArmCallSvc)

ASM_PFX(ArmCallSvc):
  // Push frame pointer and return address on the stack
  stp   x29, x30, [sp, #-32]!
  mov   x29, sp

  // Push x0 on the stack - The stack must always be quad-word aligned
  str   x0, [sp, #16]

  // Load the SVC arguments values into the appropriate registers
  ldp   x6, x7, [x0, #48]
  ldp   x4, x5, [x0, #32]
  ldp   x2, x3, [x0, #16]
  ldp   x0, x1, [x0, #0]

  svc   #0

  // Pop the ARM_SVC_ARGS structure address from the stack into x9
  ldr   x9, [sp, #16]

  // Store the SVC returned values into the ARM_SVC_ARGS structure.
  // A SVC call can return up to 4 values - we do not need to store back x4-x7.
  stp   x0, x1, [x9, #0]
  stp   x2, x3, [x9, #16]

  mov   x0, x9

  ldp   x29, x30, [sp], #32
  ret
