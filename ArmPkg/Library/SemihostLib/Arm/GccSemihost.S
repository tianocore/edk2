#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLib.h>

/*
  Semihosting operation request mechanism

  SVC  0x123456  in ARM state (for all architectures)
  SVC  0xAB in Thumb state (excluding ARMv7-M)
  BKPT 0xAB for ARMv7-M (Thumb-2 only)

  R0 - operation type
  R1 - block containing all other parametes

  lr - must be saved as svc instruction will cause an svc exception and write
       the svc lr register. That happens to be the one we are using, so we must
       save it or we will not be able to return.
 */
ASM_FUNC(GccSemihostCall)
  stmfd   sp!, {lr}
  svc     #0x123456
  ldmfd   sp!, {lr}
  bx      lr


