;------------------------------------------------------------------------------
;
; Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
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
    add     rsp, -20h
    call    PageFaultHandler
    add     rsp, 20h
    test    al, al
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    pop     rax                         ; restore all volatile registers
    jnz     @F
    jmp     mOriginalHandler
@@:
    add     rsp, 08h                    ; skip error code for PF
    iretq
PageFaultHandlerHook ENDP
  END
