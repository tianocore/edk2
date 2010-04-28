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
;   SetMem.asm
;
; Abstract:
;
;   memset function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
;  VOID *
;  memset (
;    OUT VOID   *Buffer,
;    IN  UINT8  Value,
;    IN  UINTN  Count
;    )
;------------------------------------------------------------------------------
memset   PROC    USES    rdi
    cmp     r8, 0                      ; if Size == 0, do nothing
    mov     r9,  rcx
    je      @SetDone
    mov     al,  dl
    mov     ah,  al
    shrd    edx, eax, 16
    shld    eax, edx, 16
    mov     rdi, rcx
    mov     rcx, r8
    shr     rcx, 2
    rep     stosd
    mov     rcx, r8
    and     rcx, 3
    rep     stosb
@SetDone:
    mov     rax, r9
    ret
memset   ENDP

    END

