;------------------------------------------------------------------------------
;
; Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   Switch the stack from temporary memory to permenent memory.
;
;------------------------------------------------------------------------------

    .586p
    .model  flat,C
    .code

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; Pei2LoaderSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
EXTERNDEF  C   MeasurePoint:PROC
Pei2LoaderSwitchStack   PROC C PUBLIC
    xor     eax, eax
    jmp     FspSwitchStack
Pei2LoaderSwitchStack   ENDP

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; Loader2PeiSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
Loader2PeiSwitchStack   PROC C PUBLIC
    jmp     FspSwitchStack
Loader2PeiSwitchStack   ENDP

;------------------------------------------------------------------------------
; UINT32
; EFIAPI
; FspSwitchStack (
;   VOID
;   )
;------------------------------------------------------------------------------
EXTERNDEF  C   SwapStack:PROC
FspSwitchStack   PROC C PUBLIC
    ; Save current contexts
    push    eax
    pushfd
    cli
    pushad
    sub     esp, 8
    sidt    fword ptr [esp]

    ; Load new stack
    push    esp
    call    SwapStack
    mov     esp, eax

    ; Restore previous contexts
    lidt    fword ptr [esp]
    add     esp, 8
    popad
    popfd
    add     esp, 4
    ret
FspSwitchStack   ENDP

    END
