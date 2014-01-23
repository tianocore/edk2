;------------------------------------------------------------------------------
;
; Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   PageFaultHandler.asm
;
; Abstract:
;
;   Defines page fault handler used to hook SMM IDT
;
;------------------------------------------------------------------------------

EXTERN mOriginalHandler:QWORD
EXTERN PageFaultHandler:PROC

  .code

PageFaultHandlerHook PROC
    push    rax                         ; save all volatile registers
    push    rcx
    push    rdx
    push    r8
    push    r9
    push    r10
    push    r11

    add     rsp, -10h * 6 - 8          ; reserve memory to store XMM registers and make address 16-byte alignment
    movdqa  [rsp], xmm0
    movdqa  [rsp + 10h], xmm1
    movdqa  [rsp + 20h], xmm2
    movdqa  [rsp + 30h], xmm3
    movdqa  [rsp + 40h], xmm4
    movdqa  [rsp + 50h], xmm5

    add     rsp, -20h
    call    PageFaultHandler
    add     rsp, 20h

    movdqa  xmm0, [rsp]
    movdqa  xmm1, [rsp + 10h] 
    movdqa  xmm2, [rsp + 20h]
    movdqa  xmm3, [rsp + 30h]
    movdqa  xmm4, [rsp + 40h]
    movdqa  xmm5, [rsp + 50h]
    add     rsp, 10h * 6 + 8
    
    test    al, al                      ; set ZF flag
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    pop     rax                         ; restore all volatile registers
    jnz     @F                          ; check ZF flag
    jmp     mOriginalHandler
@@:
    add     rsp, 08h                    ; skip error code for PF
    iretq
PageFaultHandlerHook ENDP
  END
