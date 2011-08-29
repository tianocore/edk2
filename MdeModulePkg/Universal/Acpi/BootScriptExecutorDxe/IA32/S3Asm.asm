;; @file
;   This is the assembly code for transferring to control to OS S3 waking vector
;   for IA32 platform
;
; Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
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
    .586P
    .model  flat,C
    .code

EXTERNDEF   AsmFixAddress16:DWORD
EXTERNDEF   AsmJmpAddr32:DWORD
   
;-----------------------------------------
;VOID
;AsmTransferControl (
;  IN   UINT32           S3WakingVector,
;  IN   UINT32           AcpiLowMemoryBase
;  );
;-----------------------------------------
   
AsmTransferControl  PROC
    ; S3WakingVector    :DWORD
    ; AcpiLowMemoryBase :DWORD
    push  ebp
    mov   ebp, esp    
    lea   eax, @F
    push  28h               ; CS
    push  eax
    mov   ecx, [ebp + 8]
    shrd  ebx, ecx, 20
    and   ecx, 0fh          
    mov   bx, cx          
    mov   @jmp_addr, ebx
    retf
@@:
    DB    0b8h, 30h, 0      ; mov ax, 30h as selector
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax
    mov   ss, ax
    mov   eax, cr0          ; Get control register 0  
    DB    66h
    DB    83h, 0e0h, 0feh   ; and    eax, 0fffffffeh  ; Clear PE bit (bit #0)
    DB    0fh, 22h, 0c0h    ; mov    cr0, eax         ; Activate real mode
    DB    0eah              ; jmp far @jmp_addr
@jmp_addr DD  ?

AsmTransferControl  ENDP

AsmTransferControl32  PROC
  jmp AsmTransferControl
AsmTransferControl32  ENDP

; dummy
AsmTransferControl16  PROC
AsmFixAddress16  DD ?
AsmJmpAddr32 DD  ?
AsmTransferControl16  ENDP

    END