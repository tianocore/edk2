//
//  Copyright (c) 2012, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

    EXPORT ArmCallSmc
    EXPORT ArmCallSmcArg1
    EXPORT ArmCallSmcArg2
    EXPORT ArmCallSmcArg3

    AREA   ArmSmc, CODE, READONLY

ArmCallSmc
    push    {r1}
    mov     r1, r0
    ldr     r0,[r1]
    smc     #0
    str     r0,[r1]
    pop     {r1}
    bx      lr

ArmCallSmcArg1
    push    {r2-r3}
    mov     r2, r0
    mov     r3, r1
    ldr     r0,[r2]
    ldr     r1,[r3]
    smc     #0
    str     r0,[r2]
    str     r1,[r3]
    pop     {r2-r3}
    bx      lr

ArmCallSmcArg2
    push    {r3-r5}
    mov     r3, r0
    mov     r4, r1
    mov     r5, r2
    ldr     r0,[r3]
    ldr     r1,[r4]
    ldr     r2,[r5]
    smc     #0
    str     r0,[r3]
    str     r1,[r4]
    str     r2,[r5]
    pop     {r3-r5}
    bx      lr

ArmCallSmcArg3
    push    {r4-r7}
    mov     r4, r0
    mov     r5, r1
    mov     r6, r2
    mov     r7, r3
    ldr     r0,[r4]
    ldr     r1,[r5]
    ldr     r2,[r6]
    ldr     r3,[r7]
    smc     #0
    str     r0,[r4]
    str     r1,[r5]
    str     r2,[r6]
    str     r3,[r7]
    pop     {r4-r7}
    bx      lr

    END
