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

GCC_ASM_EXPORT(DebugAgentVectorTable)
GCC_ASM_IMPORT(DefaultExceptionHandler)

.text
ASM_PFX(DebugAgentVectorTable):

//
// Current EL with SP0 : 0x0 - 0x180
//
.align 11
ASM_PFX(SynchronousExceptionSP0):
  b   ASM_PFX(SynchronousExceptionSP0)

.align 7
ASM_PFX(IrqSP0):
  b   ASM_PFX(IrqSP0)

.align 7
ASM_PFX(FiqSP0):
  b   ASM_PFX(FiqSP0)

.align 7
ASM_PFX(SErrorSP0):
  b   ASM_PFX(SErrorSP0)

//
// Current EL with SPx: 0x200 - 0x380
//
.align 7
ASM_PFX(SynchronousExceptionSPx):
  b   ASM_PFX(SynchronousExceptionSPx)

.align 7
ASM_PFX(IrqSPx):
  b   ASM_PFX(IrqSPx)

.align 7
ASM_PFX(FiqSPx):
  b   ASM_PFX(FiqSPx)

.align 7
ASM_PFX(SErrorSPx):
  b   ASM_PFX(SErrorSPx)

/* Lower EL using AArch64 : 0x400 - 0x580 */
.align 7
ASM_PFX(SynchronousExceptionA64):
  b   ASM_PFX(SynchronousExceptionA64)

.align 7
ASM_PFX(IrqA64):
  b   ASM_PFX(IrqA64)

.align 7
ASM_PFX(FiqA64):
  b   ASM_PFX(FiqA64)

.align 7
ASM_PFX(SErrorA64):
  b   ASM_PFX(SErrorA64)

//
// Lower EL using AArch32 : 0x0 - 0x180
//
.align 7
ASM_PFX(SynchronousExceptionA32):
  b   ASM_PFX(SynchronousExceptionA32)

.align 7
ASM_PFX(IrqA32):
  b   ASM_PFX(IrqA32)

.align 7
ASM_PFX(FiqA32):
  b   ASM_PFX(FiqA32)

.align 7
ASM_PFX(SErrorA32):
  b   ASM_PFX(SErrorA32)
