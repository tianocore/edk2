;------------------------------------------------------------------------------
;
; Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
;
; This program and the accompanying materials are licensed and made available
; under the terms and conditions of the BSD License which accompanies this
; distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

    EXPORT  InternalMemZeroMem
    EXPORT  InternalMemSetMem
    EXPORT  InternalMemSetMem16
    EXPORT  InternalMemSetMem32
    EXPORT  InternalMemSetMem64

    AREA    SetMem, CODE, READONLY, CODEALIGN, ALIGN=5
    THUMB

InternalMemZeroMem
    movs    r2, #0

InternalMemSetMem
    uxtb    r2, r2
    orr     r2, r2, r2, lsl #8

InternalMemSetMem16
    uxth    r2, r2
    orr     r2, r2, r2, lsr #16

InternalMemSetMem32
    mov     r3, r2

InternalMemSetMem64
    push    {r4, lr}
    cmp     r1, #16                 ; fewer than 16 bytes of input?
    add     r1, r1, r0              ; r1 := dst + length
    add     lr, r0, #16
    blt     L2
    bic     lr, lr, #15             ; align output pointer

    str     r2, [r0]                ; potentially unaligned store of 4 bytes
    str     r3, [r0, #4]            ; potentially unaligned store of 4 bytes
    str     r2, [r0, #8]            ; potentially unaligned store of 4 bytes
    str     r3, [r0, #12]           ; potentially unaligned store of 4 bytes
    beq     L1

L0
    add     lr, lr, #16             ; advance the output pointer by 16 bytes
    subs    r4, r1, lr              ; past the output?
    blt     L3                      ; break out of the loop
    strd    r2, r3, [lr, #-16]      ; aligned store of 16 bytes
    strd    r2, r3, [lr, #-8]
    bne     L0                      ; goto beginning of loop
L1
    pop     {r4, pc}

L2
    subs    r4, r1, lr
L3
    adds    r4, r4, #16
    subs    r1, r1, #8
    cmp     r4, #4                  ; between 4 and 15 bytes?
    blt     L4
    cmp     r4, #8                  ; between 8 and 15 bytes?
    str     r2, [lr, #-16]          ; overlapping store of 4 + (4 + 4) + 4 bytes
    itt     gt
    strgt   r3, [lr, #-12]
    strgt   r2, [r1]
    str     r3, [r1, #4]
    pop     {r4, pc}

L4
    cmp     r4, #2                  ; 2 or 3 bytes?
    strb    r2, [lr, #-16]          ; store 1 byte
    it      ge
    strhge  r2, [r1, #6]            ; store 2 bytes
    pop     {r4, pc}

    END
