#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2011 - 2016, ARM Limited. All rights reserved.
# Copyright (c) 2016, Linaro Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLibV8.h>

.set DAIF_RD_FIQ_BIT,   (1 << 6)
.set DAIF_RD_IRQ_BIT,   (1 << 7)

ASM_FUNC(ArmReadMidr)
  mrs     x0, midr_el1        // Read from Main ID Register (MIDR)
  ret

ASM_FUNC(ArmCacheInfo)
  mrs     x0, ctr_el0         // Read from Cache Type Regiter (CTR)
  ret

ASM_FUNC(ArmGetInterruptState)
  mrs     x0, daif
  tst     w0, #DAIF_RD_IRQ_BIT  // Check if IRQ is enabled. Enabled if 0 (Z=1)
  cset    w0, eq                // if Z=1 return 1, else 0
  ret

ASM_FUNC(ArmGetFiqState)
  mrs     x0, daif
  tst     w0, #DAIF_RD_FIQ_BIT  // Check if FIQ is enabled. Enabled if 0 (Z=1)
  cset    w0, eq                // if Z=1 return 1, else 0
  ret

ASM_FUNC(ArmWriteCpacr)
  msr     cpacr_el1, x0      // Coprocessor Access Control Reg (CPACR)
  ret

ASM_FUNC(ArmWriteAuxCr)
  EL1_OR_EL2(x1)
1:msr     actlr_el1, x0      // Aux Control Reg (ACTLR) at EL1. Also available in EL2 and EL3
  ret
2:msr     actlr_el2, x0      // Aux Control Reg (ACTLR) at EL1. Also available in EL2 and EL3
  ret

ASM_FUNC(ArmReadAuxCr)
  EL1_OR_EL2(x1)
1:mrs     x0, actlr_el1      // Aux Control Reg (ACTLR) at EL1. Also available in EL2 and EL3
  ret
2:mrs     x0, actlr_el2      // Aux Control Reg (ACTLR) at EL1. Also available in EL2 and EL3
  ret

ASM_FUNC(ArmSetTTBR0)
  EL1_OR_EL2_OR_EL3(x1)
1:msr     ttbr0_el1, x0      // Translation Table Base Reg 0 (TTBR0)
  b       4f
2:msr     ttbr0_el2, x0      // Translation Table Base Reg 0 (TTBR0)
  b       4f
3:msr     ttbr0_el3, x0      // Translation Table Base Reg 0 (TTBR0)
4:isb
  ret

ASM_FUNC(ArmGetTTBR0BaseAddress)
  EL1_OR_EL2(x1)
1:mrs     x0, ttbr0_el1
  b       3f
2:mrs     x0, ttbr0_el2
3:and     x0, x0, 0xFFFFFFFFFFFF  /* Look at bottom 48 bits */
  isb
  ret

ASM_FUNC(ArmGetTCR)
  EL1_OR_EL2_OR_EL3(x1)
1:mrs     x0, tcr_el1
  b       4f
2:mrs     x0, tcr_el2
  b       4f
3:mrs     x0, tcr_el3
4:isb
  ret

ASM_FUNC(ArmSetTCR)
  EL1_OR_EL2_OR_EL3(x1)
1:msr     tcr_el1, x0
  b       4f
2:msr     tcr_el2, x0
  b       4f
3:msr     tcr_el3, x0
4:isb
  ret

ASM_FUNC(ArmGetMAIR)
  EL1_OR_EL2_OR_EL3(x1)
1:mrs     x0, mair_el1
  b       4f
2:mrs     x0, mair_el2
  b       4f
3:mrs     x0, mair_el3
4:isb
  ret

ASM_FUNC(ArmSetMAIR)
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
ASM_FUNC(ArmUpdateTranslationTableEntry)
   dsb     nshst
   lsr     x1, x1, #12
   EL1_OR_EL2_OR_EL3(x0)
1: tlbi    vaae1, x1             // TLB Invalidate VA , EL1
   b       4f
2: tlbi    vae2, x1              // TLB Invalidate VA , EL2
   b       4f
3: tlbi    vae3, x1              // TLB Invalidate VA , EL3
4: dsb     nsh
   isb
   ret

ASM_FUNC(ArmInvalidateTlb)
   EL1_OR_EL2_OR_EL3(x0)
1: tlbi  vmalle1
   b     4f
2: tlbi  alle2
   b     4f
3: tlbi  alle3
4: dsb   sy
   isb
   ret

ASM_FUNC(ArmWriteCptr)
  msr     cptr_el3, x0           // EL3 Coprocessor Trap Reg (CPTR)
  ret

ASM_FUNC(ArmWriteScr)
  msr     scr_el3, x0            // Secure configuration register EL3
  isb
  ret

ASM_FUNC(ArmWriteMVBar)
  msr    vbar_el3, x0            // Exception Vector Base address for Monitor on EL3
  ret

ASM_FUNC(ArmCallWFE)
  wfe
  ret

ASM_FUNC(ArmCallSEV)
  sev
  ret

ASM_FUNC(ArmReadCpuActlr)
  mrs   x0, S3_1_c15_c2_0
  ret

ASM_FUNC(ArmWriteCpuActlr)
  msr   S3_1_c15_c2_0, x0
  dsb   sy
  isb
  ret

ASM_FUNC(ArmReadSctlr)
  EL1_OR_EL2_OR_EL3(x1)
1:mrs   x0, sctlr_el1
  ret
2:mrs   x0, sctlr_el2
  ret
3:mrs   x0, sctlr_el3
4:ret

ASM_FUNC(ArmWriteSctlr)
  EL1_OR_EL2_OR_EL3(x1)
1:msr   sctlr_el1, x0
  ret
2:msr   sctlr_el2, x0
  ret
3:msr   sctlr_el3, x0
4:ret

ASM_FUNC(ArmGetPhysicalAddressBits)
  mrs   x0, id_aa64mmfr0_el1
  adr   x1, .LPARanges
  and   x0, x0, #0xf
  ldrb  w0, [x1, x0]
  ret

//
// Bits 0..3 of the AA64MFR0_EL1 system register encode the size of the
// physical address space support on this CPU:
// 0 == 32 bits, 1 == 36 bits, etc etc
// 7 and up are reserved
//
.LPARanges:
  .byte 32, 36, 40, 42, 44, 48, 52,  0
  .byte  0,  0,  0,  0,  0,  0,  0,  0

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
