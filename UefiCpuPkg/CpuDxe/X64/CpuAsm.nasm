;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
;*   SPDX-License-Identifier: BSD-2-Clause-Patent
;*
;*    CpuAsm.nasm
;*
;*   Abstract:
;*
;------------------------------------------------------------------------------

    DEFAULT REL
    SECTION .text

;------------------------------------------------------------------------------
; VOID
; SetCodeSelector (
;   UINT16 Selector
;   );
;------------------------------------------------------------------------------
global ASM_PFX(SetCodeSelector)
ASM_PFX(SetCodeSelector):
    push    rcx
    lea     rax, [setCodeSelectorLongJump]
    push    rax
    o64 retf
setCodeSelectorLongJump:
    ret

;------------------------------------------------------------------------------
; VOID
; SetDataSelectors (
;   UINT16 Selector
;   );
;------------------------------------------------------------------------------
global ASM_PFX(SetDataSelectors)
ASM_PFX(SetDataSelectors):
o16 mov     ss, cx
o16 mov     ds, cx
o16 mov     es, cx
o16 mov     fs, cx
o16 mov     gs, cx
    ret

