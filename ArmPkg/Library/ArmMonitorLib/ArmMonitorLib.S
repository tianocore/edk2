//
//  Copyright (c) 2024, Google Llc. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroLib.h>

/** Monitor call.

  An HyperVisor Call (HVC) or System Monitor Call (SMC) will be issued
  depending on the default conduit. PcdMonitorConduitHvc determines the type
  of the call: if true, do an HVC.

  @param [in,out]  Args    Arguments for the HVC/SMC.
**/
ASM_FUNC(ArmMonitorCall)
  // Create a stack frame
  stp   x29, x30, [sp, #-16]!
  mov   x29, sp

  // Preserve X0 for later use
  mov   x30, x0

  // Load the SMCCC arguments values into the appropriate registers
  ldp   x0, x1, [x30, #0]
  ldp   x2, x3, [x30, #16]
  ldp   x4, x5, [x30, #32]
  ldp   x6, x7, [x30, #48]
  ldp   x8, x9, [x30, #64]
  ldp   x10, x11, [x30, #80]
  ldp   x12, x13, [x30, #96]
  ldp   x14, x15, [x30, #112]
  ldp   x16, x17, [x30, #128]

#if !defined(_PCD_VALUE_PcdMonitorConduitHvc)
#error
#elif _PCD_VALUE_PcdMonitorConduitHvc == 0
  smc   #0
#elif _PCD_VALUE_PcdMonitorConduitHvc == 1
  hvc   #0
#else
#error
#endif

  // A SMCCC SMC64/HVC64 call can return up to 18 values.
  stp   x0, x1, [x30, #0]
  stp   x2, x3, [x30, #16]
  stp   x4, x5, [x30, #32]
  stp   x6, x7, [x30, #48]
  stp   x8, x9, [x30, #64]
  stp   x10, x11, [x30, #80]
  stp   x12, x13, [x30, #96]
  stp   x14, x15, [x30, #112]
  stp   x16, x17, [x30, #128]

  // Clear return values from registers
  mov   x0, xzr
  mov   x1, xzr
  mov   x2, xzr
  mov   x3, xzr
  mov   x4, xzr
  mov   x5, xzr
  mov   x6, xzr
  mov   x7, xzr
  mov   x8, xzr
  mov   x9, xzr
  mov   x10, xzr
  mov   x11, xzr
  mov   x12, xzr
  mov   x13, xzr
  mov   x14, xzr
  mov   x15, xzr
  mov   x16, xzr
  mov   x17, xzr

  ldp   x29, x30, [sp], #16
  ret
