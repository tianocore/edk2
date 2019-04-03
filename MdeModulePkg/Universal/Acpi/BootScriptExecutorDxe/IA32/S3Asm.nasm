;; @file
;   This is the assembly code for transferring to control to OS S3 waking vector
;   for IA32 platform
;
; Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
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
    mov   [@jmp_addr + 1], ebx
    retf

BITS 16
.0:
    mov   ax, 0x30
o32 mov   ds, eax
o32 mov   es, eax
o32 mov   fs, eax
o32 mov   gs, eax
o32 mov   ss, eax
    mov   eax, cr0          ; Get control register 0
    and   eax, 0x0fffffffe  ; Clear PE bit (bit #0)
    mov   cr0, eax          ; Activate real mode
@jmp_addr:
    jmp  0x0:0x0

global ASM_PFX(AsmTransferControl32)
ASM_PFX(AsmTransferControl32):
  jmp ASM_PFX(AsmTransferControl)

; dummy
global ASM_PFX(AsmTransferControl16)
ASM_PFX(AsmTransferControl16):
ASM_PFX(AsmFixAddress16): DD 0
ASM_PFX(AsmJmpAddr32): DD 0

