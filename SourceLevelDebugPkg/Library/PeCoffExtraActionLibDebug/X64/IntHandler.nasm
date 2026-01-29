;------------------------------------------------------------------------------
;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   IntHandler.nasm
;
; Abstract:
;
;   Assembly interrupt handler function.
;
;------------------------------------------------------------------------------

global ASM_PFX(AsmInterruptHandle)

DEFAULT REL
SECTION .text
ASM_PFX(AsmInterruptHandle):
    cli
    mov   al, 1
    iretq
