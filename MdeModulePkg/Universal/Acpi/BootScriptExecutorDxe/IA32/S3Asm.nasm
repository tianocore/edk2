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
    SECTION .text

global ASM_PFX(AsmFixAddress16)
global ASM_PFX(AsmJmpAddr32)

;-----------------------------------------
;VOID
;AsmTransferControl (
;  IN   UINT32           S3WakingVector,
;  IN   UINT32           AcpiLowMemoryBase
;  );
;-----------------------------------------

global ASM_PFX(AsmTransferControl)
ASM_PFX(AsmTransferControl):
    ; S3WakingVector    :DWORD
    ; AcpiLowMemoryBase :DWORD
    push  ebp
    mov   ebp, esp
    lea   eax, [.0]
    push  0x28               ; CS
    push  eax
    mov   ecx, [ebp + 8]
    shrd  ebx, ecx, 20
    and   ecx, 0xf
    mov   bx, cx
    mov   [@jmp_addr], ebx
    retf
.0:
    DB    0xb8, 0x30, 0      ; mov ax, 30h as selector
    mov   ds, ax
    mov   es, ax
    mov   fs, ax
    mov   gs, ax
    mov   ss, ax
    mov   eax, cr0          ; Get control register 0
    DB    0x66
    DB    0x83, 0xe0, 0xfe   ; and    eax, 0fffffffeh  ; Clear PE bit (bit #0)
    DB    0xf, 0x22, 0xc0    ; mov    cr0, eax         ; Activate real mode
    DB    0xea              ; jmp far @jmp_addr
@jmp_addr: DD 0

global ASM_PFX(AsmTransferControl32)
ASM_PFX(AsmTransferControl32):
  jmp ASM_PFX(AsmTransferControl)

; dummy
global ASM_PFX(AsmTransferControl16)
ASM_PFX(AsmTransferControl16):
ASM_PFX(AsmFixAddress16): DD 0
ASM_PFX(AsmJmpAddr32): DD 0

