;------------------------------------------------------------------------------
;
; Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;   EfiSetMemRep4.asm
;
; Abstract:
;
;   SetMem function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
; VOID
; EfiCommonLibSetMem (
;   OUT     VOID                      *Buffer,
;   IN      UINTN                     Size,
;   IN      UINT8                     Value
;   );
;------------------------------------------------------------------------------
EfiCommonLibSetMem   PROC    USES    rdi
    cmp     rdx, 0                      ; if Size == 0, do nothing
    je      @SetDone
    mov     rdi, rcx
    mov     al,  r8b
    mov     ah,  al
    shrd    ecx, eax, 16
    shld    eax, ecx, 16
    mov     rcx, rdx
    shr     rcx, 2
    rep     stosd
    mov     rcx, rdx
    and     rcx, 3
    rep     stosb
@SetDone:
    ret
EfiCommonLibSetMem   ENDP

    END

