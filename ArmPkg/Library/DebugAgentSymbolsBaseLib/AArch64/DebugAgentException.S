#------------------------------------------------------------------------------
#
# Copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>
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

#include <Chipset/AArch64.h>

GCC_ASM_IMPORT(DefaultExceptionHandler)

.text
VECTOR_BASE(DebugAgentVectorTable)

//
// Current EL with SP0 : 0x0 - 0x180
//
VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_CUR_SP0_SYNC)
ASM_PFX(SynchronousExceptionSP0):
  b   ASM_PFX(SynchronousExceptionSP0)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_CUR_SP0_IRQ)
ASM_PFX(IrqSP0):
  b   ASM_PFX(IrqSP0)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_CUR_SP0_FIQ)
ASM_PFX(FiqSP0):
  b   ASM_PFX(FiqSP0)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_CUR_SP0_SERR)
ASM_PFX(SErrorSP0):
  b   ASM_PFX(SErrorSP0)

//
// Current EL with SPx: 0x200 - 0x380
//
VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_CUR_SPx_SYNC)
ASM_PFX(SynchronousExceptionSPx):
  b   ASM_PFX(SynchronousExceptionSPx)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_CUR_SPx_IRQ)
ASM_PFX(IrqSPx):
  b   ASM_PFX(IrqSPx)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_CUR_SPx_FIQ)
ASM_PFX(FiqSPx):
  b   ASM_PFX(FiqSPx)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_CUR_SPx_SERR)
ASM_PFX(SErrorSPx):
  b   ASM_PFX(SErrorSPx)

/* Lower EL using AArch64 : 0x400 - 0x580 */
VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_LOW_A64_SYNC)
ASM_PFX(SynchronousExceptionA64):
  b   ASM_PFX(SynchronousExceptionA64)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_LOW_A64_IRQ)
ASM_PFX(IrqA64):
  b   ASM_PFX(IrqA64)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_LOW_A64_FIQ)
ASM_PFX(FiqA64):
  b   ASM_PFX(FiqA64)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_LOW_A64_SERR)
ASM_PFX(SErrorA64):
  b   ASM_PFX(SErrorA64)

//
// Lower EL using AArch32 : 0x600 - 0x780
//
VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_LOW_A32_SYNC)
ASM_PFX(SynchronousExceptionA32):
  b   ASM_PFX(SynchronousExceptionA32)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_LOW_A32_IRQ)
ASM_PFX(IrqA32):
  b   ASM_PFX(IrqA32)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_LOW_A32_FIQ)
ASM_PFX(FiqA32):
  b   ASM_PFX(FiqA32)

VECTOR_ENTRY(DebugAgentVectorTable, ARM_VECTOR_LOW_A32_SERR)
ASM_PFX(SErrorA32):
  b   ASM_PFX(SErrorA32)

VECTOR_END(DebugAgentVectorTable)
