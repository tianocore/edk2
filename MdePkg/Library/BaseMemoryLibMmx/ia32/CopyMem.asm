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
;   CopyMem.asm
;
; Abstract:
;
;   CopyMem function
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
;  _mem_CopyMem (
;    IN VOID   *Destination,
;    IN VOID   *Source,
;    IN UINTN  Count
;    )
;------------------------------------------------------------------------------
InternalMemCopyMem  PROC    USES    esi edi
    mov     esi, [esp + 16]             ; esi <- Source
    mov     edi, [esp + 12]             ; edi <- Destination
    mov     edx, [esp + 20]             ; edx <- Count
    lea     eax, [edi + edx - 1]        ; eax <- End of Destination
    cmp     esi, edi
    jae     @F
    cmp     eax, esi                    ; Overlapped?
    jae     @CopyBackward               ; Copy backward if overlapped
@@:
    xor     ecx, ecx
    sub     ecx, esi
    and     ecx, 7                      ; ecx + esi aligns on 8-byte boundary
    jz      @F
    cmp     ecx, edx
    cmova   ecx, edx
    sub     edx, ecx                    ; edx <- remaining bytes to copy
    rep     movsb
@@:
    mov     ecx, edx
    and     edx, 7
    shr     ecx, 3                      ; ecx <- # of Qwords to copy
    jz      @CopyBytes
    push    eax
    push    eax
    movq    [esp], mm0                  ; save mm0
@@:
    movq    mm0, [esi]
    movntq  [edi], mm0
    add     esi, 8
    add     edi, 8
    loop    @B
    mfence
    movq    mm0, [esp]                  ; restore mm0
    pop     ecx                         ; stack cleanup
    pop     ecx                         ; stack cleanup
    jmp     @CopyBytes
@CopyBackward:
    mov     edi, eax                    ; edi <- Last byte in Destination
    lea     esi, [esi + edx - 1]        ; esi <- Last byte in Source
    std
@CopyBytes:
    mov     ecx, edx
    rep     movsb
    cld
    mov     eax, [esp + 12]
    ret
InternalMemCopyMem  ENDP

    END
