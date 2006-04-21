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
;   SetMem.asm
;
; Abstract:
;
;   SetMem function
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
;  _mem_SetMem (
;    IN VOID   *Buffer,
;    IN UINTN  Count,
;    IN UINT8  Value
;    )
;------------------------------------------------------------------------------
InternalMemSetMem   PROC    USES    edi
    mov     ecx, [esp + 12]             ; ecx <- Count
    mov     edi, [esp + 8]              ; edi <- Buffer
    mov     edx, ecx
    shr     ecx, 3                      ; # of Qwords to set
    mov     al, [esp + 16]              ; al <- Value
    jz      @SetBytes
    mov     ah, al                      ; ax <- Value | (Value << 8)
    push    ecx
    push    ecx
    movq    [esp], mm0                  ; save mm0
    movd    mm0, eax
    pshufw  mm0, mm0, 0                 ; fill mm0 with 8 Value's
@@:
    movntq  [edi], mm0
    add     edi, 8
    loop    @B
    mfence
    movq    mm0, [esp]                  ; restore mm0
    pop     ecx                         ; stack cleanup
    pop     ecx
@SetBytes:
    and     edx, 7
    mov     ecx, edx
    rep     stosb
    mov     eax, [esp + 8]              ; eax <- Buffer as return value
    ret
InternalMemSetMem   ENDP

    END
