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
#   SetMem.Asm
#
# Abstract:
#
#   SetMem function
#
# Notes:
#
#------------------------------------------------------------------------------

    .386:
    .code:

.globl _InternalMemSetMem

#------------------------------------------------------------------------------
#  VOID *
#  InternalMemSetMem (
#    IN VOID   *Buffer,
#    IN UINTN  Count,
#    IN UINT8  Value
#    )
#------------------------------------------------------------------------------
_InternalMemSetMem:
    push    %edi
    movl    12(%esp),%ecx
    movb    16(%esp),%al
    movb    %ah, %al
    shrd    $16, %eax, %edx
    shld    $16, %edx, %eax
    movl    %edx, %ecx
    movl    8(%esp),%edi
    shrd    $2, %ecx, %ecx
    rep    
    stosl
    movl    %ecx, %edx
    andl    $3, %ecx
    rep     
    stosb
    movl    8(%esp),%eax
    pop     %edi
    ret
