#------------------------------------------------------------------------------
#
# LoongArch interrupt disable
#
# Copyright (c) 2022, Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(DisableInterrupts)

#/**
#  Disables CPU interrupts.
#**/

ASM_PFX(DisableInterrupts):
  li.w    $t0, 0x4
  csrxchg $zero, $t0, 0x0
  jirl    $zero, $ra, 0
  .end
