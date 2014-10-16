;; @file
;   This is the assembly code for transferring to control to OS S3 waking vector
;   for X64 platform
;
; Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
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

EXTERN mOriginalHandler:QWORD
EXTERN PageFaultHandler:PROC

    .code
    
PUBLIC   AsmFixAddress16
PUBLIC   AsmJmpAddr32
    
AsmTransferControl  PROC
    ; rcx S3WakingVector    :DWORD
    ; rdx AcpiLowMemoryBase :DWORD
    lea   eax, @F
    mov   r8, 2800000000h
    or    rax, r8
    push  rax
    shrd  ebx, ecx, 20
    and   ecx, 0fh          
    mov   bx, cx          
    mov   [@jmp_addr], ebx
    retf
@@:
    DB    0b8h, 30h, 0      ; mov ax, 30h as selector
    mov   ds, eax
    mov   es, eax
    mov   fs, eax
    mov   gs, eax
    mov   ss, eax
    mov   rax, cr0
    mov   rbx, cr4        
    DB    66h
    and   eax, ((NOT 080000001h) AND 0ffffffffh)
    and   bl, NOT (1 SHL 5)
    mov   cr0, rax
    DB    66h
    mov   ecx, 0c0000080h
    rdmsr
    and   ah, NOT 1
    wrmsr
    mov   cr4, rbx
    DB    0eah              ; jmp far @jmp_addr
@jmp_addr DD  ?
AsmTransferControl  ENDP

AsmTransferControl32  PROC
    ; S3WakingVector    :DWORD
    ; AcpiLowMemoryBase :DWORD
    push  rbp
    mov   ebp, esp    
    DB    8dh, 05h          ;  lea   eax, AsmTransferControl16
AsmFixAddress16  DD ?
    push  28h               ; CS
    push  rax
    retf
AsmTransferControl32  ENDP

AsmTransferControl16  PROC
    DB    0b8h, 30h, 0      ; mov ax, 30h as selector
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax
    mov   ss, ax
    mov   rax, cr0          ; Get control register 0  
    DB    66h
    DB    83h, 0e0h, 0feh   ; and    eax, 0fffffffeh  ; Clear PE bit (bit #0)
    DB    0fh, 22h, 0c0h    ; mov    cr0, eax         ; Activate real mode
    DB    0eah              ; jmp far AsmJmpAddr32
AsmJmpAddr32 DD  ?
AsmTransferControl16  ENDP

PageFaultHandlerHook PROC
    push    rax                         ; save all volatile registers
    push    rcx
    push    rdx
    push    r8
    push    r9
    push    r10
    push    r11
    ; save volatile fp registers
    add     rsp, -68h
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
    add     rsp, 68h

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
