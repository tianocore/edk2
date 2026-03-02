;------------------------------------------------------------------------------
; IA32/StackCookieInterrupt.nasm
;
; Copyright (c) Microsoft Corporation.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;------------------------------------------------------------------------------

#include <Library/StackCheckLib.h>

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
; Checks the stack cookie value against __security_cookie and calls the
; stack cookie failure handler if there is a mismatch, passing along the
; exception address in ecx.
;
; VOID
; EFIAPI
; TriggerStackCookieInterrupt (
;   EFI_PHYSICAL_ADDRESS ExceptionAddress
;   );
;------------------------------------------------------------------------------
global ASM_PFX(TriggerStackCookieInterrupt)
ASM_PFX(TriggerStackCookieInterrupt):
    mov     ecx, [esp + 4]                              ; first parameter, skipping return address
    int     STACK_CHECK_EXCEPTION_VECTOR
    ret
