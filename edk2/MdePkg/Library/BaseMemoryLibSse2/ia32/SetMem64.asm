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

    .686
    .model  flat,C
    .xmm
    .code

;------------------------------------------------------------------------------
;  VOID *
;  _mem_SetMem64 (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT64 Value
;    )
;------------------------------------------------------------------------------
InternalMemSetMem64 PROC    USES    edi
    mov     ecx, [esp + 12]
    mov     edi, [esp + 8]
    test    edi, 8
    DB      0f2h, 0fh, 12h, 44h, 24h, 16    ; movddup xmm0, [esp + 16]
    jz      @F
    movq    [edi], xmm0
    add     edi, 8
    dec     ecx
@@:
    mov     edx, ecx
    shr     ecx, 1
    jz      @SetQwords
@@:
    movntdq [edi], xmm0
    add     edi, 16
    loop    @B
    mfence
@SetQwords:
    test    dl, 1
    jz      @F
    movq    [edi], xmm0
@@:
    ret
InternalMemSetMem64 ENDP

    END
