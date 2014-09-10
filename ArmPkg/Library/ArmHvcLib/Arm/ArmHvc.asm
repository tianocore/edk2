//
//  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
//  Copyright (c) 2014, Linaro Limited. All rights reserved.
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

    EXPORT ArmCallHvc

    AREA   ArmHvc, CODE, READONLY

ArmCallHvc
    push    {r4-r8}
    // r0 will be popped just after the HVC call
    push     {r0}

    // Load the HVC arguments values into the appropriate registers
    ldr     r7, [r0, #28]
    ldr     r6, [r0, #24]
    ldr     r5, [r0, #20]
    ldr     r4, [r0, #16]
    ldr     r3, [r0, #12]
    ldr     r2, [r0, #8]
    ldr     r1, [r0, #4]
    ldr     r0, [r0, #0]

    hvc     #0

    // Pop the ARM_HVC_ARGS structure address from the stack into r8
    pop     {r8}

    // Load the HVC returned values into the appropriate registers
    // A HVC call can return up to 4 values - we do not need to store back r4-r7.
    str     r3, [r8, #12]
    str     r2, [r8, #8]
    str     r1, [r8, #4]
    str     r0, [r8, #0]

    mov     r0, r8

    // Restore the registers r4-r8
    pop     {r4-r8}

    bx      lr

    END
