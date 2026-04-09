#------------------------------------------------------------------------------
#
# Copyright (c) 2011 - 2013, ARM Limited. All rights reserved.
# Copyright (c) 2016, Linaro Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroLib.h>

/* Special accessors for VHE */
#define cntp_tval_el02          s3_5_c14_c2_0
#define cntp_ctl_el02           s3_5_c14_c2_1
#define cntp_cval_el02          s3_5_c14_c2_2
#define cntv_tval_el02          s3_5_c14_c3_0
#define cntv_ctl_el02           s3_5_c14_c3_1
#define cntv_cval_el02          s3_5_c14_c3_2

#define RCNT(reg, sysreg)       rvhe reg, sysreg, sysreg ## 2
#define wCNT(sysreg, reg, tmp)  wvhe sysreg, reg, sysreg ## 2, tmp

        /* Read from 'altname' when running at EL2+VHE, or 'sysreg' otherwise */
        .macro  rvhe, reg, sysreg, altname
        mrs     \reg, CurrentEL
        tbz     \reg, #3, .L\@
        mrs     \reg, hcr_el2
        tbz     \reg, #34, .L\@   // TCR.E2H
        mrs     \reg, \altname
        ret
.L\@:   mrs     \reg, \sysreg
        ret
        .endm

        /* Write to 'altname' when running at EL2+VHE, or 'sysreg' otherwise */
        .macro  wvhe, sysreg, reg, altname, tmp
        mrs     \tmp, CurrentEL
        tbz     \tmp, #3, .L\@
        mrs     \tmp, hcr_el2
        tbz     \tmp, #34, .L\@   // TCR.E2H
        msr     \altname, \reg
        ret
.L\@:   msr     \sysreg, \reg
        ret
        .endm

ASM_FUNC(ArmReadCntFrq)
  mrs   x0, cntfrq_el0           // Read CNTFRQ
  ret


# NOTE - Can only write while at highest implemented EL level (EL3 on model). Else ReadOnly (EL2, EL1, EL0)
ASM_FUNC(ArmWriteCntFrq)
  msr   cntfrq_el0, x0           // Write to CNTFRQ
  ret


ASM_FUNC(ArmReadCntPct)
  mrs   x0, cntpct_el0           // Read CNTPCT (Physical counter register)
  ret


ASM_FUNC(ArmReadCntkCtl)
  mrs   x0, cntkctl_el1          // Read CNTK_CTL (Timer PL1 Control Register)
  ret


ASM_FUNC(ArmWriteCntkCtl)
  msr   cntkctl_el1, x0          // Write to CNTK_CTL (Timer PL1 Control Register)
  ret


ASM_FUNC(ArmReadCntpTval)
  RCNT  (x0, cntp_tval_el0)      // Read CNTP_TVAL (PL1 physical timer value register)


ASM_FUNC(ArmWriteCntpTval)
  wCNT  (cntp_tval_el0, x0, x1)  // Write to CNTP_TVAL (PL1 physical timer value register)


ASM_FUNC(ArmReadCntpCtl)
  RCNT  (x0, cntp_ctl_el0)       // Read CNTP_CTL (PL1 Physical Timer Control Register)


ASM_FUNC(ArmWriteCntpCtl)
  wCNT  (cntp_ctl_el0, x0, x1)   // Write to  CNTP_CTL (PL1 Physical Timer Control Register)


ASM_FUNC(ArmReadCntvTval)
  RCNT  (x0, cntv_tval_el0)      // Read CNTV_TVAL (Virtual Timer Value register)


ASM_FUNC(ArmWriteCntvTval)
  wCNT  (cntv_tval_el0, x0, x1) // Write to CNTV_TVAL (Virtual Timer Value register)


ASM_FUNC(ArmReadCntvCtl)
  RCNT  (x0, cntv_ctl_el0)       // Read CNTV_CTL (Virtual Timer Control Register)


ASM_FUNC(ArmWriteCntvCtl)
  wCNT  (cntv_ctl_el0, x0, x1)   // Write to CNTV_CTL (Virtual Timer Control Register)


ASM_FUNC(ArmReadCntvCt)
  mrs  x0, cntvct_el0            // Read CNTVCT  (Virtual Count Register)
  ret

ASM_FUNC(ArmReadCntpCval)
  RCNT  (x0, cntp_cval_el0)      // Read CNTP_CTVAL (Physical Timer Compare Value Register)


ASM_FUNC(ArmWriteCntpCval)
  wCNT  (cntp_cval_el0, x0, x1)  // Write to CNTP_CTVAL (Physical Timer Compare Value Register)


ASM_FUNC(ArmReadCntvCval)
  RCNT  (x0, cntv_cval_el0)      // Read CNTV_CTVAL (Virtual Timer Compare Value Register)


ASM_FUNC(ArmWriteCntvCval)
  wCNT  (cntv_cval_el0, x0, x1)  // write to  CNTV_CTVAL (Virtual Timer Compare Value Register)


ASM_FUNC(ArmReadCntvOff)
  mrs   x0, cntvoff_el2          // Read CNTVOFF (virtual Offset register)
  ret


ASM_FUNC(ArmWriteCntvOff)
  msr   cntvoff_el2, x0          // Write to CNTVOFF (Virtual Offset register)
  ret


ASM_FUNCTION_REMOVE_IF_UNREFERENCED
