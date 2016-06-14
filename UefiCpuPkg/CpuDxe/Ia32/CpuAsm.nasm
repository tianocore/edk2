;------------------------------------------------------------------------------
;*
;*   Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
;*   This program and the accompanying materials
;*   are licensed and made available under the terms and conditions of the BSD License
;*   which accompanies this distribution.  The full text of the license may be found at
;*   http://opensource.org/licenses/bsd-license.php
;*
;*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;*
;*    CpuAsm.nasm
;*
;*   Abstract:
;*
;------------------------------------------------------------------------------

    SECTION .text

;------------------------------------------------------------------------------
; VOID
; SetCodeSelector (
;   UINT16 Selector
;   );
;------------------------------------------------------------------------------
global ASM_PFX(SetCodeSelector)
ASM_PFX(SetCodeSelector):
    mov     ecx, [esp+4]
    sub     esp, 0x10
    lea     eax, [setCodeSelectorLongJump]
    mov     [esp], eax
    mov     [esp+4], cx
    jmp     dword far [esp]
setCodeSelectorLongJump:
    add     esp, 0x10
    ret

;------------------------------------------------------------------------------
; VOID
; SetDataSelectors (
;   UINT16 Selector
;   );
;------------------------------------------------------------------------------
global ASM_PFX(SetDataSelectors)
ASM_PFX(SetDataSelectors):
    mov     ecx, [esp+4]
o16 mov     ss, cx
o16 mov     ds, cx
o16 mov     es, cx
o16 mov     fs, cx
o16 mov     gs, cx
    ret

