;------------------------------------------------------------------------------
;
; Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
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
    mov     eax, ebp
    ret

