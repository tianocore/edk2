;------------------------------------------------------------------------------
;
; Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   FSP Debug functions
;
;------------------------------------------------------------------------------

    .386
    .model  flat,C
    .code

;------------------------------------------------------------------------------
; UINT32 *
; EFIAPI
; GetStackFramePointer (
;   VOID
;   );
;------------------------------------------------------------------------------
GetStackFramePointer  PROC  PUBLIC
    mov     eax, ebp
    ret
GetStackFramePointer  ENDP

    END
