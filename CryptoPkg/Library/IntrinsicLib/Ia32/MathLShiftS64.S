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
#   MathLShiftS64.S
#
# Abstract:
#
#   64-bit Math Worker Function.
#   Shifts a 64-bit signed value left by a certain number of bits.
#
#------------------------------------------------------------------------------

    .686:
    .code:

ASM_GLOBAL ASM_PFX(__ashldi3)

#------------------------------------------------------------------------------
#
# void __cdecl __ashldi3 (void)
#
#------------------------------------------------------------------------------
ASM_PFX(__ashldi3):
    #
    # Handle shifting of 64 or more bits (return 0)
    #
    cmpb    $64, %cl
    jae     ReturnZero

    #
    # Handle shifting of between 0 and 31 bits
    #
    cmpb    $32, %cl
    jae     More32
    shld    %cl, %eax, %edx
    shl     %cl, %eax
    ret

    #
    # Handle shifting of between 32 and 63 bits
    #
More32:
    movl    %eax, %edx
    xor     %eax, %eax
    and     $31, %cl
    shl     %cl, %edx
    ret

ReturnZero:
    xor     %eax, %eax
    xor     %edx, %edx
    ret
