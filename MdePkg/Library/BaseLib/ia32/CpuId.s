#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation
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
#   CpuId.Asm
#
# Abstract:
#
#   AsmCpuid function
#
# Notes:
#
#------------------------------------------------------------------------------




#------------------------------------------------------------------------------
#  VOID
#  EFIAPI
#  AsmCpuid (
#    IN   UINT32  RegisterInEax,
#    OUT  UINT32  *RegisterOutEax  OPTIONAL,
#    OUT  UINT32  *RegisterOutEbx  OPTIONAL,
#    OUT  UINT32  *RegisterOutEcx  OPTIONAL,
#    OUT  UINT32  *RegisterOutEdx  OPTIONAL
#    )
#------------------------------------------------------------------------------
.globl _AsmCpuid
_AsmCpuid:
    push   %ebx
    push   %edi
    movl    12(%esp),%eax
    cpuid
    movl    %ecx,%edi
    movl    16(%esp),%ecx
    jecxz   L1
    movl    %eax,(%ecx)
L1:
    movl    20(%esp),%ecx
    jecxz   L2
    movl    %ebx,(%ecx)
L2:
    movl    24(%esp),%ecx
    jecxz   L3
    movl    %edi,(%ecx)
L3:
    movl    28(%esp),%ecx
    jecxz   L4
    movl    %edx,(%ecx)
L4:
    pop    %edi
    pop    %ebx
    ret
