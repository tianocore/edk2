;------------------------------------------------------------------------------
;
; Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php.
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

extern ASM_PFX(mTopOfApCommonStack)
extern ASM_PFX(ApEntryPointInC)

SECTION .data

;
; This lock only allows one AP to use the mTopOfApCommonStack stack at a time
;
ApStackLock:
    dd      0

SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmApEntryPoint (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmApEntryPoint)
ASM_PFX(AsmApEntryPoint):
    cli
AsmApEntryPointAcquireLock:
lock bts    dword [ApStackLock], 0
    pause
    jc      AsmApEntryPointAcquireLock

    mov     esp, [ASM_PFX(mTopOfApCommonStack)]
    call    ASM_PFX(ApEntryPointInC)

    cli

lock btc    dword [ApStackLock], 0

    mov     eax, 0x100
AsmApEntryPointShareLock:
    pause
    dec     eax
    jnz     AsmApEntryPointShareLock

    jmp     ASM_PFX(AsmApEntryPoint)

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmApDoneWithCommonStack (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmApDoneWithCommonStack)
ASM_PFX(AsmApDoneWithCommonStack):
lock btc    dword [ApStackLock], 0
    ret

