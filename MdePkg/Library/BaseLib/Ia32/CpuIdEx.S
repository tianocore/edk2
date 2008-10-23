#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2008, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   CpuIdEx.S
#
# Abstract:
#
#   AsmCpuidEx function
#
# Notes:
#
#------------------------------------------------------------------------------

    .686:
    .code:

#------------------------------------------------------------------------------
#  UINT32
#  EFIAPI
#  AsmCpuidEx (
#    IN   UINT32  RegisterInEax,
#    IN   UINT32  RegisterInEcx,
#    OUT  UINT32  *RegisterOutEax  OPTIONAL,
#    OUT  UINT32  *RegisterOutEbx  OPTIONAL,
#    OUT  UINT32  *RegisterOutEcx  OPTIONAL,
#    OUT  UINT32  *RegisterOutEdx  OPTIONAL
#    )
#------------------------------------------------------------------------------
.globl ASM_PFX(AsmCpuidEx)
ASM_PFX(AsmCpuidEx):
    push    %ebx
    push    %ebp
    movl    %esp, %ebp
    movl    12(%ebp), %eax
    movl    16(%ebp), %ecx
    cpuid
    push    %ecx
    movl    20(%ebp), %ecx
    jecxz   L1
    movl    %eax, (%ecx)
L1:
    movl    24(%ebp), %ecx
    jecxz   L2
    movl    %ebx, (%ecx)
L2:
    movl    28(%ebp), %ecx
    jecxz   L3
    popl    (%ecx)
L3:
    movl    32(%ebp), %edx
    jecxz   L4
    movl    %edx, (%ecx)
L4:
    movl    12(%ebp), %eax
    leave
    pop     %ebx
    ret
