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
  push  {r4-r7}

  // Preserve R0 for later use
  mov   ip, r0

  // Load the SMCCC arguments values into the appropriate registers
  ldm   r0, {r0-r7}

#if !defined(_PCD_VALUE_PcdMonitorConduitHvc)
#error
#elif _PCD_VALUE_PcdMonitorConduitHvc == 0
  .arch_extension sec
  smc   #0
#elif _PCD_VALUE_PcdMonitorConduitHvc == 1
  .arch_extension virt
  hvc   #0
#else
#error
#endif

  // A SMCCC SMC32/HVC32 call can return up to 8 values.
  stm   ip, {r0-r7}

  // Clear return values from registers
  mov   r0, #0
  mov   r1, #0
  mov   r2, #0
  mov   r3, #0

  pop   {r4-r7}
  bx    lr
