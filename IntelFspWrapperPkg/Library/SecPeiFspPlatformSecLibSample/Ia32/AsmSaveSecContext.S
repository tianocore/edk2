#------------------------------------------------------------------------------
#
# Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
