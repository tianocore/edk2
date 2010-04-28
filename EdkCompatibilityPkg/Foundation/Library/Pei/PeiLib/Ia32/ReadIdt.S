#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   ReadIdtBase.Asm
#
# Abstract:
#
#   ReadIdtBase function
#
# Notes:
#
#include "EfiBind.h"
#------------------------------------------------------------------------------

    .586: 
    #.MODEL flat,C
    .code: 

.globl ASM_PFX(ReadIdtBasea)
.globl ASM_PFX(ReadIdtLimita)

#------------------------------------------------------------------------------
# UINTN
# ReadIdtBase (
#   void
#   )
#
# Abstract: Returns physical address of IDTR
#
ASM_PFX(ReadIdtBasea):
   push   %ebp
   mov    %esp,%ebp
   add    $0xfffffff8,%esp
   sidtl  0xfffffffa(%ebp)
   mov    0xfffffffc(%ebp),%eax
   leave  
   ret    



#------------------------------------------------------------------------------
# UINT16
# ReadIdtLimit (
#   void
#   )
#
# Abstract: Returns Limit of IDTR
#
ASM_PFX(ReadIdtLimita):
   push   %ebp
   mov    %esp,%ebp
   add    $0xfffffff8,%esp
   sidtl  0xfffffffa(%ebp)
   mov    0xfffffffa(%ebp),%ax
   leave  
   ret    



