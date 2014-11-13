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

#include <Base.h>

extern ASM_PFX(mTopOfApCommonStack):QWORD
extern ASM_PFX(ApEntryPointInC):PROC

.data

;
; This lock only allows one AP to use the mTopOfApCommonStack stack at a time
;
ApStackLock:
    dd      0

.code

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmApEntryPoint (
;   VOID
;   );
;------------------------------------------------------------------------------
ASM_PFX(AsmApEntryPoint) PROC PUBLIC

    cli
AsmApEntryPointAcquireLock:
lock bts    dword ptr [ApStackLock], 0
    pause
    jc      AsmApEntryPointAcquireLock

    mov     rsp, [ASM_PFX(mTopOfApCommonStack)]
    call    ASM_PFX(ApEntryPointInC)

    cli

lock btc    dword ptr [ApStackLock], 0

    mov     eax, 100h
AsmApEntryPointShareLock:
    pause
    dec     eax
    jnz     AsmApEntryPointShareLock

    jmp     ASM_PFX(AsmApEntryPoint)

ASM_PFX(AsmApEntryPoint) ENDP

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmApDoneWithCommonStack (
;   VOID
;   );
;------------------------------------------------------------------------------
ASM_PFX(AsmApDoneWithCommonStack) PROC PUBLIC

lock btc    dword ptr [ApStackLock], 0
    ret

ASM_PFX(AsmApDoneWithCommonStack) ENDP

END

