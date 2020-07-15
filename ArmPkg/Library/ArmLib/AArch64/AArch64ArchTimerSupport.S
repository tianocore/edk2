#------------------------------------------------------------------------------
#
# Copyright (c) 2011 - 2013, ARM Limited. All rights reserved.
# Copyright (c) 2016, Linaro Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLibV8.h>

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
  mrs   x0, cntp_tval_el0        // Read CNTP_TVAL (PL1 physical timer value register)
  ret


ASM_FUNC(ArmWriteCntpTval)
  msr   cntp_tval_el0, x0        // Write to CNTP_TVAL (PL1 physical timer value register)
  ret


ASM_FUNC(ArmReadCntpCtl)
  mrs   x0, cntp_ctl_el0         // Read CNTP_CTL (PL1 Physical Timer Control Register)
  ret


ASM_FUNC(ArmWriteCntpCtl)
  msr   cntp_ctl_el0, x0         // Write to  CNTP_CTL (PL1 Physical Timer Control Register)
  ret


ASM_FUNC(ArmReadCntvTval)
  mrs   x0, cntv_tval_el0        // Read CNTV_TVAL (Virtual Timer Value register)
  ret


ASM_FUNC(ArmWriteCntvTval)
  msr   cntv_tval_el0, x0        // Write to CNTV_TVAL (Virtual Timer Value register)
  ret


ASM_FUNC(ArmReadCntvCtl)
  mrs   x0, cntv_ctl_el0         // Read CNTV_CTL (Virtual Timer Control Register)
  ret


ASM_FUNC(ArmWriteCntvCtl)
  msr   cntv_ctl_el0, x0         // Write to CNTV_CTL (Virtual Timer Control Register)
  ret


ASM_FUNC(ArmReadCntvCt)
  mrs  x0, cntvct_el0            // Read CNTVCT  (Virtual Count Register)
  ret


ASM_FUNC(ArmReadCntpCval)
  mrs   x0, cntp_cval_el0        // Read CNTP_CTVAL (Physical Timer Compare Value Register)
  ret


ASM_FUNC(ArmWriteCntpCval)
  msr   cntp_cval_el0, x0        // Write to CNTP_CTVAL (Physical Timer Compare Value Register)
  ret


ASM_FUNC(ArmReadCntvCval)
  mrs   x0, cntv_cval_el0        // Read CNTV_CTVAL (Virtual Timer Compare Value Register)
  ret


ASM_FUNC(ArmWriteCntvCval)
  msr   cntv_cval_el0, x0        // write to  CNTV_CTVAL (Virtual Timer Compare Value Register)
  ret


ASM_FUNC(ArmReadCntvOff)
  mrs   x0, cntvoff_el2          // Read CNTVOFF (virtual Offset register)
  ret


ASM_FUNC(ArmWriteCntvOff)
  msr   cntvoff_el2, x0          // Write to CNTVOFF (Virtual Offset register)
  ret


ASM_FUNCTION_REMOVE_IF_UNREFERENCED
