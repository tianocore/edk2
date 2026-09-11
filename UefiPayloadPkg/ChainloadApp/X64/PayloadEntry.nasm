;; @file
;  X64 payload entry - sets up stack and jumps to payload
;
;  Copyright (c) 2026, Amazon.com, Inc. or its affiliates. All Rights Reserved.<BR>
;  SPDX-License-Identifier: BSD-2-Clause-Patent
;;

    SECTION .text

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; JumpToPayload (
;   IN UINTN  NewStack,    // rcx
;   IN UINTN  HobList,     // rdx
;   IN UINTN  EntryPoint   // r8
;   );
;------------------------------------------------------------------------------
global ASM_PFX(JumpToPayload)
ASM_PFX(JumpToPayload):
    ;
    ; Mask interrupts, as the AArch64 stub does.  Not a hole today --
    ; CoreExitBootServices() calls gTimer->SetTimerPeriod (gTimer, 0), so
    ; the timer is already off -- but a device the outer firmware left
    ; armed can still raise an interrupt into an IDT that is about to
    ; become the payload's free RAM.
    ;
    cli
    mov     rsp, rcx        ; Set new stack
    and     rsp, ~0xF       ; Align to 16 bytes
    sub     rsp, 0x20       ; Shadow space
    mov     rcx, rdx        ; HobList as first arg
    call    r8              ; Call payload entry
    ; Never returns
.loop:
    hlt
    jmp     .loop
