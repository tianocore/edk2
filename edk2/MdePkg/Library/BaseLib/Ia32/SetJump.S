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
#   SetJump.Asm
#
# Abstract:
#
#   Implementation of SetJump() on IA-32.
#
#------------------------------------------------------------------------------

.global _SetJump, _InternalAssertJumpBuffer

#------------------------------------------------------------------------------
# UINTN
# EFIAPI
# SetJump (
#   OUT     BASE_LIBRARY_JUMP_BUFFER  *JumpBuffer
#   );
#------------------------------------------------------------------------------
_SetJump:
    pushl   0x4(%esp)
    call    _InternalAssertJumpBuffer
    pop     %ecx
    pop     %ecx
    movl    (%esp), %edx
    movl    %ebx, (%edx)
    movl    %esi, 4(%edx)
    movl    %edi, 8(%edx)
    movl    %ebp, 12(%edx)
    movl    %esp, 16(%edx)
    movl    %ecx, 20(%edx)
    xorl    %eax, %eax
    jmp     *%ecx
