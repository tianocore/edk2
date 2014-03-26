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
GCC_ASM_EXPORT (ArmReadMidr)
GCC_ASM_EXPORT (ArmCacheInfo)
GCC_ASM_EXPORT (ArmGetInterruptState)
GCC_ASM_EXPORT (ArmGetFiqState)
GCC_ASM_EXPORT (ArmGetTTBR0BaseAddress)
GCC_ASM_EXPORT (ArmSetTTBR0)
GCC_ASM_EXPORT (ArmGetTCR)
GCC_ASM_EXPORT (ArmSetTCR)
GCC_ASM_EXPORT (ArmGetMAIR)
GCC_ASM_EXPORT (ArmSetMAIR)
GCC_ASM_EXPORT (ArmWriteCpacr)
GCC_ASM_EXPORT (ArmWriteAuxCr)
GCC_ASM_EXPORT (ArmReadAuxCr)
GCC_ASM_EXPORT (ArmInvalidateTlb)
GCC_ASM_EXPORT (ArmUpdateTranslationTableEntry)
GCC_ASM_EXPORT (ArmWriteCptr)
GCC_ASM_EXPORT (ArmWriteScr)
GCC_ASM_EXPORT (ArmWriteMVBar)
GCC_ASM_EXPORT (ArmCallWFE)
GCC_ASM_EXPORT (ArmCallSEV)
GCC_ASM_EXPORT (ArmReadCpuActlr)
GCC_ASM_EXPORT (ArmWriteCpuActlr)

#------------------------------------------------------------------------------

.set DAIF_FIQ_BIT,   (1 << 0)
.set DAIF_IRQ_BIT,   (1 << 1)

ASM_PFX(ArmReadMidr):
  mrs     x0, midr_el1        // Read from Main ID Register (MIDR)
  ret

ASM_PFX(ArmCacheInfo):
  mrs     x0, ctr_el0         // Read from Cache Type Regiter (CTR)
  ret

ASM_PFX(ArmGetInterruptState):
  mrs     x0, daif
  tst     w0, #DAIF_IRQ_BIT   // Check if IRQ is enabled. Enabled if 0.
  mov     w0, #0
  mov     w1, #1
  csel    w0, w1, w0, ne
  ret

ASM_PFX(ArmGetFiqState):
  mrs     x0, daif
  tst     w0, #DAIF_FIQ_BIT   // Check if FIQ is enabled. Enabled if 0.
  mov     w0, #0
  mov     w1, #1
  csel    w0, w1, w0, ne
  ret

ASM_PFX(ArmWriteCpacr):
  msr     cpacr_el1, x0      // Coprocessor Access Control Reg (CPACR)
  ret

ASM_PFX(ArmWriteAuxCr):
  EL1_OR_EL2(x1)
1:msr     actlr_el1, x0      // Aux Control Reg (ACTLR) at EL1. Also available in EL2 and EL3
  ret
2:msr     actlr_el2, x0      // Aux Control Reg (ACTLR) at EL1. Also available in EL2 and EL3
  ret

ASM_PFX(ArmReadAuxCr):
  EL1_OR_EL2(x1)
1:mrs     x0, actlr_el1      // Aux Control Reg (ACTLR) at EL1. Also available in EL2 and EL3
  ret
2:mrs     x0, actlr_el2      // Aux Control Reg (ACTLR) at EL1. Also available in EL2 and EL3
  ret

ASM_PFX(ArmSetTTBR0):
  EL1_OR_EL2_OR_EL3(x1)
1:msr     ttbr0_el1, x0      // Translation Table Base Reg 0 (TTBR0)
  b       4f
2:msr     ttbr0_el2, x0      // Translation Table Base Reg 0 (TTBR0)
  b       4f
3:msr     ttbr0_el3, x0      // Translation Table Base Reg 0 (TTBR0)
4:isb
  ret

ASM_PFX(ArmGetTTBR0BaseAddress):
  EL1_OR_EL2(x1)
1:mrs     x0, ttbr0_el1
  b       3f
2:mrs     x0, ttbr0_el2
3:LoadConstantToReg(0xFFFFFFFFFFFF, x1) /* Look at bottom 48 bits */
  and     x0, x0, x1
  isb
  ret

ASM_PFX(ArmGetTCR):
  EL1_OR_EL2_OR_EL3(x1)
1:mrs     x0, tcr_el1
  b       4f
2:mrs     x0, tcr_el2
  b       4f
3:mrs     x0, tcr_el3
4:isb
  ret

ASM_PFX(ArmSetTCR):
  EL1_OR_EL2_OR_EL3(x1)
1:msr     tcr_el1, x0
  b       4f
2:msr     tcr_el2, x0
  b       4f
3:msr     tcr_el3, x0
4:isb
  ret

ASM_PFX(ArmGetMAIR):
  EL1_OR_EL2_OR_EL3(x1)
1:mrs     x0, mair_el1
  b       4f
2:mrs     x0, mair_el2
  b       4f
3:mrs     x0, mair_el3
4:isb
  ret

ASM_PFX(ArmSetMAIR):
  EL1_OR_EL2_OR_EL3(x1)
1:msr     mair_el1, x0
  b       4f
2:msr     mair_el2, x0
  b       4f
3:msr     mair_el3, x0
4:isb
  ret


//
//VOID
//ArmUpdateTranslationTableEntry (
//  IN VOID  *TranslationTableEntry  // X0
//  IN VOID  *MVA                    // X1
//  );
ASM_PFX(ArmUpdateTranslationTableEntry):
   dc      civac, x0             // Clean and invalidate data line
   dsb     sy
   EL1_OR_EL2_OR_EL3(x0)
1: tlbi    vaae1, x1             // TLB Invalidate VA , EL1
   b       4f
2: tlbi    vae2, x1              // TLB Invalidate VA , EL2
   b       4f
3: tlbi    vae3, x1              // TLB Invalidate VA , EL3
4: dsb     sy
   isb
   ret

ASM_PFX(ArmInvalidateTlb):
   EL1_OR_EL2_OR_EL3(x0)
1: tlbi  vmalle1
   b     4f
2: tlbi  alle2
   b     4f
3: tlbi  alle3
4: dsb   sy
   isb
   ret

ASM_PFX(ArmWriteCptr):
  msr     cptr_el3, x0           // EL3 Coprocessor Trap Reg (CPTR)
  ret

ASM_PFX(ArmWriteScr):
  msr     scr_el3, x0            // Secure configuration register EL3
  ret

ASM_PFX(ArmWriteMVBar):
  msr    vbar_el3, x0            // Exception Vector Base address for Monitor on EL3
  ret

ASM_PFX(ArmCallWFE):
  wfe
  ret

ASM_PFX(ArmCallSEV):
  sev
  ret

ASM_PFX(ArmReadCpuActlr):
  mrs   x0, S3_1_c15_c2_0
  ret

ASM_PFX(ArmWriteCpuActlr):
  msr   S3_1_c15_c2_0, x0
  dsb   sy
  isb
  ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
