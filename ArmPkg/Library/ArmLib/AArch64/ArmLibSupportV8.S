#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2011 - 2014, ARM Limited. All rights reserved.
# Copyright (c) 2016, Linaro Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLibV8.h>

.set MPIDR_U_BIT,    (30)
.set MPIDR_U_MASK,   (1 << MPIDR_U_BIT)

// DAIF bit definitions for writing through msr daifclr/sr daifset
.set DAIF_WR_FIQ_BIT,   (1 << 0)
.set DAIF_WR_IRQ_BIT,   (1 << 1)
.set DAIF_WR_ABORT_BIT, (1 << 2)
.set DAIF_WR_DEBUG_BIT, (1 << 3)
.set DAIF_WR_INT_BITS,  (DAIF_WR_FIQ_BIT | DAIF_WR_IRQ_BIT)
.set DAIF_WR_ALL,       (DAIF_WR_DEBUG_BIT | DAIF_WR_ABORT_BIT | DAIF_WR_INT_BITS)


ASM_FUNC(ArmIsMpCore)
  mrs   x0, mpidr_el1         // Read EL1 Multiprocessor Affinty Reg (MPIDR)
  and   x0, x0, #MPIDR_U_MASK // U Bit clear, the processor is part of a multiprocessor system
  lsr   x0, x0, #MPIDR_U_BIT
  eor   x0, x0, #1
  ret


ASM_FUNC(ArmEnableAsynchronousAbort)
  msr   daifclr, #DAIF_WR_ABORT_BIT
  isb
  ret


ASM_FUNC(ArmDisableAsynchronousAbort)
  msr   daifset, #DAIF_WR_ABORT_BIT
  isb
  ret


ASM_FUNC(ArmEnableIrq)
  msr   daifclr, #DAIF_WR_IRQ_BIT
  isb
  ret


ASM_FUNC(ArmDisableIrq)
  msr   daifset, #DAIF_WR_IRQ_BIT
  isb
  ret


ASM_FUNC(ArmEnableFiq)
  msr   daifclr, #DAIF_WR_FIQ_BIT
  isb
  ret


ASM_FUNC(ArmDisableFiq)
  msr   daifset, #DAIF_WR_FIQ_BIT
  isb
  ret


ASM_FUNC(ArmEnableInterrupts)
  msr   daifclr, #DAIF_WR_INT_BITS
  isb
  ret


ASM_FUNC(ArmDisableInterrupts)
  msr   daifset, #DAIF_WR_INT_BITS
  isb
  ret


ASM_FUNC(ArmDisableAllExceptions)
  msr   daifset, #DAIF_WR_ALL
  isb
  ret


// UINTN
// ReadCCSIDR (
//   IN UINT32 CSSELR
//   )
ASM_FUNC(ReadCCSIDR)
  msr   csselr_el1, x0        // Write Cache Size Selection Register (CSSELR)
  isb
  mrs   x0, ccsidr_el1        // Read current Cache Size ID Register (CCSIDR)
  ret


// UINT32
// ReadCLIDR (
//   IN UINT32 CSSELR
//   )
ASM_FUNC(ReadCLIDR)
  mrs   x0, clidr_el1         // Read Cache Level ID Register
  ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
