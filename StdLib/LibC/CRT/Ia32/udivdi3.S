#------------------------------------------------------------------------------
#
# Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available
# under the terms and conditions of the BSD License which accompanies this
# distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   MathDivU64x64.S
#
# Abstract:
#
#   64-bit Math Worker Function.
#   Divides a 64-bit unsigned value with a 64-bit unsigned value and returns
#   a 64-bit unsigned result.
#
#------------------------------------------------------------------------------

    .686:
    .code:

ASM_GLOBAL ASM_PFX(__udivdi3), ASM_PFX(DivU64x64Remainder)

#------------------------------------------------------------------------------
#
# void __cdecl __udivdi3 (void)
#
#------------------------------------------------------------------------------
ASM_PFX(__udivdi3):
    # Original local stack when calling __udivdi3
    #               -----------------
    #               |               |
    #               |---------------|
    #               |               |
    #               |--  Divisor  --|
    #               |               |
    #               |---------------|
    #               |               |
    #               |--  Dividend --|
    #               |               |
    #               |---------------|
    #               |  ReturnAddr** |
    #       ESP---->|---------------|
    #

    #
    # Set up the local stack for NULL Reminder pointer
    #
    xorl    %eax, %eax
    push    %eax

    #
    # Set up the local stack for Divisor parameter
    #
    movl    20(%esp), %eax
    push    %eax
    movl    20(%esp), %eax
    push    %eax

    #
    # Set up the local stack for Dividend parameter
    #
    movl    20(%esp), %eax
    push    %eax
    movl    20(%esp), %eax
    push    %eax

    #
    # Call native DivU64x64Remainder of BaseLib
    #
    jmp     ASM_PFX(DivU64x64Remainder)

    #
    # Adjust stack
    #
    addl    $20, %esp

    ret     $16
