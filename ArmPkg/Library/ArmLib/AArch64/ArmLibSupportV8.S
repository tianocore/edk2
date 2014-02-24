#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2011 - 2014, ARM Limited. All rights reserved.
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLibV8.h>

.text
.align 3

GCC_ASM_EXPORT (ArmIsMpCore)
GCC_ASM_EXPORT (ArmEnableAsynchronousAbort)
GCC_ASM_EXPORT (ArmDisableAsynchronousAbort)
GCC_ASM_EXPORT (ArmEnableIrq)
GCC_ASM_EXPORT (ArmDisableIrq)
GCC_ASM_EXPORT (ArmEnableFiq)
GCC_ASM_EXPORT (ArmDisableFiq)
GCC_ASM_EXPORT (ArmEnableInterrupts)
GCC_ASM_EXPORT (ArmDisableInterrupts)
GCC_ASM_EXPORT (ArmDisableAllExceptions)
GCC_ASM_EXPORT (ReadCCSIDR)
GCC_ASM_EXPORT (ReadCLIDR)

#------------------------------------------------------------------------------

.set MPIDR_U_BIT,    (30)
.set MPIDR_U_MASK,   (1 << MPIDR_U_BIT)
.set DAIF_FIQ_BIT,   (1 << 0)
.set DAIF_IRQ_BIT,   (1 << 1)
.set DAIF_ABORT_BIT, (1 << 2)
.set DAIF_DEBUG_BIT, (1 << 3)
.set DAIF_INT_BITS,  (DAIF_FIQ_BIT | DAIF_IRQ_BIT)
.set DAIF_ALL,       (DAIF_DEBUG_BIT | DAIF_ABORT_BIT | DAIF_INT_BITS)


ASM_PFX(ArmIsMpCore):
  mrs   x0, mpidr_el1         // Read EL1 Mutliprocessor Affinty Reg (MPIDR)
  and   x0, x0, #MPIDR_U_MASK // U Bit clear, the processor is part of a multiprocessor system
  lsr   x0, x0, #MPIDR_U_BIT
  eor   x0, x0, #1
  ret


ASM_PFX(ArmEnableAsynchronousAbort):
  msr   daifclr, #DAIF_ABORT_BIT
  isb
  ret


ASM_PFX(ArmDisableAsynchronousAbort):
  msr   daifset, #DAIF_ABORT_BIT
  isb
  ret


ASM_PFX(ArmEnableIrq):
  msr   daifclr, #DAIF_IRQ_BIT
  isb
  ret


ASM_PFX(ArmDisableIrq):
  msr   daifset, #DAIF_IRQ_BIT
  isb
  ret


ASM_PFX(ArmEnableFiq):
  msr   daifclr, #DAIF_FIQ_BIT
  isb
  ret


ASM_PFX(ArmDisableFiq):
  msr   daifset, #DAIF_FIQ_BIT
  isb
  ret


ASM_PFX(ArmEnableInterrupts):
  msr   daifclr, #DAIF_INT_BITS
  isb
  ret


ASM_PFX(ArmDisableInterrupts):
  msr   daifset, #DAIF_INT_BITS
  isb
  ret


ASM_PFX(ArmDisableAllExceptions):
  msr   daifset, #DAIF_ALL
  isb
  ret


// UINT32
// ReadCCSIDR (
//   IN UINT32 CSSELR
//   )
ASM_PFX(ReadCCSIDR):
  msr   csselr_el1, x0        // Write Cache Size Selection Register (CSSELR)
  isb
  mrs   x0, ccsidr_el1        // Read current Cache Size ID Register (CCSIDR)
  ret


// UINT32
// ReadCLIDR (
//   IN UINT32 CSSELR
//   )
ASM_PFX(ReadCLIDR):
  mrs   x0, clidr_el1         // Read Cache Level ID Register
  ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
