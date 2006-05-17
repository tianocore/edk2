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
#   SetMem64.asm
#
# Abstract:
#
#   SetMem64 function
#
# Notes:
#
#------------------------------------------------------------------------------

    .686: 
    #.MODEL flat,C
    .xmm: 
    .code: 

#------------------------------------------------------------------------------
#  VOID *
#  _mem_SetMem64 (
#    IN VOID   *Buffer,
#    IN UINTN  Count,
#    IN UINT64 Value
#    )
#------------------------------------------------------------------------------
.global _InternalMemSetMem64
_InternalMemSetMem64:
    push    %edi
    movq    16(%esp), %mm0
    movl    12(%esp), %ecx
    movl    8(%esp), %edi
    movl    %edi, %eax
L0: 
    movntq  %mm0, (%edi)
    addl    $8, %edi
    loopl   L0
    mfence
    pop     %edi
    ret
