#------------------------------------------------------------------------------
#
# Copyright (c) 2011 - 2013, ARM Limited. All rights reserved.
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

.text
.align 2

ASM_GLOBAL ASM_PFX(ArmReadCntFrq)
ASM_GLOBAL ASM_PFX(ArmWriteCntFrq)
ASM_GLOBAL ASM_PFX(ArmReadCntPct)
ASM_GLOBAL ASM_PFX(ArmReadCntkCtl)
ASM_GLOBAL ASM_PFX(ArmWriteCntkCtl)
ASM_GLOBAL ASM_PFX(ArmReadCntpTval)
ASM_GLOBAL ASM_PFX(ArmWriteCntpTval)
ASM_GLOBAL ASM_PFX(ArmReadCntpCtl)
ASM_GLOBAL ASM_PFX(ArmWriteCntpCtl)
ASM_GLOBAL ASM_PFX(ArmReadCntvTval)
ASM_GLOBAL ASM_PFX(ArmWriteCntvTval)
ASM_GLOBAL ASM_PFX(ArmReadCntvCtl)
ASM_GLOBAL ASM_PFX(ArmWriteCntvCtl)
ASM_GLOBAL ASM_PFX(ArmReadCntvCt)
ASM_GLOBAL ASM_PFX(ArmReadCntpCval)
ASM_GLOBAL ASM_PFX(ArmWriteCntpCval)
ASM_GLOBAL ASM_PFX(ArmReadCntvCval)
ASM_GLOBAL ASM_PFX(ArmWriteCntvCval)
ASM_GLOBAL ASM_PFX(ArmReadCntvOff)
ASM_GLOBAL ASM_PFX(ArmWriteCntvOff)

ASM_PFX(ArmReadCntFrq):
  mrs   x0, cntfrq_el0           // Read CNTFRQ
  ret


# NOTE - Can only write while at highest implemented EL level (EL3 on model). Else ReadOnly (EL2, EL1, EL0)
ASM_PFX(ArmWriteCntFrq):
  msr   cntfrq_el0, x0           // Write to CNTFRQ
  ret


ASM_PFX(ArmReadCntPct):
  mrs   x0, cntpct_el0           // Read CNTPCT (Physical counter register)
  ret


ASM_PFX(ArmReadCntkCtl):
  mrs   x0, cntkctl_el1          // Read CNTK_CTL (Timer PL1 Control Register)
  ret


ASM_PFX(ArmWriteCntkCtl):
  mrs   x0, cntkctl_el1          // Write to CNTK_CTL (Timer PL1 Control Register)
  ret


ASM_PFX(ArmReadCntpTval):
  mrs   x0, cntp_tval_el0        // Read CNTP_TVAL (PL1 physical timer value register)
  ret


ASM_PFX(ArmWriteCntpTval):
  msr   cntp_tval_el0, x0        // Write to CNTP_TVAL (PL1 physical timer value register)
  ret


ASM_PFX(ArmReadCntpCtl):
  mrs   x0, cntp_ctl_el0         // Read CNTP_CTL (PL1 Physical Timer Control Register)
  ret


ASM_PFX(ArmWriteCntpCtl):
  msr   cntp_ctl_el0, x0         // Write to  CNTP_CTL (PL1 Physical Timer Control Register)
  ret


ASM_PFX(ArmReadCntvTval):
  mrs   x0, cntv_tval_el0        // Read CNTV_TVAL (Virtual Timer Value register)
  ret


ASM_PFX(ArmWriteCntvTval):
  msr   cntv_tval_el0, x0        // Write to CNTV_TVAL (Virtual Timer Value register)
  ret


ASM_PFX(ArmReadCntvCtl):
  mrs   x0, cntv_ctl_el0         // Read CNTV_CTL (Virtual Timer Control Register)
  ret


ASM_PFX(ArmWriteCntvCtl):
  msr   cntv_ctl_el0, x0         // Write to CNTV_CTL (Virtual Timer Control Register)
  ret


ASM_PFX(ArmReadCntvCt):
  mrs  x0, cntvct_el0            // Read CNTVCT  (Virtual Count Register)
  ret


ASM_PFX(ArmReadCntpCval):
  mrs   x0, cntp_cval_el0        // Read CNTP_CTVAL (Physical Timer Compare Value Register)
  ret


ASM_PFX(ArmWriteCntpCval):
  msr   cntp_cval_el0, x0        // Write to CNTP_CTVAL (Physical Timer Compare Value Register)
  ret


ASM_PFX(ArmReadCntvCval):
  mrs   x0, cntv_cval_el0        // Read CNTV_CTVAL (Virtual Timer Compare Value Register)
  ret


ASM_PFX(ArmWriteCntvCval):
  msr   cntv_cval_el0, x0        // write to  CNTV_CTVAL (Virtual Timer Compare Value Register)
  ret


ASM_PFX(ArmReadCntvOff):
  mrs   x0, cntvoff_el2          // Read CNTVOFF (virtual Offset register)
  ret


ASM_PFX(ArmWriteCntvOff):
  msr   cntvoff_el2, x0          // Write to CNTVOFF (Virtual Offset register)
  ret


ASM_FUNCTION_REMOVE_IF_UNREFERENCED
