//
//  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
//  Copyright (c) 2014, Linaro Limited. All rights reserved.
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

.text
.align 3

GCC_ASM_EXPORT(ArmCallHvc)

ASM_PFX(ArmCallHvc):
  // Push x0 on the stack - The stack must always be quad-word aligned
  str   x0, [sp, #-16]!

  // Load the HVC arguments values into the appropriate registers
  ldp   x6, x7, [x0, #48]
  ldp   x4, x5, [x0, #32]
  ldp   x2, x3, [x0, #16]
  ldp   x0, x1, [x0, #0]

  hvc   #0

  // Pop the ARM_HVC_ARGS structure address from the stack into x9
  ldr   x9, [sp], #16

  // Store the HVC returned values into the ARM_HVC_ARGS structure.
  // A HVC call can return up to 4 values
  stp   x2, x3, [x9, #16]
  stp   x0, x1, [x9, #0]

  mov   x0, x9

  ret
