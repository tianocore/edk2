#------------------------------------------------------------------------------
#
# Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Module Name:
#
#  AsmSaveSecContext.S
#
# Abstract:
#
#   Save Sec Conext before call FspInit API
#
#------------------------------------------------------------------------------

#----------------------------------------------------------------------------
#  MMX Usage:
#              MM0 = BIST State
#              MM5 = Save time-stamp counter value high32bit
#              MM6 = Save time-stamp counter value low32bit.
#
#  It should be same as SecEntry.asm and PeiCoreEntry.asm.
#----------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(AsmSaveBistValue)
ASM_PFX(AsmSaveBistValue):
  movl    4(%esp), %eax
  movd    %eax, %mm0
  ret

ASM_GLOBAL ASM_PFX(AsmSaveTickerValue)
ASM_PFX(AsmSaveTickerValue):
  movl    4(%esp), %eax
  movd    %eax, %mm6
  movl    8(%esp), %eax
  movd    %eax, %mm5
  ret
