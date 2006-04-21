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
;   SetMem64.asm
;
; Abstract:
;
;   SetMem64 function
;
; Notes:
;
;------------------------------------------------------------------------------

    .code

;------------------------------------------------------------------------------
;  VOID *
;  _mem_SetMem64 (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT8  Value
;    )
;------------------------------------------------------------------------------
InternalMemSetMem64 PROC    USES    rdi
    mov     rdi, rcx
    mov     r9, rcx
    test    cl, 8
    jz      @F
    mov     [rdi], r8
    add     rdi, 8
    dec     rdx
@@:
    mov     rcx, rdx
    shr     rcx, 1
    jz      @SetQwords
    movd    xmm0, r8
    movlhps xmm0, xmm0
@@:
    movntdq [rdi], xmm0
    add     rdi, 16
    loop    @B
    mfence
@SetQwords:
    test    dl, 1
    jz      @F
    mov     [rdi], r8
@@:
    ret
InternalMemSetMem64 ENDP

    END
