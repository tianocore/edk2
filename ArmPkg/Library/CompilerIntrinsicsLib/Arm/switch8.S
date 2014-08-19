#/** @file
#  Compiler intrinsic for ARM compiler
#
#  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http)://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.;
#
#**/
#

.text
.p2align 2

GCC_ASM_EXPORT(__switch8)

ASM_PFX(__switch8):
    ldrb      ip, [lr, #-1]
    cmp       r0, ip
    ldrccsb   r0, [lr, r0]
    ldrcssb   r0, [lr, ip]
    add       ip, lr, r0, lsl #1
    bx        ip

