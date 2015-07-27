;; @file
;   This is the assembly code for page fault handler hook.
;
; Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
;
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;;

EXTERN PageFaultHandler:PROC

    .code

PageFaultHandlerHook PROC
    add     rsp, -10h
    ; save rax
    mov     [rsp + 08h], rax

    ;push    rax                         ; save all volatile registers
    push    rcx
    push    rdx
    push    r8
    push    r9
    push    r10
    push    r11
    ; save volatile fp registers
    ; 68h + 08h(for alignment)
    add     rsp, -70h
    stmxcsr [rsp + 60h]
    movdqa  [rsp + 0h], xmm0
    movdqa  [rsp + 10h], xmm1
    movdqa  [rsp + 20h], xmm2
    movdqa  [rsp + 30h], xmm3
    movdqa  [rsp + 40h], xmm4
    movdqa  [rsp + 50h], xmm5

    add     rsp, -20h
    call    PageFaultHandler
    add     rsp, 20h

    ; load volatile fp registers
    ldmxcsr [rsp + 60h]
    movdqa  xmm0,  [rsp + 0h]
    movdqa  xmm1,  [rsp + 10h]
    movdqa  xmm2,  [rsp + 20h]
    movdqa  xmm3,  [rsp + 30h]
    movdqa  xmm4,  [rsp + 40h]
    movdqa  xmm5,  [rsp + 50h]
    add     rsp, 70h

    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    ;pop     rax                         ; restore all volatile registers

    add     rsp, 10h

    ; rax returned from PageFaultHandler is NULL or OriginalHandler address
    ; NULL if the page fault is handled by PageFaultHandler
    ; OriginalHandler address if the page fault is not handled by PageFaultHandler
    test    rax, rax

    ; save OriginalHandler address
    mov     [rsp - 10h], rax
    ; restore rax
    mov     rax, [rsp - 08h]

    jz      @F

    ; jump to OriginalHandler
    jmp     qword ptr [rsp - 10h]

@@:
    add     rsp, 08h                    ; skip error code for PF
    iretq
PageFaultHandlerHook ENDP

    END
