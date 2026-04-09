;------------------------------------------------------------------------------
;
; Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   FSP Debug functions
;
;------------------------------------------------------------------------------

    SECTION .text

;------------------------------------------------------------------------------
; UINT32 *
; EFIAPI
; GetStackFramePointer (
;   VOID
;   );
;------------------------------------------------------------------------------
global ASM_PFX(GetStackFramePointer)
ASM_PFX(GetStackFramePointer):
    mov     rax, rbp
    ret

