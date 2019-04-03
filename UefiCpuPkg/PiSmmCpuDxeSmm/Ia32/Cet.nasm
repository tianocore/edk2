;------------------------------------------------------------------------------ ;
; Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;-------------------------------------------------------------------------------

%include "Nasm.inc"

SECTION .text

global ASM_PFX(DisableCet)
ASM_PFX(DisableCet):

    ; Skip the pushed data for call
    mov     eax, 1
    INCSSP_EAX

    mov     eax, cr4
    btr     eax, 23                      ; clear CET
    mov     cr4, eax
    ret

global ASM_PFX(EnableCet)
ASM_PFX(EnableCet):

    mov     eax, cr4
    bts     eax, 23                      ; set CET
    mov     cr4, eax

    ; use jmp to skip the check for ret
    pop     eax
    jmp     eax

