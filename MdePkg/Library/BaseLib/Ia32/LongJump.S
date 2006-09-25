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
#   LongJump.Asm
#
# Abstract:
#
#   Implementation of _LongJump() on IA-32.
#
#------------------------------------------------------------------------------

.globl _InternalLongJump

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# InternalLongJump (
#   IN      BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer,
#   IN      UINTN                     Value
#   );
#------------------------------------------------------------------------------
_InternalLongJump:
    pop     %eax
    pop     %edx
    pop     %eax
    movl    (%edx), %ebx
    movl    4(%edx), %esi
    movl    8(%edx), %edi
    movl    12(%edx), %ebp
    movl    16(%edx), %esp
    jmp     *20(%edx)
