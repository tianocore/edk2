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
;   EfiZeroMemRep8.asm
;
; Abstract:
;
;   ZeroMem function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
;  VOID
;  EfiCommonLibZeroMem (
;    IN VOID   *Buffer,
;    IN UINTN  Size
;    );
;------------------------------------------------------------------------------
EfiCommonLibZeroMem  PROC    USES    rdi
    xor     rax, rax
    mov     rdi, rcx
    mov     rcx, rdx
    shr     rcx, 3
    and     rdx, 7
    rep     stosq
    mov     rcx, rdx
    rep     stosb
    ret
EfiCommonLibZeroMem  ENDP

    END