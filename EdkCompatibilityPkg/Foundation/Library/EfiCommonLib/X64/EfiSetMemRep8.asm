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
;   EfiSetMemRep8.asm
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
EfiCommonLibSetMem   PROC    USES    rdi rbx
    cmp     rdx, 0                      ; if Size == 0, do nothing
    je      @SetDone
    mov     rax, r8
    mov     bl,  al
    mov     bh,  bl
    mov     ax,  bx
    shl     rax, 10h
    mov     ax,  bx
    mov     ebx, eax 
    shl     rax, 20h
    mov     eax, ebx
    mov     rdi, rcx
    mov     rcx, rdx
    shr     rcx, 3
    rep     stosq
    mov     rcx, rdx
    and     rcx, 7
    rep     stosb
@SetDone:
    ret
EfiCommonLibSetMem   ENDP

    END

