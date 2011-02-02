#/** @file
#  Compiler intrinsic for ARM compiler
#
#  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.;
#
#**/
#        

.text
.p2align 2

GCC_ASM_EXPORT(__switch32)

ASM_PFX(__switch32):    
    ldr     ip, [lr, #-1]
    cmp     r0, ip
    add     r0, lr, r0, lsl #2
    ldrcc   r0, [r0, #3]
    add     ip, lr, ip, lsl #2
    ldrcs   r0, [ip, #3]
    add     ip, lr, r0
    bx      ip

