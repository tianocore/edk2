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

    .686
    .model  flat,C
    .xmm
    .code

;------------------------------------------------------------------------------
;  VOID *
;  memset (
;    OUT VOID   *Buffer,
;    IN  UINT8  Value,
;    IN  UINTN  Count
;    )
;------------------------------------------------------------------------------
memset   PROC    USES    edi
    mov     edx, [esp + 16]             ; edx <- Count
    cmp     edx, 0                      ; if Count == 0, do nothing
    je      @SetDone    
    mov     edi, [esp + 8]              ; edi <- Buffer
    mov     al, [esp + 12]              ; al <- Value
    xor     ecx, ecx
    sub     ecx, edi
    and     ecx, 15                     ; ecx + edi aligns on 16-byte boundary
    jz      @F
    cmp     ecx, edx
    cmova   ecx, edx
    sub     edx, ecx
    rep     stosb
@@:
    mov     ecx, edx
    and     edx, 15
    shr     ecx, 4                      ; ecx <- # of DQwords to set
    jz      @SetBytes
    mov     ah, al                      ; ax <- Value | (Value << 8)
    add     esp, -16
    movd    xmm0, eax
    pshuflw xmm0, xmm0, 0               ; xmm0[0..63] <- Value repeats 8 times
    movlhps xmm0, xmm0                  ; xmm0 <- Value repeats 16 times
@@:
    movdqa  [edi], xmm0                 ; edi should be 16-byte aligned
    add     edi, 16
    loop    @B
    add     esp, 16                     ; stack cleanup
@SetBytes:
    mov     ecx, edx
    rep     stosb
@SetDone:    
    mov     eax, [esp + 8]              ; eax <- Buffer as return value
    ret
memset   ENDP

    END
