;------------------------------------------------------------------------------
;
; Copyright (c) 2006, Intel Corporation
; All rights reserved. This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   SwitchStack.Asm
;
; Abstract:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; Routine Description:
;
;   Routine for switching stacks with 1 parameter
;
; Arguments:
;
;   (rcx) EntryPoint    - Entry point with new stack.
;   (rdx) Context       - Parameter for entry point.
;   (r8)  Context2      - Parameter2 for entry point.
;   (r9)  NewStack      - Pointer to new stack.
;
; Returns:
;
;   None
;
;------------------------------------------------------------------------------
SwitchStack PROC
    mov     rax, rcx
    mov     rcx, rdx
    mov     rdx, r8
    lea     rsp, [r9 - 20h]
    call    rax
SwitchStack ENDP

    END
